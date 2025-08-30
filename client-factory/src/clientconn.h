#pragma once
// ===============================================
// Enhanced client connection with heartbeat, auto-reconnect, error handling
// Features: heartbeat timer, exponential backoff, send queue, connection quality
// ===============================================
#include <QtCore>
#include <QtNetwork>
#include <QTimer>
#include <QQueue>
#include "../../common/protocol.h"

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Error
};

enum class NetworkQuality {
    Unknown,
    Excellent,  // < 50ms RTT
    Good,       // 50-100ms RTT  
    Fair,       // 100-200ms RTT
    Poor        // > 200ms RTT
};

struct QueuedMessage {
    quint16 type;
    QJsonObject json;
    QByteArray bin;
    QString roomId;
    QString senderId;
    quint16 flags;
    QDateTime queueTime;
    int retryCount = 0;
};

class ClientConn : public QObject {
    Q_OBJECT
    
public:
    explicit ClientConn(QObject* parent = nullptr);
    ~ClientConn();
    
    // Connection management
    void connectTo(const QString& host, quint16 port);
    void disconnect();
    void setAutoReconnect(bool enabled) { autoReconnectEnabled_ = enabled; }
    void setReconnectDelay(int minMs, int maxMs) { minReconnectDelayMs_ = minMs; maxReconnectDelayMs_ = maxMs; }
    
    // Message sending
    void send(quint16 type, const QJsonObject& json, const QByteArray& bin = QByteArray());
    void sendWithOptions(quint16 type, const QJsonObject& json, const QByteArray& bin = QByteArray(),
                        const QString& roomId = QString(), const QString& senderId = QString(), 
                        quint16 flags = FLAG_NONE);
    
    // Heartbeat configuration
    void setHeartbeatInterval(int seconds) { heartbeatIntervalSec_ = seconds; }
    void setHeartbeatTimeout(int seconds) { heartbeatTimeoutSec_ = seconds; }
    
    // Queue management
    void setMaxQueueSize(int size) { maxQueueSize_ = size; }
    void clearQueue();
    int getQueueSize() const { return sendQueue_.size(); }
    
    // State information
    ConnectionState getState() const { return state_; }
    NetworkQuality getNetworkQuality() const { return networkQuality_; }
    QString getHost() const { return host_; }
    quint16 getPort() const { return port_; }
    qint64 getRttMs() const { return lastRttMs_; }
    int getReconnectAttempt() const { return reconnectAttempts_; }
    QDateTime getLastConnectedTime() const { return lastConnectedTime_; }
    QString getLastError() const { return lastError_; }
    
    // Statistics
    qint64 getBytesSent() const { return bytesSent_; }
    qint64 getBytesReceived() const { return bytesReceived_; }
    int getMessagesSent() const { return messagesSent_; }
    int getMessagesReceived() const { return messagesReceived_; }

signals:
    // Connection events
    void connected();
    void disconnected();
    void reconnecting(int attempt);
    void connectionError(const QString& error);
    
    // Message events
    void packetArrived(Packet pkt);
    void messageSent(quint32 seq);
    void messageAcknowledged(quint32 seq);
    
    // State events
    void stateChanged(ConnectionState state);
    void networkQualityChanged(NetworkQuality quality);
    
    // Queue events
    void queueFull();
    void queueCleared();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onHeartbeatTimer();
    void onHeartbeatTimeout();
    void onReconnectTimer();
    void processQueue();

private:
    // Connection
    QTcpSocket* socket_;
    QString host_;
    quint16 port_;
    ConnectionState state_;
    bool autoReconnectEnabled_;
    
    // Reconnection
    QTimer* reconnectTimer_;
    int reconnectAttempts_;
    int minReconnectDelayMs_;
    int maxReconnectDelayMs_;
    
    // Heartbeat
    QTimer* heartbeatTimer_;
    QTimer* heartbeatTimeoutTimer_;
    int heartbeatIntervalSec_;
    int heartbeatTimeoutSec_;
    QDateTime lastHeartbeatSent_;
    QDateTime lastHeartbeatReceived_;
    qint64 lastRttMs_;
    NetworkQuality networkQuality_;
    
    // Message queue
    QQueue<QueuedMessage> sendQueue_;
    QTimer* queueTimer_;
    int maxQueueSize_;
    QHash<quint32, QDateTime> pendingAcks_;
    
    // Buffer
    QByteArray buffer_;
    
    // Statistics
    qint64 bytesSent_;
    qint64 bytesReceived_;
    int messagesSent_;
    int messagesReceived_;
    QDateTime lastConnectedTime_;
    QString lastError_;
    
    // Helper methods
    void setState(ConnectionState state);
    void updateNetworkQuality(qint64 rttMs);
    void sendHeartbeat();
    void handleHeartbeatResponse(const Packet& packet);
    void queueMessage(const QueuedMessage& msg);
    void sendQueuedMessage(const QueuedMessage& msg);
    bool shouldDropMessage(const QueuedMessage& msg);
    int calculateReconnectDelay();
    void resetStatistics();
    void logError(const QString& error);
};