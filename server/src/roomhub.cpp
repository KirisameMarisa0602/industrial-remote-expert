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

    // Work order management operations (需要认证但不需要在房间中)
    if (p.type == MSG_CREATE_WORKORDER) {
        handleWorkOrderCreate(c, p);
        return;
    }
    
    if (p.type == MSG_LIST_WORKORDERS) {
        handleWorkOrderList(c, p);
        return;
    }
    
    if (p.type == MSG_UPDATE_WORKORDER) {
        handleWorkOrderUpdate(c, p);
        return;
    }
    
    if (p.type == MSG_DELETE_WORKORDER) {
        handleWorkOrderDelete(c, p);
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
    // Create data directory if it doesn't exist
    QDir dataDir("./data");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName("./data/server.db");
    
    if (!db_.open()) {
        qCritical() << "Cannot open database:" << db_.lastError().text();
        return false;
    }
    
    QSqlQuery query(db_);
    
    // 创建用户表 (updated with role and salt)
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT CHECK(role IN ('expert','factory')) NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    // 创建会话表 (updated to reference user_id instead of username)
    QString createSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            expires_at DATETIME NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )";
    
    if (!query.exec(createSessionsTable)) {
        qCritical() << "Failed to create sessions table:" << query.lastError().text();
        return false;
    }
    
    // 创建工单表
    QString createWorkOrdersTable = R"(
        CREATE TABLE IF NOT EXISTS work_orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            status TEXT CHECK(status IN ('open','in_progress','closed')) NOT NULL DEFAULT 'open',
            created_by INTEGER NOT NULL,
            assigned_to INTEGER NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (created_by) REFERENCES users (id),
            FOREIGN KEY (assigned_to) REFERENCES users (id)
        )
    )";
    
    if (!query.exec(createWorkOrdersTable)) {
        qCritical() << "Failed to create work_orders table:" << query.lastError().text();
        return false;
    }
    
    // 创建工单评论表 (optional)
    QString createWorkOrderCommentsTable = R"(
        CREATE TABLE IF NOT EXISTS work_order_comments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            work_order_id INTEGER NOT NULL,
            author_id INTEGER NOT NULL,
            body TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (work_order_id) REFERENCES work_orders (id),
            FOREIGN KEY (author_id) REFERENCES users (id)
        )
    )";
    
    if (!query.exec(createWorkOrderCommentsTable)) {
        qCritical() << "Failed to create work_order_comments table:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "Database initialized successfully";
    return true;
}

bool RoomHub::registerUser(const QString& username, const QString& password, const QString& role) {
    if (username.isEmpty() || password.isEmpty() || role.isEmpty()) {
        return false;
    }
    
    if (role != "expert" && role != "factory") {
        qWarning() << "Invalid role provided:" << role;
        return false;
    }
    
    // 生成随机盐值
    QString salt = generateSalt();
    
    // 使用盐值和密码生成哈希
    QString passwordHash = hashPassword(password, salt);
    
    QSqlQuery query(db_);
    query.prepare("INSERT INTO users (username, password_hash, salt, role) VALUES (?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    query.addBindValue(salt);
    query.addBindValue(role);
    
    if (!query.exec()) {
        qWarning() << "Failed to register user:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "User registered successfully:" << username << "with role:" << role;
    return true;
}

QString RoomHub::loginUser(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return QString();
    }
    
    QSqlQuery query(db_);
    query.prepare("SELECT id, salt, password_hash, role FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        qWarning() << "Login failed for user:" << username;
        return QString();
    }
    
    int userId = query.value(0).toInt();
    QString salt = query.value(1).toString();
    QString storedHash = query.value(2).toString();
    QString role = query.value(3).toString();
    
    // 验证密码
    QString passwordHash = hashPassword(password, salt);
    if (passwordHash != storedHash) {
        qWarning() << "Password verification failed for user:" << username;
        return QString();
    }
    
    // 生成会话令牌
    QString token = generateSessionToken();
    
    // 存储会话（24小时有效期）
    QSqlQuery sessionQuery(db_);
    sessionQuery.prepare("INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, datetime('now', '+24 hours'))");
    sessionQuery.addBindValue(token);
    sessionQuery.addBindValue(userId);
    
    if (!sessionQuery.exec()) {
        qWarning() << "Failed to create session:" << sessionQuery.lastError().text();
        return QString();
    }
    
    qInfo() << "User logged in successfully:" << username << "Role:" << role;
    return token;
}

bool RoomHub::validateSessionToken(const QString& token) {
    if (token.isEmpty()) {
        return false;
    }
    
    QSqlQuery query(db_);
    query.prepare("SELECT user_id FROM sessions WHERE token = ? AND expires_at > datetime('now')");
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

QString RoomHub::generateSalt() {
    // 生成16字节随机盐值
    QByteArray salt;
    for (int i = 0; i < 16; ++i) {
        salt.append(static_cast<char>(qrand() % 256));
    }
    return QString(salt.toBase64());
}

QString RoomHub::hashPassword(const QString& password, const QString& salt) {
    // 使用SHA-256和盐值进行密码哈希
    QString combined = salt + password;
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

void RoomHub::handleRegister(ClientCtx* c, const Packet& p) {
    QString username = p.json.value("username").toString();
    QString password = p.json.value("password").toString();
    QString role = p.json.value("role").toString();
    
    if (username.isEmpty() || password.isEmpty() || role.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "username, password and role required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    if (role != "expert" && role != "factory") {
        QJsonObject response{{"code", 400}, {"message", "role must be 'expert' or 'factory'"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    if (registerUser(username, password, role)) {
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
        // 获取用户信息
        QSqlQuery query(db_);
        query.prepare("SELECT id, role FROM users WHERE username = ?");
        query.addBindValue(username);
        
        if (query.exec() && query.next()) {
            int userId = query.value(0).toInt();
            QString role = query.value(1).toString();
            
            c->authenticated = true;
            c->sessionToken = token;
            c->user = username;
            c->userId = userId;
            c->userRole = role;
            
            QJsonObject response{
                {"code", 0}, 
                {"message", "login successful"}, 
                {"token", token},
                {"role", role},
                {"username", username}
            };
            c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        } else {
            QJsonObject response{{"code", 500}, {"message", "failed to retrieve user information"}};
            c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        }
    } else {
        QJsonObject response{{"code", 401}, {"message", "invalid username or password"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    }
}

/* ---------- 工单管理系统 ---------- */

void RoomHub::handleWorkOrderCreate(ClientCtx* c, const Packet& p) {
    QString title = p.json.value("title").toString();
    QString description = p.json.value("description").toString();
    
    if (title.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "title is required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    QSqlQuery query(db_);
    query.prepare("INSERT INTO work_orders (title, description, created_by) VALUES (?, ?, ?)");
    query.addBindValue(title);
    query.addBindValue(description);
    query.addBindValue(c->userId);
    
    if (!query.exec()) {
        qWarning() << "Failed to create work order:" << query.lastError().text();
        QJsonObject response{{"code", 500}, {"message", "failed to create work order"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    // 获取新创建的工单ID
    qint64 workOrderId = query.lastInsertId().toLongLong();
    
    QJsonObject response{
        {"code", 0}, 
        {"message", "work order created successfully"},
        {"workOrderId", QString::number(workOrderId)}
    };
    c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    
    qInfo() << "Work order created by user" << c->user << "ID:" << workOrderId << "Title:" << title;
}

void RoomHub::handleWorkOrderList(ClientCtx* c, const Packet& p) {
    QString filter = p.json.value("filter").toString();
    
    QSqlQuery query(db_);
    QString sql = R"(
        SELECT w.id, w.title, w.description, w.status, w.created_at, w.updated_at,
               creator.username as creator_name, assignee.username as assignee_name
        FROM work_orders w
        LEFT JOIN users creator ON w.created_by = creator.id
        LEFT JOIN users assignee ON w.assigned_to = assignee.id
    )";
    
    // 添加过滤条件
    if (filter == "open") {
        sql += " WHERE w.status = 'open'";
    } else if (filter == "assigned_to_me") {
        sql += " WHERE w.assigned_to = ?";
    } else if (filter == "created_by_me") {
        sql += " WHERE w.created_by = ?";
    }
    
    sql += " ORDER BY w.created_at DESC";
    
    query.prepare(sql);
    if (filter == "assigned_to_me" || filter == "created_by_me") {
        query.addBindValue(c->userId);
    }
    
    if (!query.exec()) {
        qWarning() << "Failed to list work orders:" << query.lastError().text();
        QJsonObject response{{"code", 500}, {"message", "failed to retrieve work orders"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    QJsonArray workOrders;
    while (query.next()) {
        QJsonObject workOrder{
            {"id", query.value("id").toInt()},
            {"title", query.value("title").toString()},
            {"description", query.value("description").toString()},
            {"status", query.value("status").toString()},
            {"createdAt", query.value("created_at").toString()},
            {"updatedAt", query.value("updated_at").toString()},
            {"creatorName", query.value("creator_name").toString()},
            {"assigneeName", query.value("assignee_name").toString()}
        };
        workOrders.append(workOrder);
    }
    
    QJsonObject response{
        {"code", 0},
        {"message", "work orders retrieved successfully"},
        {"workOrders", workOrders}
    };
    c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
}

void RoomHub::handleWorkOrderUpdate(ClientCtx* c, const Packet& p) {
    int workOrderId = p.json.value("id").toInt();
    QString status = p.json.value("status").toString();
    int assignedTo = p.json.value("assigned_to").toInt();
    QString title = p.json.value("title").toString();
    QString description = p.json.value("description").toString();
    
    if (workOrderId <= 0) {
        QJsonObject response{{"code", 400}, {"message", "valid work order ID is required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    // 检查工单是否存在
    QSqlQuery checkQuery(db_);
    checkQuery.prepare("SELECT created_by FROM work_orders WHERE id = ?");
    checkQuery.addBindValue(workOrderId);
    
    if (!checkQuery.exec() || !checkQuery.next()) {
        QJsonObject response{{"code", 404}, {"message", "work order not found"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    // 构建动态更新查询
    QStringList updateFields;
    QVariantList values;
    
    if (!status.isEmpty() && (status == "open" || status == "in_progress" || status == "closed")) {
        updateFields << "status = ?";
        values << status;
    }
    
    if (assignedTo > 0) {
        updateFields << "assigned_to = ?";
        values << assignedTo;
    }
    
    if (!title.isEmpty()) {
        updateFields << "title = ?";
        values << title;
    }
    
    if (!description.isEmpty()) {
        updateFields << "description = ?";
        values << description;
    }
    
    if (updateFields.isEmpty()) {
        QJsonObject response{{"code", 400}, {"message", "no fields to update"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    updateFields << "updated_at = datetime('now')";
    
    QString sql = QString("UPDATE work_orders SET %1 WHERE id = ?").arg(updateFields.join(", "));
    values << workOrderId;
    
    QSqlQuery updateQuery(db_);
    updateQuery.prepare(sql);
    for (const QVariant& value : values) {
        updateQuery.addBindValue(value);
    }
    
    if (!updateQuery.exec()) {
        qWarning() << "Failed to update work order:" << updateQuery.lastError().text();
        QJsonObject response{{"code", 500}, {"message", "failed to update work order"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    QJsonObject response{
        {"code", 0},
        {"message", "work order updated successfully"}
    };
    c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    
    qInfo() << "Work order" << workOrderId << "updated by user" << c->user;
}

void RoomHub::handleWorkOrderDelete(ClientCtx* c, const Packet& p) {
    int workOrderId = p.json.value("id").toInt();
    
    if (workOrderId <= 0) {
        QJsonObject response{{"code", 400}, {"message", "valid work order ID is required"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    // 检查工单是否存在并验证权限（只有创建者可以删除）
    QSqlQuery checkQuery(db_);
    checkQuery.prepare("SELECT created_by FROM work_orders WHERE id = ?");
    checkQuery.addBindValue(workOrderId);
    
    if (!checkQuery.exec() || !checkQuery.next()) {
        QJsonObject response{{"code", 404}, {"message", "work order not found"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    int createdBy = checkQuery.value("created_by").toInt();
    if (createdBy != c->userId) {
        QJsonObject response{{"code", 403}, {"message", "only the creator can delete this work order"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    // 删除相关评论
    QSqlQuery deleteCommentsQuery(db_);
    deleteCommentsQuery.prepare("DELETE FROM work_order_comments WHERE work_order_id = ?");
    deleteCommentsQuery.addBindValue(workOrderId);
    deleteCommentsQuery.exec(); // 不检查错误，评论可能不存在
    
    // 删除工单
    QSqlQuery deleteQuery(db_);
    deleteQuery.prepare("DELETE FROM work_orders WHERE id = ?");
    deleteQuery.addBindValue(workOrderId);
    
    if (!deleteQuery.exec()) {
        qWarning() << "Failed to delete work order:" << deleteQuery.lastError().text();
        QJsonObject response{{"code", 500}, {"message", "failed to delete work order"}};
        c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
        return;
    }
    
    QJsonObject response{
        {"code", 0},
        {"message", "work order deleted successfully"}
    };
    c->sock->write(buildPacket(MSG_SERVER_EVENT, response));
    
    qInfo() << "Work order" << workOrderId << "deleted by user" << c->user;
}
