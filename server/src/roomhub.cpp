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
    db_.setDatabaseName("industrial_remote_expert.db");
    
    if (!db_.open()) {
        qCritical() << "Cannot open database:" << db_.lastError().text();
        return false;
    }
    
    // 创建用户表
    QSqlQuery query(db_);
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    // 创建会话表
    QString createSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at DATETIME NOT NULL,
            FOREIGN KEY (username) REFERENCES users (username)
        )
    )";
    
    if (!query.exec(createSessionsTable)) {
        qCritical() << "Failed to create sessions table:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "Database initialized successfully";
    return true;
}

bool RoomHub::registerUser(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return false;
    }
    
    // 简单的密码哈希（实际项目中应使用更安全的方法）
    QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    
    QSqlQuery query(db_);
    query.prepare("INSERT INTO users (username, password_hash) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    
    if (!query.exec()) {
        qWarning() << "Failed to register user:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "User registered successfully:" << username;
    return true;
}

QString RoomHub::loginUser(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return QString();
    }
    
    QString passwordHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    
    QSqlQuery query(db_);
    query.prepare("SELECT username FROM users WHERE username = ? AND password_hash = ?");
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "Login failed for user:" << username;
        return QString();
    }
    
    // 生成会话令牌
    QString token = generateSessionToken();
    
    // 存储会话（24小时有效期）
    QSqlQuery sessionQuery(db_);
    sessionQuery.prepare("INSERT INTO sessions (token, username, expires_at) VALUES (?, ?, datetime('now', '+24 hours'))");
    sessionQuery.addBindValue(token);
    sessionQuery.addBindValue(username);
    
    if (!sessionQuery.exec()) {
        qWarning() << "Failed to create session:" << sessionQuery.lastError().text();
        return QString();
    }
    
    qInfo() << "User logged in successfully:" << username;
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
    // 生成随机会话令牌
    QByteArray token = QUuid::createUuid().toByteArray().toBase64();
    return QString(token).remove('=').remove('+').remove('/');
}

void RoomHub::handleRegister(ClientCtx* c, const Packet& p) {
    QString username = p.json.value("username").toString();
    QString password = p.json.value("password").toString();
    
    if (username.isEmpty() || password.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "username and password required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    if (registerUser(username, password)) {
        QJsonObject response{{"code", 0}, {"message", "registration successful"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    } else {
        QJsonObject response{{"code", 409}, {"message", "username already exists or registration failed"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    }
}

void RoomHub::handleLogin(ClientCtx* c, const Packet& p) {
    QString username = p.json.value("username").toString();
    QString password = p.json.value("password").toString();
    
    if (username.isEmpty() || password.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "username and password required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    QString token = loginUser(username, password);
    if (!token.isEmpty()) {
        c->authenticated = true;
        c->sessionToken = token;
        c->user = username;
        
        QJsonObject response{{"code", 0}, {"message", "login successful"}, {"token", token}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    } else {
        QJsonObject response{{"code", 401}, {"message", "invalid username or password"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    }
}
