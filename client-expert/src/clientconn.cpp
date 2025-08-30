#include "clientconn.h"
#include <QDateTime>
#include <QRandomGenerator>
#include <QtMath>

ClientConn::ClientConn(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , port_(0)
    , state_(ConnectionState::Disconnected)
    , autoReconnectEnabled_(true)
    , reconnectTimer_(new QTimer(this))
    , reconnectAttempts_(0)
    , minReconnectDelayMs_(1000)
    , maxReconnectDelayMs_(30000)
    , heartbeatTimer_(new QTimer(this))
    , heartbeatTimeoutTimer_(new QTimer(this))
    , heartbeatIntervalSec_(30)
    , heartbeatTimeoutSec_(90)
    , lastRttMs_(0)
    , networkQuality_(NetworkQuality::Unknown)
    , queueTimer_(new QTimer(this))
    , maxQueueSize_(100)
    , bytesSent_(0)
    , bytesReceived_(0)
    , messagesSent_(0)
    , messagesReceived_(0)
{
    // Setup socket connections
    connect(socket_, &QTcpSocket::connected, this, &ClientConn::onSocketConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientConn::onSocketDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ClientConn::onSocketError);
    connect(socket_, &QTcpSocket::readyRead, this, &ClientConn::onReadyRead);
    
    // Setup timers
    reconnectTimer_->setSingleShot(true);
    connect(reconnectTimer_, &QTimer::timeout, this, &ClientConn::onReconnectTimer);
    
    heartbeatTimer_->setInterval(heartbeatIntervalSec_ * 1000);
    connect(heartbeatTimer_, &QTimer::timeout, this, &ClientConn::onHeartbeatTimer);
    
    heartbeatTimeoutTimer_->setSingleShot(true);
    heartbeatTimeoutTimer_->setInterval(heartbeatTimeoutSec_ * 1000);
    connect(heartbeatTimeoutTimer_, &QTimer::timeout, this, &ClientConn::onHeartbeatTimeout);
    
    queueTimer_->setInterval(100); // Process queue every 100ms
    connect(queueTimer_, &QTimer::timeout, this, &ClientConn::processQueue);
    queueTimer_->start();
}

ClientConn::~ClientConn()
{
    disconnect();
}

void ClientConn::connectTo(const QString& host, quint16 port)
{
    if (state_ == ConnectionState::Connected || state_ == ConnectionState::Connecting) {
        return; // Already connected or connecting
    }
    
    host_ = host;
    port_ = port;
    reconnectAttempts_ = 0;
    
    setState(ConnectionState::Connecting);
    socket_->connectToHost(host, port);
    
    qCInfo(logNetwork) << "Connecting to" << host << ":" << port;
}

void ClientConn::disconnect()
{
    heartbeatTimer_->stop();
    heartbeatTimeoutTimer_->stop();
    reconnectTimer_->stop();
    
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
    }
    
    setState(ConnectionState::Disconnected);
    clearQueue();
}

void ClientConn::send(quint16 type, const QJsonObject& json, const QByteArray& bin)
{
    sendWithOptions(type, json, bin);
}

void ClientConn::sendWithOptions(quint16 type, const QJsonObject& json, const QByteArray& bin,
                                const QString& roomId, const QString& senderId, quint16 flags)
{
    QueuedMessage msg;
    msg.type = type;
    msg.json = json;
    msg.bin = bin;
    msg.roomId = roomId;
    msg.senderId = senderId;
    msg.flags = flags;
    msg.queueTime = QDateTime::currentDateTime();
    
    queueMessage(msg);
}

void ClientConn::clearQueue()
{
    sendQueue_.clear();
    pendingAcks_.clear();
    emit queueCleared();
}

void ClientConn::onSocketConnected()
{
    setState(ConnectionState::Connected);
    reconnectAttempts_ = 0;
    lastConnectedTime_ = QDateTime::currentDateTime();
    lastHeartbeatReceived_ = QDateTime::currentDateTime();
    
    // Start heartbeat
    heartbeatTimer_->start();
    sendHeartbeat();
    
    qCInfo(logNetwork) << "Connected to server";
    emit connected();
}

void ClientConn::onSocketDisconnected()
{
    heartbeatTimer_->stop();
    heartbeatTimeoutTimer_->stop();
    
    if (state_ != ConnectionState::Disconnected) {
        setState(ConnectionState::Disconnected);
        
        qCInfo(logNetwork) << "Disconnected from server";
        emit disconnected();
        
        // Auto-reconnect if enabled
        if (autoReconnectEnabled_ && !host_.isEmpty()) {
            setState(ConnectionState::Reconnecting);
            int delay = calculateReconnectDelay();
            reconnectTimer_->start(delay);
            
            qCInfo(logNetwork) << "Scheduling reconnect in" << delay << "ms (attempt" << (reconnectAttempts_ + 1) << ")";
            emit reconnecting(reconnectAttempts_ + 1);
        }
    }
}

void ClientConn::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorMsg = socket_->errorString();
    logError(errorMsg);
    
    setState(ConnectionState::Error);
    emit connectionError(errorMsg);
    
    qCWarning(logNetwork) << "Socket error:" << error << errorMsg;
}

void ClientConn::onReadyRead()
{
    QByteArray newData = socket_->readAll();
    buffer_.append(newData);
    bytesReceived_ += newData.size();
    
    QVector<Packet> packets;
    QString error;
    
    if (drainPackets(buffer_, packets, &error)) {
        for (const Packet& packet : packets) {
            messagesReceived_++;
            
            // Handle special message types
            if (packet.type == MSG_HEARTBEAT) {
                handleHeartbeatResponse(packet);
            } else if (packet.type == MSG_ACK) {
                quint32 seq = packet.json.value("seq").toVariant().toUInt();
                if (pendingAcks_.contains(seq)) {
                    pendingAcks_.remove(seq);
                    emit messageAcknowledged(seq);
                }
            }
            
            emit packetArrived(packet);
        }
    } else if (!error.isEmpty()) {
        logError("Packet parsing error: " + error);
    }
}

void ClientConn::onHeartbeatTimer()
{
    if (state_ == ConnectionState::Connected) {
        sendHeartbeat();
    }
}

void ClientConn::onHeartbeatTimeout()
{
    qCWarning(logNetwork) << "Heartbeat timeout - disconnecting";
    logError("Heartbeat timeout");
    socket_->disconnectFromHost();
}

void ClientConn::onReconnectTimer()
{
    reconnectAttempts_++;
    
    if (state_ == ConnectionState::Reconnecting) {
        setState(ConnectionState::Connecting);
        socket_->connectToHost(host_, port_);
        
        qCInfo(logNetwork) << "Reconnect attempt" << reconnectAttempts_ << "to" << host_ << ":" << port_;
    }
}

void ClientConn::processQueue()
{
    if (state_ != ConnectionState::Connected || sendQueue_.isEmpty()) {
        return;
    }
    
    // Process up to 5 messages per cycle to avoid blocking
    int processed = 0;
    while (!sendQueue_.isEmpty() && processed < 5) {
        QueuedMessage msg = sendQueue_.dequeue();
        
        if (shouldDropMessage(msg)) {
            qCWarning(logNetwork) << "Dropping stale message type" << msg.type;
            continue;
        }
        
        sendQueuedMessage(msg);
        processed++;
    }
}

void ClientConn::setState(ConnectionState state)
{
    if (state_ != state) {
        state_ = state;
        emit stateChanged(state);
    }
}

void ClientConn::updateNetworkQuality(qint64 rttMs)
{
    lastRttMs_ = rttMs;
    
    NetworkQuality newQuality;
    if (rttMs < 50) {
        newQuality = NetworkQuality::Excellent;
    } else if (rttMs < 100) {
        newQuality = NetworkQuality::Good;
    } else if (rttMs < 200) {
        newQuality = NetworkQuality::Fair;
    } else {
        newQuality = NetworkQuality::Poor;
    }
    
    if (networkQuality_ != newQuality) {
        networkQuality_ = newQuality;
        emit networkQualityChanged(newQuality);
    }
}

void ClientConn::sendHeartbeat()
{
    QJsonObject heartbeat;
    heartbeat["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    heartbeat["clientId"] = "factory-client"; // Could be configurable
    
    lastHeartbeatSent_ = QDateTime::currentDateTime();
    
    QByteArray packet = buildPacket(MSG_HEARTBEAT, heartbeat, QByteArray(), QString(), "client");
    if (socket_->write(packet) > 0) {
        bytesSent_ += packet.size();
        heartbeatTimeoutTimer_->start(); // Reset timeout
        qCDebug(logNetwork) << "Heartbeat sent";
    }
}

void ClientConn::handleHeartbeatResponse(const Packet& packet)
{
    lastHeartbeatReceived_ = QDateTime::currentDateTime();
    heartbeatTimeoutTimer_->stop(); // Cancel timeout
    
    // Calculate RTT
    // qint64 serverTimestamp = packet.json.value("timestamp").toVariant().toLongLong(); // Unused for now
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 rtt = qAbs(currentTime - lastHeartbeatSent_.toMSecsSinceEpoch());
    
    updateNetworkQuality(rtt);
    
    qCDebug(logNetwork) << "Heartbeat response received, RTT:" << rtt << "ms";
}

void ClientConn::queueMessage(const QueuedMessage& msg)
{
    if (sendQueue_.size() >= maxQueueSize_) {
        // Remove oldest message to make room
        sendQueue_.dequeue();
        emit queueFull();
    }
    
    sendQueue_.enqueue(msg);
    
    // If connected, try to send immediately
    if (state_ == ConnectionState::Connected) {
        processQueue();
    }
}

void ClientConn::sendQueuedMessage(const QueuedMessage& msg)
{
    QByteArray packet = buildPacket(msg.type, msg.json, msg.bin, msg.roomId, msg.senderId, msg.flags);
    
    if (socket_->write(packet) > 0) {
        bytesSent_ += packet.size();
        messagesSent_++;
        
        // Track ACK if required
        if (msg.flags & FLAG_ACK_REQUIRED) {
            // Note: sequence number would be extracted from the built packet in a full implementation
            // For now, we'll use a simple timestamp-based tracking
            quint32 seq = static_cast<quint32>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);
            pendingAcks_[seq] = QDateTime::currentDateTime();
            emit messageSent(seq);
        }
        
        qCDebug(logNetwork) << "Message sent, type:" << msg.type << "size:" << packet.size();
    } else {
        // Failed to send, put back in queue for retry
        QueuedMessage retryMsg = msg;
        retryMsg.retryCount++;
        if (retryMsg.retryCount < 3) { // Max 3 retries
            sendQueue_.prepend(retryMsg);
        }
    }
}

bool ClientConn::shouldDropMessage(const QueuedMessage& msg)
{
    // Drop messages older than 30 seconds
    return msg.queueTime.secsTo(QDateTime::currentDateTime()) > 30;
}

int ClientConn::calculateReconnectDelay()
{
    // Exponential backoff with jitter
    int baseDelay = qMin(static_cast<int>(minReconnectDelayMs_ * qPow(2, qMin(reconnectAttempts_, 6))), maxReconnectDelayMs_);
    int jitter = QRandomGenerator::global()->bounded(baseDelay / 4); // 25% jitter
    return baseDelay + jitter;
}

void ClientConn::resetStatistics()
{
    bytesSent_ = 0;
    bytesReceived_ = 0;
    messagesSent_ = 0;
    messagesReceived_ = 0;
}

void ClientConn::logError(const QString& error)
{
    lastError_ = error;
    qCWarning(logNetwork) << "ClientConn error:" << error;
}