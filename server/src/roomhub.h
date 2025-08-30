#pragma once
// ===============================================
// server/src/roomhub.h
// Enhanced server: TCP listener with room management, persistence, heartbeats
// Features: room broadcasting, member management, SQLite persistence, heartbeat/timeout
// ===============================================
#include <QtCore>
#include <QtNetwork>
#include <QTimer>
#include "../../common/protocol.h"
#include "database.h"

struct ClientCtx {
    QTcpSocket* sock = nullptr;
    QString userId;    // User identifier (username or generated ID)
    QString roomId;    // Current room; empty if not in any room
    QString clientInfo; // IP address and connection info
    QDateTime lastHeartbeat; // Last heartbeat received
    QTimer* heartbeatTimer = nullptr; // Timer for heartbeat timeout
    qint64 bytesReceived = 0; // For rate limiting
    qint64 messagesReceived = 0; // For rate limiting
    QDateTime connectionTime;
    bool isAuthenticated = false; // Future: authentication state
};

class RoomHub : public QObject {
    Q_OBJECT
public:
    explicit RoomHub(QObject* parent = nullptr);
    ~RoomHub();
    
    // Server lifecycle
    bool start(quint16 port, const QString& dbPath = QString());
    void stop();
    
    // Configuration
    void setHeartbeatInterval(int seconds) { heartbeatIntervalSec_ = seconds; }
    void setHeartbeatTimeout(int seconds) { heartbeatTimeoutSec_ = seconds; }
    void setMaxClientsPerRoom(int max) { maxClientsPerRoom_ = max; }
    void setRateLimitEnabled(bool enabled) { rateLimitEnabled_ = enabled; }

    // Statistics
    int getClientCount() const { return clients_.size(); }
    int getRoomCount() const { return getRoomList().size(); }
    QStringList getRoomList() const;
    QStringList getRoomMembers(const QString& roomId) const;
    
    // Database access
    DatabaseManager* database() { return &db_; }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onHeartbeatTimeout();
    void checkInactiveClients();

private:
    QTcpServer server_;
    DatabaseManager db_;
    RateLimiter rateLimiter_;
    
    // Client and room management
    QHash<QTcpSocket*, ClientCtx*> clients_;
    QMultiHash<QString, QTcpSocket*> rooms_;
    
    // Configuration
    int heartbeatIntervalSec_ = 30;  // Send heartbeat every 30 seconds
    int heartbeatTimeoutSec_ = 90;   // Disconnect after 90 seconds without heartbeat
    int maxClientsPerRoom_ = 50;     // Maximum clients per room
    bool rateLimitEnabled_ = true;
    
    // Timers
    QTimer* cleanupTimer_ = nullptr;
    
    // Message handling
    void handlePacket(ClientCtx* c, const Packet& p);
    void handleJoinWorkorder(ClientCtx* c, const Packet& p);
    void handleLeaveWorkorder(ClientCtx* c, const Packet& p);
    void handleHeartbeat(ClientCtx* c, const Packet& p);
    void handleTextMessage(ClientCtx* c, const Packet& p);
    void handleDeviceData(ClientCtx* c, const Packet& p);
    
    // Room management
    void joinRoom(ClientCtx* c, const QString& roomId);
    void leaveRoom(ClientCtx* c);
    void broadcastToRoom(const QString& roomId,
                         const QByteArray& packet,
                         QTcpSocket* except = nullptr);
    void broadcastRoomMemberUpdate(const QString& roomId);
    
    // Client management
    void setupClient(ClientCtx* c);
    void cleanupClient(ClientCtx* c);
    void disconnectClient(ClientCtx* c, const QString& reason);
    void sendHeartbeatRequest(ClientCtx* c);
    void sendError(ClientCtx* c, ErrorCode code, const QString& message = QString());
    void sendAck(ClientCtx* c, quint32 seq);
    
    // Validation and security
    bool validatePacket(ClientCtx* c, const Packet& p);
    bool checkRateLimit(ClientCtx* c);
    QString generateClientId();
};