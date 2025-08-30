#pragma once
// ===============================================
// server/src/database.h
// SQLite database manager for persistent storage
// Tables: users, workorders, sessions, messages, recordings
// ===============================================

#include <QtCore>
#include <QtSql>
#include "../../common/protocol.h"

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    // Initialization
    bool initialize(const QString& dbPath = QString());
    void close();
    
    // Schema management
    bool createTables();
    bool upgradeSchema(int fromVersion, int toVersion);
    int getSchemaVersion();
    void setSchemaVersion(int version);

    // Users table operations
    struct User {
        int id = -1;
        QString username;
        QString email;
        QString passwordHash;  // Future: will implement password hashing
        QDateTime createdAt;
        QDateTime lastLoginAt;
        bool isActive = true;
    };
    
    bool insertUser(const User& user);
    bool updateUser(const User& user);
    QList<User> getUsers();
    User getUserByUsername(const QString& username);
    User getUserById(int id);

    // Work orders table operations  
    struct WorkOrder {
        int id = -1;
        QString title;
        QString description;
        QString status = "created"; // created, active, completed, cancelled
        int createdByUserId = -1;
        QDateTime createdAt;
        QDateTime updatedAt;
        QString metadata; // JSON string for additional data
    };
    
    bool insertWorkOrder(const WorkOrder& workOrder);
    bool updateWorkOrder(const WorkOrder& workOrder);
    QList<WorkOrder> getWorkOrders();
    WorkOrder getWorkOrderById(int id);
    QList<WorkOrder> getWorkOrdersByStatus(const QString& status);

    // Sessions table operations (room membership tracking)
    struct Session {
        int id = -1;
        QString roomId;
        QString userId;  // Can be username or client ID
        QDateTime joinedAt;
        QDateTime leftAt; // NULL if still active
        QString clientInfo; // IP, user agent, etc.
    };
    
    bool insertSession(const Session& session);
    bool updateSessionLeftAt(int sessionId, const QDateTime& leftAt);
    QList<Session> getActiveSessions();
    QList<Session> getSessionsByRoom(const QString& roomId);
    QList<Session> getSessionsByUser(const QString& userId);

    // Messages table operations
    struct Message {
        int id = -1;
        QString roomId;
        QString senderId;
        quint16 messageType;
        QString jsonPayload; // JSON as string
        QByteArray binaryPayload;
        QDateTime timestamp;
        quint32 sequenceNumber;
    };
    
    bool insertMessage(const Message& message);
    QList<Message> getMessagesByRoom(const QString& roomId, int limit = 100);
    QList<Message> getMessagesByRoomSince(const QString& roomId, const QDateTime& since);
    QList<Message> getRecentMessages(int limit = 50);

    // Recordings table operations (for future A/V recording)
    struct Recording {
        int id = -1;
        QString roomId;
        QString filename;
        QString type = "messages"; // messages, audio, video, device_data
        qint64 fileSize = 0;
        QDateTime startTime;
        QDateTime endTime;
        QString metadata; // JSON string
    };
    
    bool insertRecording(const Recording& recording);
    bool updateRecording(const Recording& recording);
    QList<Recording> getRecordingsByRoom(const QString& roomId);
    QList<Recording> getRecordingsByType(const QString& type);

    // Convenience methods
    bool logMessage(const Packet& packet);
    bool logSessionJoin(const QString& roomId, const QString& userId, const QString& clientInfo = QString());
    bool logSessionLeave(const QString& roomId, const QString& userId);
    
    // Statistics and cleanup
    int getTotalMessages();
    int getTotalSessions();
    int getActiveSessionCount();
    bool cleanupOldSessions(int daysOld = 30);
    bool cleanupOldMessages(int daysOld = 90);

    // Database info
    QString getDatabasePath() const { return db_.databaseName(); }
    bool isOpen() const { return db_.isOpen(); }

private:
    QSqlDatabase db_;
    QString dbPath_;
    static const int CURRENT_SCHEMA_VERSION = 1;
    
    bool executeQuery(const QString& sql, const QVariantList& params = QVariantList());
    QSqlQuery prepareQuery(const QString& sql);
    void logSqlError(const QString& operation, const QSqlError& error);
};