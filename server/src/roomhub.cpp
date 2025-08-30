#include "roomhub.h"
#include <QHostAddress>
#include <QDateTime>
#include <QUuid>
#include <QJsonObject>

RoomHub::RoomHub(QObject* parent) 
    : QObject(parent)
    , rateLimiter_(100, 60000) // 100 requests per minute
{
    // Setup cleanup timer for inactive clients
    cleanupTimer_ = new QTimer(this);
    connect(cleanupTimer_, &QTimer::timeout, this, &RoomHub::checkInactiveClients);
    cleanupTimer_->start(30000); // Check every 30 seconds
}

RoomHub::~RoomHub()
{
    stop();
}

bool RoomHub::start(quint16 port, const QString& dbPath)
{
    // Initialize database
    if (!db_.initialize(dbPath)) {
        qCCritical(logRoomHub) << "Failed to initialize database";
        return false;
    }
    
    // Setup server
    connect(&server_, &QTcpServer::newConnection, this, &RoomHub::onNewConnection);
    
    if (!server_.listen(QHostAddress::Any, port)) {
        qCCritical(logRoomHub) << "Failed to listen on port" << port << ":" << server_.errorString();
        return false;
    }
    
    qCInfo(logRoomHub) << "Server listening on" << server_.serverAddress().toString() << ":" << port;
    qCInfo(logRoomHub) << "Database:" << db_.getDatabasePath();
    qCInfo(logRoomHub) << "Heartbeat interval:" << heartbeatIntervalSec_ << "seconds";
    qCInfo(logRoomHub) << "Heartbeat timeout:" << heartbeatTimeoutSec_ << "seconds";
    
    return true;
}

void RoomHub::stop()
{
    if (server_.isListening()) {
        server_.close();
    }
    
    // Disconnect all clients
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        cleanupClient(it.value());
    }
    clients_.clear();
    rooms_.clear();
    
    db_.close();
    qCInfo(logRoomHub) << "Server stopped";
}

void RoomHub::onNewConnection()
{
    while (server_.hasPendingConnections()) {
        QTcpSocket* sock = server_.nextPendingConnection();
        auto* ctx = new ClientCtx;
        ctx->sock = sock;
        ctx->connectionTime = QDateTime::currentDateTime();
        ctx->lastHeartbeat = QDateTime::currentDateTime();
        ctx->clientInfo = QString("%1:%2").arg(sock->peerAddress().toString()).arg(sock->peerPort());
        ctx->userId = generateClientId();
        
        clients_.insert(sock, ctx);
        setupClient(ctx);
        
        qCInfo(logRoomHub) << "New client connected:" << ctx->clientInfo << "assigned ID:" << ctx->userId;
    }
}

void RoomHub::setupClient(ClientCtx* c)
{
    connect(c->sock, &QTcpSocket::readyRead, this, &RoomHub::onReadyRead);
    connect(c->sock, &QTcpSocket::disconnected, this, &RoomHub::onDisconnected);
    
    // Setup heartbeat timer
    c->heartbeatTimer = new QTimer(this);
    c->heartbeatTimer->setSingleShot(true);
    c->heartbeatTimer->setInterval(heartbeatTimeoutSec_ * 1000);
    connect(c->heartbeatTimer, &QTimer::timeout, this, &RoomHub::onHeartbeatTimeout);
    c->heartbeatTimer->start();
}

void RoomHub::onDisconnected()
{
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    
    auto it = clients_.find(sock);
    if (it == clients_.end()) return;
    
    ClientCtx* c = it.value();
    qCInfo(logRoomHub) << "Client disconnected:" << c->userId << "from room:" << c->roomId;
    
    cleanupClient(c);
    clients_.erase(it);
}

void RoomHub::cleanupClient(ClientCtx* c)
{
    if (!c) return;
    
    // Log session end in database
    if (!c->roomId.isEmpty()) {
        db_.logSessionLeave(c->roomId, c->userId);
        leaveRoom(c);
    }
    
    // Cleanup timers
    if (c->heartbeatTimer) {
        c->heartbeatTimer->deleteLater();
        c->heartbeatTimer = nullptr;
    }
    
    // Cleanup socket
    if (c->sock) {
        c->sock->deleteLater();
        c->sock = nullptr;
    }
    
    delete c;
}

void RoomHub::onReadyRead()
{
    auto* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    
    auto it = clients_.find(sock);
    if (it == clients_.end()) return;
    
    ClientCtx* c = it.value();
    
    // Read data and update statistics
    static QHash<QTcpSocket*, QByteArray> buffers;
    QByteArray& buf = buffers[sock];
    QByteArray newData = sock->readAll();
    buf.append(newData);
    c->bytesReceived += newData.size();
    
    // Parse packets
    QVector<Packet> packets;
    QString error;
    if (drainPackets(buf, packets, &error)) {
        for (const Packet& p : packets) {
            c->messagesReceived++;
            
            // Update last heartbeat time for any message
            c->lastHeartbeat = QDateTime::currentDateTime();
            c->heartbeatTimer->start(); // Reset timeout
            
            if (validatePacket(c, p) && checkRateLimit(c)) {
                handlePacket(c, p);
            }
        }
    } else if (!error.isEmpty()) {
        qCWarning(logRoomHub) << "Packet parsing error from" << c->userId << ":" << error;
        sendError(c, ERR_INVALID_FRAME, error);
    }
}

void RoomHub::handlePacket(ClientCtx* c, const Packet& p)
{
    qCDebug(logRoomHub) << "Handling packet type" << p.type << "from" << c->userId << "in room" << c->roomId;
    
    switch (p.type) {
        case MSG_JOIN_WORKORDER:
            handleJoinWorkorder(c, p);
            break;
            
        case MSG_LEAVE_WORKORDER:
            handleLeaveWorkorder(c, p);
            break;
            
        case MSG_HEARTBEAT:
            handleHeartbeat(c, p);
            break;
            
        case MSG_TEXT:
            handleTextMessage(c, p);
            break;
            
        case MSG_DEVICE_DATA:
        case MSG_AUDIO_FRAME:
        case MSG_VIDEO_FRAME:
        case MSG_CONTROL_CMD:
            handleDeviceData(c, p);
            break;
            
        default:
            qCWarning(logRoomHub) << "Unknown message type" << p.type << "from" << c->userId;
            sendError(c, ERR_INVALID_FRAME, QString("Unknown message type: %1").arg(p.type));
    }
}

void RoomHub::handleJoinWorkorder(ClientCtx* c, const Packet& p)
{
    QString roomId = p.json.value("roomId").toString();
    QString username = p.json.value("user").toString();
    
    if (roomId.isEmpty()) {
        sendError(c, ERR_ROOM_NOT_FOUND, "roomId required");
        return;
    }
    
    // Check room capacity
    if (rateLimitEnabled_ && rooms_.count(roomId) >= maxClientsPerRoom_) {
        sendError(c, ERR_RATE_LIMITED, "Room is full");
        return;
    }
    
    // Update user info
    if (!username.isEmpty()) {
        c->userId = username;
    }
    
    // Join room
    joinRoom(c, roomId);
    
    // Log to database
    db_.logSessionJoin(roomId, c->userId, c->clientInfo);
    
    // Send confirmation
    QJsonObject response;
    response["code"] = 0;
    response["message"] = "joined";
    response["roomId"] = roomId;
    response["userId"] = c->userId;
    response["memberCount"] = rooms_.count(roomId);
    
    QByteArray packet = buildPacket(MSG_SERVER_EVENT, response, QByteArray(), roomId, "server");
    c->sock->write(packet);
    
    // Broadcast member join to room
    broadcastRoomMemberUpdate(roomId);
    
    qCInfo(logRoomHub) << "User" << c->userId << "joined room" << roomId;
}

void RoomHub::handleLeaveWorkorder(ClientCtx* c, const Packet& p)
{
    Q_UNUSED(p);
    
    if (c->roomId.isEmpty()) {
        sendError(c, ERR_NOT_IN_ROOM, "Not in any room");
        return;
    }
    
    QString oldRoomId = c->roomId;
    leaveRoom(c);
    
    // Log to database
    db_.logSessionLeave(oldRoomId, c->userId);
    
    // Send confirmation
    QJsonObject response;
    response["code"] = 0;
    response["message"] = "left";
    response["roomId"] = oldRoomId;
    
    QByteArray packet = buildPacket(MSG_SERVER_EVENT, response);
    c->sock->write(packet);
    
    // Broadcast member leave to room
    broadcastRoomMemberUpdate(oldRoomId);
    
    qCInfo(logRoomHub) << "User" << c->userId << "left room" << oldRoomId;
}

void RoomHub::handleHeartbeat(ClientCtx* c, const Packet& p)
{
    // Send heartbeat response
    QJsonObject response;
    response["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    response["clientId"] = c->userId;
    
    QByteArray packet = buildPacket(MSG_HEARTBEAT, response, QByteArray(), c->roomId, "server");
    c->sock->write(packet);
    
    // Send ACK if requested
    if (p.flags & FLAG_ACK_REQUIRED) {
        sendAck(c, p.seq);
    }
}

void RoomHub::handleTextMessage(ClientCtx* c, const Packet& p)
{
    if (c->roomId.isEmpty()) {
        sendError(c, ERR_NOT_IN_ROOM, "Join a room first");
        return;
    }
    
    // Log message to database
    db_.logMessage(p);
    
    // Broadcast to room (excluding sender)
    QByteArray packet = buildPacket(p.type, p.json, p.bin, c->roomId, c->userId);
    broadcastToRoom(c->roomId, packet, c->sock);
    
    // Send ACK if requested
    if (p.flags & FLAG_ACK_REQUIRED) {
        sendAck(c, p.seq);
    }
}

void RoomHub::handleDeviceData(ClientCtx* c, const Packet& p)
{
    if (c->roomId.isEmpty()) {
        sendError(c, ERR_NOT_IN_ROOM, "Join a room first");
        return;
    }
    
    // Log message to database for device data
    if (p.type == MSG_DEVICE_DATA) {
        db_.logMessage(p);
    }
    
    // Broadcast to room (excluding sender)
    QByteArray packet = buildPacket(p.type, p.json, p.bin, c->roomId, c->userId);
    broadcastToRoom(c->roomId, packet, c->sock);
    
    // Send ACK if requested
    if (p.flags & FLAG_ACK_REQUIRED) {
        sendAck(c, p.seq);
    }
}

void RoomHub::joinRoom(ClientCtx* c, const QString& roomId)
{
    // Leave current room if any
    if (!c->roomId.isEmpty()) {
        leaveRoom(c);
    }
    
    c->roomId = roomId;
    rooms_.insert(roomId, c->sock);
}

void RoomHub::leaveRoom(ClientCtx* c)
{
    if (!c->roomId.isEmpty()) {
        // Remove from room index
        auto range = rooms_.equal_range(c->roomId);
        for (auto i = range.first; i != range.second; ) {
            if (i.value() == c->sock) {
                i = rooms_.erase(i);
            } else {
                ++i;
            }
        }
        c->roomId.clear();
    }
}

void RoomHub::broadcastToRoom(const QString& roomId, const QByteArray& packet, QTcpSocket* except)
{
    auto range = rooms_.equal_range(roomId);
    int sent = 0;
    
    for (auto i = range.first; i != range.second; ++i) {
        QTcpSocket* sock = i.value();
        if (sock != except && sock->state() == QTcpSocket::ConnectedState) {
            sock->write(packet);
            sent++;
        }
    }
    
    qCDebug(logRoomHub) << "Broadcast to room" << roomId << "sent to" << sent << "clients";
}

void RoomHub::broadcastRoomMemberUpdate(const QString& roomId)
{
    QStringList members = getRoomMembers(roomId);
    
    QJsonObject update;
    update["roomId"] = roomId;
    update["memberCount"] = members.size();
    update["members"] = QJsonArray::fromStringList(members);
    
    QByteArray packet = buildPacket(MSG_ROOM_STATE, update, QByteArray(), roomId, "server");
    broadcastToRoom(roomId, packet);
}

void RoomHub::onHeartbeatTimeout()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;
    
    // Find client by timer
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        ClientCtx* c = it.value();
        if (c->heartbeatTimer == timer) {
            qCWarning(logRoomHub) << "Heartbeat timeout for client" << c->userId << "- disconnecting";
            disconnectClient(c, "Heartbeat timeout");
            return;
        }
    }
}

void RoomHub::checkInactiveClients()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<ClientCtx*> toDisconnect;
    
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        ClientCtx* c = it.value();
        
        // Check if client has been silent too long
        qint64 secondsSinceHeartbeat = c->lastHeartbeat.secsTo(now);
        if (secondsSinceHeartbeat > heartbeatTimeoutSec_) {
            toDisconnect.append(c);
        }
    }
    
    // Disconnect inactive clients
    for (ClientCtx* c : toDisconnect) {
        qCWarning(logRoomHub) << "Disconnecting inactive client" << c->userId;
        disconnectClient(c, "Inactive timeout");
    }
}

void RoomHub::disconnectClient(ClientCtx* c, const QString& reason)
{
    if (c && c->sock) {
        qCInfo(logRoomHub) << "Disconnecting client" << c->userId << "reason:" << reason;
        c->sock->disconnectFromHost();
    }
}

void RoomHub::sendError(ClientCtx* c, ErrorCode code, const QString& message)
{
    QJsonObject error;
    error["code"] = static_cast<int>(code);
    error["message"] = message.isEmpty() ? errorCodeToString(code) : message;
    
    QByteArray packet = buildPacket(MSG_ERROR, error);
    c->sock->write(packet);
}

void RoomHub::sendAck(ClientCtx* c, quint32 seq)
{
    QJsonObject ack;
    ack["seq"] = static_cast<qint64>(seq);
    ack["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QByteArray packet = buildPacket(MSG_ACK, ack);
    c->sock->write(packet);
}

bool RoomHub::validatePacket(ClientCtx* c, const Packet& p)
{
    Q_UNUSED(c);
    
    // Basic validation - could be expanded
    if (p.type == 0) {
        return false;
    }
    
    // Validate JSON payload size
    if (p.json.size() > 100) { // Arbitrary limit for demo
        return false;
    }
    
    return true;
}

bool RoomHub::checkRateLimit(ClientCtx* c)
{
    if (!rateLimitEnabled_) {
        return true;
    }
    
    return rateLimiter_.checkRateLimit(c->userId);
}

QString RoomHub::generateClientId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QStringList RoomHub::getRoomList() const
{
    return rooms_.keys();
}

QStringList RoomHub::getRoomMembers(const QString& roomId) const
{
    QStringList members;
    auto range = rooms_.equal_range(roomId);
    
    for (auto i = range.first; i != range.second; ++i) {
        QTcpSocket* sock = i.value();
        auto it = clients_.find(sock);
        if (it != clients_.end()) {
            members.append(it.value()->userId);
        }
    }
    
    return members;
}