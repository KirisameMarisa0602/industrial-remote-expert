#include "database.h"
#include "../../common/protocol.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QJsonDocument>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    // Determine database path
    if (dbPath.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        dbPath_ = dataDir + "/industrial_remote_expert.db";
    } else {
        dbPath_ = dbPath;
        // Ensure directory exists
        QFileInfo info(dbPath_);
        QDir().mkpath(info.absolutePath());
    }

    // Open database
    db_ = QSqlDatabase::addDatabase("QSQLITE", "main");
    db_.setDatabaseName(dbPath_);
    
    if (!db_.open()) {
        qCCritical(logRoomHub) << "Failed to open database:" << db_.lastError().text();
        return false;
    }

    qCInfo(logRoomHub) << "Database opened:" << dbPath_;
    
    // Enable foreign keys and set performance options
    if (!executeQuery("PRAGMA foreign_keys = ON")) return false;
    if (!executeQuery("PRAGMA journal_mode = WAL")) return false;
    if (!executeQuery("PRAGMA synchronous = NORMAL")) return false;
    
    // Create tables if needed
    if (!createTables()) {
        qCCritical(logRoomHub) << "Failed to create database tables";
        return false;
    }
    
    return true;
}

void DatabaseManager::close()
{
    if (db_.isOpen()) {
        db_.close();
        qCInfo(logRoomHub) << "Database closed";
    }
}

bool DatabaseManager::createTables()
{
    // Check current schema version
    int currentVersion = getSchemaVersion();
    
    if (currentVersion == 0) {
        // Fresh database, create all tables
        
        // Users table
        if (!executeQuery(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                email TEXT,
                password_hash TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                last_login_at DATETIME,
                is_active BOOLEAN DEFAULT 1
            )
        )")) return false;

        // Work orders table
        if (!executeQuery(R"(
            CREATE TABLE IF NOT EXISTS workorders (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                title TEXT NOT NULL,
                description TEXT,
                status TEXT DEFAULT 'created',
                created_by_user_id INTEGER,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                metadata TEXT,
                FOREIGN KEY (created_by_user_id) REFERENCES users(id)
            )
        )")) return false;

        // Sessions table (room membership tracking)
        if (!executeQuery(R"(
            CREATE TABLE IF NOT EXISTS sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                room_id TEXT NOT NULL,
                user_id TEXT NOT NULL,
                joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                left_at DATETIME,
                client_info TEXT
            )
        )")) return false;

        // Messages table
        if (!executeQuery(R"(
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                room_id TEXT NOT NULL,
                sender_id TEXT NOT NULL,
                message_type INTEGER NOT NULL,
                json_payload TEXT,
                binary_payload BLOB,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                sequence_number INTEGER
            )
        )")) return false;

        // Recordings table
        if (!executeQuery(R"(
            CREATE TABLE IF NOT EXISTS recordings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                room_id TEXT NOT NULL,
                filename TEXT NOT NULL,
                type TEXT DEFAULT 'messages',
                file_size INTEGER DEFAULT 0,
                start_time DATETIME,
                end_time DATETIME,
                metadata TEXT
            )
        )")) return false;

        // Create indexes for performance
        executeQuery("CREATE INDEX IF NOT EXISTS idx_sessions_room_id ON sessions(room_id)");
        executeQuery("CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id)");
        executeQuery("CREATE INDEX IF NOT EXISTS idx_messages_room_id ON messages(room_id)");
        executeQuery("CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp)");
        executeQuery("CREATE INDEX IF NOT EXISTS idx_recordings_room_id ON recordings(room_id)");
        executeQuery("CREATE INDEX IF NOT EXISTS idx_workorders_status ON workorders(status)");

        setSchemaVersion(CURRENT_SCHEMA_VERSION);
        qCInfo(logRoomHub) << "Database tables created successfully";
    }
    else if (currentVersion < CURRENT_SCHEMA_VERSION) {
        // Upgrade schema
        if (!upgradeSchema(currentVersion, CURRENT_SCHEMA_VERSION)) {
            return false;
        }
    }
    
    return true;
}

bool DatabaseManager::upgradeSchema(int fromVersion, int toVersion)
{
    qCInfo(logRoomHub) << "Upgrading database schema from version" << fromVersion << "to" << toVersion;
    
    // Future: Add schema migration logic here
    
    setSchemaVersion(toVersion);
    return true;
}

int DatabaseManager::getSchemaVersion()
{
    QSqlQuery query = prepareQuery("PRAGMA user_version");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

void DatabaseManager::setSchemaVersion(int version)
{
    executeQuery(QString("PRAGMA user_version = %1").arg(version));
}

// User operations
bool DatabaseManager::insertUser(const User& user)
{
    QSqlQuery query = prepareQuery(R"(
        INSERT INTO users (username, email, password_hash, is_active)
        VALUES (?, ?, ?, ?)
    )");
    
    query.addBindValue(user.username);
    query.addBindValue(user.email);
    query.addBindValue(user.passwordHash);
    query.addBindValue(user.isActive);
    
    if (!query.exec()) {
        logSqlError("insertUser", query.lastError());
        return false;
    }
    
    return true;
}

// Work order operations
bool DatabaseManager::insertWorkOrder(const WorkOrder& workOrder)
{
    QSqlQuery query = prepareQuery(R"(
        INSERT INTO workorders (title, description, status, created_by_user_id, metadata)
        VALUES (?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(workOrder.title);
    query.addBindValue(workOrder.description);
    query.addBindValue(workOrder.status);
    query.addBindValue(workOrder.createdByUserId > 0 ? workOrder.createdByUserId : QVariant());
    query.addBindValue(workOrder.metadata);
    
    if (!query.exec()) {
        logSqlError("insertWorkOrder", query.lastError());
        return false;
    }
    
    return true;
}

// Session operations
bool DatabaseManager::insertSession(const Session& session)
{
    QSqlQuery query = prepareQuery(R"(
        INSERT INTO sessions (room_id, user_id, client_info)
        VALUES (?, ?, ?)
    )");
    
    query.addBindValue(session.roomId);
    query.addBindValue(session.userId);
    query.addBindValue(session.clientInfo);
    
    if (!query.exec()) {
        logSqlError("insertSession", query.lastError());
        return false;
    }
    
    return true;
}

bool DatabaseManager::updateSessionLeftAt(int sessionId, const QDateTime& leftAt)
{
    QSqlQuery query = prepareQuery("UPDATE sessions SET left_at = ? WHERE id = ?");
    query.addBindValue(leftAt);
    query.addBindValue(sessionId);
    
    if (!query.exec()) {
        logSqlError("updateSessionLeftAt", query.lastError());
        return false;
    }
    
    return true;
}

// Message operations
bool DatabaseManager::insertMessage(const Message& message)
{
    QSqlQuery query = prepareQuery(R"(
        INSERT INTO messages (room_id, sender_id, message_type, json_payload, binary_payload, timestamp, sequence_number)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(message.roomId);
    query.addBindValue(message.senderId);
    query.addBindValue(message.messageType);
    query.addBindValue(message.jsonPayload);
    query.addBindValue(message.binaryPayload);
    query.addBindValue(message.timestamp);
    query.addBindValue(message.sequenceNumber);
    
    if (!query.exec()) {
        logSqlError("insertMessage", query.lastError());
        return false;
    }
    
    return true;
}

QList<DatabaseManager::Message> DatabaseManager::getMessagesByRoom(const QString& roomId, int limit)
{
    QList<Message> messages;
    
    QSqlQuery query = prepareQuery(R"(
        SELECT id, room_id, sender_id, message_type, json_payload, binary_payload, timestamp, sequence_number
        FROM messages 
        WHERE room_id = ? 
        ORDER BY timestamp DESC 
        LIMIT ?
    )");
    
    query.addBindValue(roomId);
    query.addBindValue(limit);
    
    if (!query.exec()) {
        logSqlError("getMessagesByRoom", query.lastError());
        return messages;
    }
    
    while (query.next()) {
        Message msg;
        msg.id = query.value(0).toInt();
        msg.roomId = query.value(1).toString();
        msg.senderId = query.value(2).toString();
        msg.messageType = query.value(3).toUInt();
        msg.jsonPayload = query.value(4).toString();
        msg.binaryPayload = query.value(5).toByteArray();
        msg.timestamp = query.value(6).toDateTime();
        msg.sequenceNumber = query.value(7).toUInt();
        messages.append(msg);
    }
    
    return messages;
}

// Recording operations
bool DatabaseManager::insertRecording(const Recording& recording)
{
    QSqlQuery query = prepareQuery(R"(
        INSERT INTO recordings (room_id, filename, type, file_size, start_time, end_time, metadata)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(recording.roomId);
    query.addBindValue(recording.filename);
    query.addBindValue(recording.type);
    query.addBindValue(recording.fileSize);
    query.addBindValue(recording.startTime);
    query.addBindValue(recording.endTime);
    query.addBindValue(recording.metadata);
    
    if (!query.exec()) {
        logSqlError("insertRecording", query.lastError());
        return false;
    }
    
    return true;
}

// Convenience methods
bool DatabaseManager::logMessage(const Packet& packet)
{
    Message msg;
    msg.roomId = packet.roomId;
    msg.senderId = packet.senderId;
    msg.messageType = packet.type;
    msg.jsonPayload = QJsonDocument(packet.json).toJson(QJsonDocument::Compact);
    msg.binaryPayload = packet.bin;
    msg.timestamp = QDateTime::fromMSecsSinceEpoch(packet.timestampMs);
    msg.sequenceNumber = packet.seq;
    
    return insertMessage(msg);
}

bool DatabaseManager::logSessionJoin(const QString& roomId, const QString& userId, const QString& clientInfo)
{
    Session session;
    session.roomId = roomId;
    session.userId = userId;
    session.clientInfo = clientInfo;
    
    return insertSession(session);
}

bool DatabaseManager::logSessionLeave(const QString& roomId, const QString& userId)
{
    // Find the active session and mark it as left
    QSqlQuery query = prepareQuery(R"(
        SELECT id FROM sessions 
        WHERE room_id = ? AND user_id = ? AND left_at IS NULL 
        ORDER BY joined_at DESC 
        LIMIT 1
    )");
    
    query.addBindValue(roomId);
    query.addBindValue(userId);
    
    if (query.exec() && query.next()) {
        int sessionId = query.value(0).toInt();
        return updateSessionLeftAt(sessionId, QDateTime::currentDateTime());
    }
    
    return false;
}

// Statistics
int DatabaseManager::getTotalMessages()
{
    QSqlQuery query = prepareQuery("SELECT COUNT(*) FROM messages");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getActiveSessionCount()
{
    QSqlQuery query = prepareQuery("SELECT COUNT(*) FROM sessions WHERE left_at IS NULL");
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// Helper methods
bool DatabaseManager::executeQuery(const QString& sql, const QVariantList& params)
{
    QSqlQuery query = prepareQuery(sql);
    
    for (const QVariant& param : params) {
        query.addBindValue(param);
    }
    
    if (!query.exec()) {
        logSqlError("executeQuery", query.lastError());
        return false;
    }
    
    return true;
}

QSqlQuery DatabaseManager::prepareQuery(const QString& sql)
{
    QSqlQuery query(db_);
    if (!query.prepare(sql)) {
        logSqlError("prepareQuery", query.lastError());
    }
    return query;
}

void DatabaseManager::logSqlError(const QString& operation, const QSqlError& error)
{
    qCCritical(logRoomHub) << "SQL Error in" << operation << ":" << error.text();
    qCDebug(logRoomHub) << "Last query:" << error.databaseText();
}