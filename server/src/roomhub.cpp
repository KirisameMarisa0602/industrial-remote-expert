#include "roomhub.h"
#include <QCryptographicHash>
#include <QUuid>
#include <QSqlQuery>
#include <QSqlError>

RoomHub::RoomHub(QObject* parent) : QObject(parent) {
    if (!initDatabase()) {
        qCritical() << "Failed to initialize database";
    }
}

bool RoomHub::start(quint16 port) {
    connect(&server_, &QTcpServer::newConnection, this, &RoomHub::onNewConnection);
    if (!server_.listen(QHostAddress::Any, port)) {
        qWarning() << "Listen failed on port" << port << ":" << server_.errorString();
        return false;
    }
    qInfo() << "Server listening on" << server_.serverAddress().toString() << ":" << port;
    return true;
}

void RoomHub::onNewConnection() {
    while (server_.hasPendingConnections()) {
        QTcpSocket* sock = server_.nextPendingConnection();
        auto* ctx = new ClientCtx;
        ctx->sock = sock;
        clients_.insert(sock, ctx);

        qInfo() << "New client from" << sock->peerAddress().toString() << sock->peerPort();

        connect(sock, &QTcpSocket::readyRead, this, &RoomHub::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, &RoomHub::onDisconnected);
    }
}

void RoomHub::onDisconnected() {
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    auto it = clients_.find(sock);
    if (it == clients_.end()) return;
    ClientCtx* c = it.value();

    if (!c->roomId.isEmpty()) {
        // 从房间索引里移除
        auto range = rooms_.equal_range(c->roomId);
        for (auto i = range.first; i != range.second; ) {
            if (i.value() == sock) i = rooms_.erase(i);
            else ++i;
        }
    }
    qInfo() << "Client disconnected" << c->user << c->roomId;
    clients_.erase(it);
    sock->deleteLater();
    delete c;
}

void RoomHub::onReadyRead() {
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    auto it = clients_.find(sock);
    if (it == clients_.end()) return;
    ClientCtx* c = it.value();

    static QHash<QTcpSocket*, QByteArray> buffers; // 简易 per-socket 缓冲
    QByteArray& buf = buffers[sock];
    buf.append(sock->readAll());

    QVector<Packet> pkts;
    if (drainPackets(buf, pkts)) {
        for (const Packet& p : pkts) {
            handlePacket(c, p);
        }
    }
}

void RoomHub::handlePacket(ClientCtx* c, const Packet& p) {
    // 处理注册请求
    if (p.type == MSG_REGISTER) {
        handleRegister(c, p);
        return;
    }
    
    // 处理登录请求
    if (p.type == MSG_LOGIN) {
        handleLogin(c, p);
        return;
    }
    
    // 房间操作需要认证
    if (p.type == MSG_JOIN_WORKORDER) {
        if (!c->authenticated) {
            QJsonObject j{{"code",401},{"message","authentication required"}};
            c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
            return;
        }
        
        const QString roomId = p.json.value("roomId").toString();
        const QString user   = p.json.value("user").toString();
        if (roomId.isEmpty()) {
            QJsonObject j{{"code",400},{"message","roomId required"}};
            c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
            return;
        }
        c->user = user;
        joinRoom(c, roomId);
        QJsonObject j{{"code",0},{"message","joined"},{"roomId",roomId}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
        qInfo() << "Join" << roomId << "user" << (user.isEmpty() ? "(anonymous)" : user);
        return;
    }

    // 其他操作也需要认证且加入房间
    if (!c->authenticated) {
        QJsonObject j{{"code",401},{"message","authentication required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
        return;
    }

    if (c->roomId.isEmpty()) {
        QJsonObject j{{"code",403},{"message","join a room first"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
        return;
    }

    // 简单转发（同房间广播，排除发送者）
    if (p.type == MSG_TEXT || p.type == MSG_DEVICE_DATA ||
        p.type == MSG_VIDEO_FRAME || p.type == MSG_AUDIO_FRAME ||
        p.type == MSG_CONTROL_CMD) {
        // 保持原包结构，服务端不改内容
        QByteArray raw = buildPacket(p.type, p.json, p.bin);
        broadcastToRoom(c->roomId, raw, c->sock);
        return;
    }

    // 未识别类型：回一个提示
    QJsonObject j{{"code",404},{"message",QString("unknown type %1").arg(p.type)}};
    c->sock->write(buildPacket(MSG_SERVER_EVENT, j));
}

void RoomHub::joinRoom(ClientCtx* c, const QString& roomId) {
    // 先从原房间移除
    if (!c->roomId.isEmpty()) {
        auto range = rooms_.equal_range(c->roomId);
        for (auto i = range.first; i != range.second; ) {
            if (i.value() == c->sock) i = rooms_.erase(i);
            else ++i;
        }
    }
    c->roomId = roomId;
    rooms_.insert(roomId, c->sock);
}

void RoomHub::broadcastToRoom(const QString& roomId, const QByteArray& packet, QTcpSocket* except) {
    auto range = rooms_.equal_range(roomId);
    for (auto i = range.first; i != range.second; ++i) {
        QTcpSocket* s = i.value();
        if (s == except) continue;
        s->write(packet);
    }
}

/* ---------- 用户认证系统 ---------- */

bool RoomHub::initDatabase() {
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    
    // Use configurable database path
    QString dbPath = "server/data/app.db";
    // TODO: Read from config.ini when implemented
    
    // Ensure data directory exists
    QDir dataDir("server/data");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    db_.setDatabaseName(dbPath);
    
    if (!db_.open()) {
        qCritical() << "Cannot open database:" << db_.lastError().text();
        return false;
    }
    
    QSqlQuery query(db_);
    
    // Enhanced users table with role and proper security
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            salt BLOB NOT NULL,
            password_hash BLOB NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('factory','expert')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    // Enhanced sessions table
    QString createSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at DATETIME NOT NULL,
            FOREIGN KEY(username) REFERENCES users(username)
        )
    )";
    
    if (!query.exec(createSessionsTable)) {
        qCritical() << "Failed to create sessions table:" << query.lastError().text();
        return false;
    }
    
    // Tickets table for work order management
    QString createTicketsTable = R"(
        CREATE TABLE IF NOT EXISTS tickets (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ticket_no TEXT UNIQUE NOT NULL,
            title TEXT,
            status TEXT NOT NULL DEFAULT 'open',
            created_by TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(created_by) REFERENCES users(username)
        )
    )";
    
    if (!query.exec(createTicketsTable)) {
        qCritical() << "Failed to create tickets table:" << query.lastError().text();
        return false;
    }
    
    // Ticket participants table
    QString createTicketParticipantsTable = R"(
        CREATE TABLE IF NOT EXISTS ticket_participants (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ticket_no TEXT NOT NULL,
            username TEXT NOT NULL,
            joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(ticket_no) REFERENCES tickets(ticket_no),
            FOREIGN KEY(username) REFERENCES users(username)
        )
    )";
    
    if (!query.exec(createTicketParticipantsTable)) {
        qCritical() << "Failed to create ticket_participants table:" << query.lastError().text();
        return false;
    }
    
    // Messages table for chat persistence
    QString createMessagesTable = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            ticket_no TEXT NOT NULL,
            sender TEXT NOT NULL,
            type TEXT NOT NULL,
            text TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(ticket_no) REFERENCES tickets(ticket_no),
            FOREIGN KEY(sender) REFERENCES users(username)
        )
    )";
    
    if (!query.exec(createMessagesTable)) {
        qCritical() << "Failed to create messages table:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "Database initialized successfully at" << dbPath;
    return true;
}

bool RoomHub::registerUser(const QString& username, const QString& password, const QString& role) {
    if (username.isEmpty() || password.isEmpty() || role.isEmpty()) {
        return false;
    }
    
    if (role != "factory" && role != "expert") {
        qWarning() << "Invalid role:" << role;
        return false;
    }
    
    // Generate random salt (16 bytes)
    QByteArray salt(16, 0);
    for (int i = 0; i < 16; ++i) {
        salt[i] = static_cast<char>(qrand() % 256);
    }
    
    // Create salted hash: SHA-256(salt + password)
    QByteArray saltedPassword = salt + password.toUtf8();
    QByteArray passwordHash = QCryptographicHash::hash(saltedPassword, QCryptographicHash::Sha256);
    
    QSqlQuery query(db_);
    query.prepare("INSERT INTO users (username, salt, password_hash, role) VALUES (?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(salt);
    query.addBindValue(passwordHash);
    query.addBindValue(role);
    
    if (!query.exec()) {
        qWarning() << "Failed to register user:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "User registered successfully:" << username << "role:" << role;
    return true;
}

QString RoomHub::loginUser(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return QString();
    }
    
    // Retrieve user salt and hash
    QSqlQuery query(db_);
    query.prepare("SELECT salt, password_hash, role FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "User not found:" << username;
        return QString();
    }
    
    QByteArray storedSalt = query.value(0).toByteArray();
    QByteArray storedHash = query.value(1).toByteArray();
    QString userRole = query.value(2).toString();
    
    // Verify password
    QByteArray saltedPassword = storedSalt + password.toUtf8();
    QByteArray computedHash = QCryptographicHash::hash(saltedPassword, QCryptographicHash::Sha256);
    
    if (computedHash != storedHash) {
        qWarning() << "Password verification failed for user:" << username;
        return QString();
    }
    
    // Generate session token
    QString token = generateSessionToken();
    
    // Store session (24 hour validity)
    QSqlQuery sessionQuery(db_);
    sessionQuery.prepare("INSERT INTO sessions (token, username, expires_at) VALUES (?, ?, datetime('now', '+24 hours'))");
    sessionQuery.addBindValue(token);
    sessionQuery.addBindValue(username);
    
    if (!sessionQuery.exec()) {
        qWarning() << "Failed to create session:" << sessionQuery.lastError().text();
        return QString();
    }
    
    qInfo() << "User logged in successfully:" << username << "role:" << userRole;
    return token;
}

bool RoomHub::validateSessionToken(const QString& token) {
    if (token.isEmpty()) {
        return false;
    }
    
    QSqlQuery query(db_);
    query.prepare("SELECT username FROM sessions WHERE token = ? AND expires_at > datetime('now')");
    query.addBindValue(token);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    return true;
}

QString RoomHub::generateSessionToken() {
    // Generate cryptographically random 32-byte hex token
    QByteArray tokenBytes(32, 0);
    for (int i = 0; i < 32; ++i) {
        tokenBytes[i] = static_cast<char>(qrand() % 256);
    }
    return tokenBytes.toHex();
}

QString RoomHub::getUserRole(const QString& username) {
    if (username.isEmpty()) {
        return QString();
    }
    
    QSqlQuery query(db_);
    query.prepare("SELECT role FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        return QString();
    }
    
    return query.value(0).toString();
}

void RoomHub::handleRegister(ClientCtx* c, const Packet& p) {
    QString username = p.json.value("username").toString();
    QString password = p.json.value("password").toString();
    QString role = p.json.value("role").toString();
    
    if (username.isEmpty() || password.isEmpty() || role.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "username, password and role required"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
        return;
    }
    
    if (role != "factory" && role != "expert") {
        QJsonObject response{{"code", 400}, {"message", "role must be 'factory' or 'expert'"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
        return;
    }
    
    if (registerUser(username, password, role)) {
        QJsonObject response{{"code", 0}, {"message", "registration successful"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
    } else {
        QJsonObject response{{"code", 409}, {"message", "username already exists or registration failed"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
    }
}

void RoomHub::handleLogin(ClientCtx* c, const Packet& p) {
    QString username = p.json.value("username").toString();
    QString password = p.json.value("password").toString();
    
    if (username.isEmpty() || password.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "username and password required"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
        return;
    }
    
    QString token = loginUser(username, password);
    if (!token.isEmpty()) {
        QString role = getUserRole(username);
        c->authenticated = true;
        c->sessionToken = token;
        c->user = username;
        c->role = role;
        
        QJsonObject response{
            {"code", 0}, 
            {"message", "login successful"}, 
            {"token", token},
            {"role", role}
        };
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
    } else {
        QJsonObject response{{"code", 401}, {"message", "invalid username or password"}};
        c->sock->write(buildPacket(MSG_AUTH_RESULT, response));
    }
}
