#include "recording.h"
#include <QUuid>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QJsonArray>

// FileRecordingManager implementation
FileRecordingManager::FileRecordingManager(QObject* parent)
    : IRecordingManager(parent)
    , maxFileSize_(100 * 1024 * 1024) // 100MB default
    , compressionEnabled_(false)
{
    // Default recording directory
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recordings";
    setRecordingDirectory(defaultDir);
}

FileRecordingManager::~FileRecordingManager()
{
    // Stop all active recordings
    QStringList sessionIds;
    for (auto it = activeSessions_.begin(); it != activeSessions_.end(); ++it) {
        sessionIds.append(it.key());
    }
    
    for (const QString& sessionId : sessionIds) {
        stopRecording(sessionId);
    }
}

bool FileRecordingManager::configure(const QJsonObject& config)
{
    if (config.contains("recordingDirectory")) {
        setRecordingDirectory(config["recordingDirectory"].toString());
    }
    if (config.contains("maxFileSize")) {
        maxFileSize_ = config["maxFileSize"].toVariant().toLongLong();
    }
    if (config.contains("compressionEnabled")) {
        compressionEnabled_ = config["compressionEnabled"].toBool();
    }
    
    return true;
}

QJsonObject FileRecordingManager::getConfiguration() const
{
    QJsonObject config;
    config["recordingDirectory"] = recordingDir_;
    config["maxFileSize"] = static_cast<qint64>(maxFileSize_);
    config["compressionEnabled"] = compressionEnabled_;
    return config;
}

void FileRecordingManager::setRecordingDirectory(const QString& path)
{
    recordingDir_ = path;
    QDir().mkpath(recordingDir_);
}

QString FileRecordingManager::startRecording(const QString& roomId, RecordingType type, const QJsonObject& metadata)
{
    QString sessionId = generateSessionId();
    
    auto* session = new ActiveSession();
    session->info = RecordingSession(sessionId, roomId, type);
    session->info.metadata = metadata;
    session->info.filename = generateFilename(roomId, type);
    
    if (!openSessionFile(session)) {
        delete session;
        return QString(); // Failed
    }
    
    QMutexLocker locker(&sessionsMutex_);
    activeSessions_[sessionId] = session;
    
    qCInfo(logRecording) << "Recording started:" << sessionId << "room:" << roomId << "file:" << session->info.filename;
    emit recordingStarted(sessionId);
    
    return sessionId;
}

bool FileRecordingManager::stopRecording(const QString& sessionId)
{
    QMutexLocker locker(&sessionsMutex_);
    
    auto it = activeSessions_.find(sessionId);
    if (it == activeSessions_.end()) {
        return false;
    }
    
    ActiveSession* session = it.value();
    session->info.endTime = QDateTime::currentDateTime();
    updateSessionStats(session);
    
    // Move to completed sessions
    completedSessions_.append(session->info);
    
    closeSessionFile(session);
    activeSessions_.erase(it);
    delete session;
    
    qCInfo(logRecording) << "Recording stopped:" << sessionId;
    emit recordingStopped(sessionId);
    
    return true;
}

bool FileRecordingManager::isRecording(const QString& sessionId) const
{
    QMutexLocker locker(&sessionsMutex_);
    return activeSessions_.contains(sessionId);
}

bool FileRecordingManager::recordMessage(const QString& sessionId, const Packet& packet)
{
    QMutexLocker locker(&sessionsMutex_);
    
    auto it = activeSessions_.find(sessionId);
    if (it == activeSessions_.end()) {
        return false;
    }
    
    ActiveSession* session = it.value();
    
    QJsonObject record;
    record["type"] = "message";
    record["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    record["messageType"] = packet.type;
    record["roomId"] = packet.roomId;
    record["senderId"] = packet.senderId;
    record["json"] = packet.json;
    record["seq"] = static_cast<qint64>(packet.seq);
    
    if (!packet.bin.isEmpty()) {
        record["binarySize"] = packet.bin.size();
        record["binaryData"] = QString::fromUtf8(packet.bin.toBase64());
    }
    
    return writeJsonLine(session, record);
}

bool FileRecordingManager::recordDeviceSample(const QString& sessionId, const DeviceSample& sample)
{
    QMutexLocker locker(&sessionsMutex_);
    
    auto it = activeSessions_.find(sessionId);
    if (it == activeSessions_.end()) {
        return false;
    }
    
    ActiveSession* session = it.value();
    
    QJsonObject record;
    record["type"] = "device_sample";
    record["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    record["sample"] = sample.toJson();
    
    return writeJsonLine(session, record);
}

QList<RecordingSession> FileRecordingManager::getActiveSessions() const
{
    QMutexLocker locker(&sessionsMutex_);
    
    QList<RecordingSession> sessions;
    for (auto it = activeSessions_.begin(); it != activeSessions_.end(); ++it) {
        sessions.append(it.value()->info);
    }
    
    return sessions;
}

QList<RecordingSession> FileRecordingManager::getSessionsByRoom(const QString& roomId) const
{
    QMutexLocker locker(&sessionsMutex_);
    
    QList<RecordingSession> sessions;
    
    // Check active sessions
    for (auto it = activeSessions_.begin(); it != activeSessions_.end(); ++it) {
        if (it.value()->info.roomId == roomId) {
            sessions.append(it.value()->info);
        }
    }
    
    // Check completed sessions
    for (const RecordingSession& session : completedSessions_) {
        if (session.roomId == roomId) {
            sessions.append(session);
        }
    }
    
    return sessions;
}

RecordingSession FileRecordingManager::getSession(const QString& sessionId) const
{
    QMutexLocker locker(&sessionsMutex_);
    
    // Check active sessions first
    auto it = activeSessions_.find(sessionId);
    if (it != activeSessions_.end()) {
        return it.value()->info;
    }
    
    // Check completed sessions
    for (const RecordingSession& session : completedSessions_) {
        if (session.sessionId == sessionId) {
            return session;
        }
    }
    
    return RecordingSession(); // Empty session if not found
}

QString FileRecordingManager::generateSessionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString FileRecordingManager::generateFilename(const QString& roomId, RecordingType type)
{
    QDateTime now = QDateTime::currentDateTime();
    QString timestamp = now.toString("yyyyMMdd_hhmmss");
    QString extension = getFileExtension(type);
    
    return QString("%1/%2_%3_%4.%5")
        .arg(recordingDir_)
        .arg(roomId)
        .arg(timestamp)
        .arg(static_cast<int>(type))
        .arg(extension);
}

QString FileRecordingManager::getFileExtension(RecordingType type) const
{
    switch (type) {
        case RecordingType::Messages:
            return "jsonl";
        case RecordingType::DeviceData:
            return "jsonl";
        case RecordingType::AudioVideo:
            return "mp4"; // Future implementation
        default:
            return "dat";
    }
}

bool FileRecordingManager::openSessionFile(ActiveSession* session)
{
    session->file = new QFile(session->info.filename);
    
    if (!session->file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString error = QString("Failed to open recording file: %1").arg(session->file->errorString());
        emit recordingError(session->info.sessionId, error);
        return false;
    }
    
    session->stream = new QTextStream(session->file);
    session->stream->setCodec("UTF-8");
    
    // Write header record
    QJsonObject header;
    header["type"] = "header";
    header["version"] = "1.0";
    header["sessionId"] = session->info.sessionId;
    header["roomId"] = session->info.roomId;
    header["recordingType"] = static_cast<int>(session->info.type);
    header["startTime"] = session->info.startTime.toString(Qt::ISODate);
    header["metadata"] = session->info.metadata;
    
    return writeJsonLine(session, header);
}

void FileRecordingManager::closeSessionFile(ActiveSession* session)
{
    if (session->stream) {
        // Write footer record
        QJsonObject footer;
        footer["type"] = "footer";
        footer["endTime"] = session->info.endTime.toString(Qt::ISODate);
        footer["itemCount"] = session->info.itemCount;
        footer["fileSize"] = session->info.fileSize;
        footer["duration"] = session->info.getDurationMs();
        
        writeJsonLine(session, footer);
        session->stream->flush();
    }
    
    session->cleanup();
}

bool FileRecordingManager::writeJsonLine(ActiveSession* session, const QJsonObject& data)
{
    if (!session->stream) {
        return false;
    }
    
    QJsonDocument doc(data);
    QByteArray line = doc.toJson(QJsonDocument::Compact);
    
    *session->stream << QString::fromUtf8(line) << "\n";
    session->stream->flush();
    
    session->info.itemCount++;
    updateSessionStats(session);
    
    // Check file size limit
    if (session->info.fileSize > maxFileSize_) {
        QString error = QString("Recording file size limit exceeded: %1 bytes").arg(maxFileSize_);
        emit recordingError(session->info.sessionId, error);
        return false;
    }
    
    return true;
}

void FileRecordingManager::updateSessionStats(ActiveSession* session)
{
    if (session->file) {
        session->info.fileSize = session->file->size();
    }
}

// MemoryRecordingManager implementation
MemoryRecordingManager::MemoryRecordingManager(QObject* parent)
    : IRecordingManager(parent)
    , maxItemsPerSession_(10000)
{
}

QString MemoryRecordingManager::startRecording(const QString& roomId, RecordingType type, const QJsonObject& metadata)
{
    QString sessionId = generateSessionId();
    
    MemorySession session;
    session.info = RecordingSession(sessionId, roomId, type);
    session.info.metadata = metadata;
    
    sessions_[sessionId] = session;
    
    qCInfo(logRecording) << "Memory recording started:" << sessionId << "room:" << roomId;
    emit recordingStarted(sessionId);
    
    return sessionId;
}

bool MemoryRecordingManager::stopRecording(const QString& sessionId)
{
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return false;
    }
    
    it.value().info.endTime = QDateTime::currentDateTime();
    
    qCInfo(logRecording) << "Memory recording stopped:" << sessionId;
    emit recordingStopped(sessionId);
    
    return true;
}

bool MemoryRecordingManager::isRecording(const QString& sessionId) const
{
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() && it.value().info.isActive();
}

bool MemoryRecordingManager::recordMessage(const QString& sessionId, const Packet& packet)
{
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end() || !it.value().info.isActive()) {
        return false;
    }
    
    QJsonObject record;
    record["type"] = "message";
    record["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    record["messageType"] = packet.type;
    record["roomId"] = packet.roomId;
    record["senderId"] = packet.senderId;
    record["json"] = packet.json;
    record["seq"] = static_cast<qint64>(packet.seq);
    
    if (!packet.bin.isEmpty()) {
        record["binarySize"] = packet.bin.size();
    }
    
    it.value().data.append(record);
    it.value().info.itemCount++;
    
    // Enforce item limit
    if (it.value().data.size() > maxItemsPerSession_) {
        it.value().data.removeFirst();
    }
    
    return true;
}

bool MemoryRecordingManager::recordDeviceSample(const QString& sessionId, const DeviceSample& sample)
{
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end() || !it.value().info.isActive()) {
        return false;
    }
    
    QJsonObject record;
    record["type"] = "device_sample";
    record["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    record["sample"] = sample.toJson();
    
    it.value().data.append(record);
    it.value().info.itemCount++;
    
    // Enforce item limit
    if (it.value().data.size() > maxItemsPerSession_) {
        it.value().data.removeFirst();
    }
    
    return true;
}

QList<RecordingSession> MemoryRecordingManager::getActiveSessions() const
{
    QList<RecordingSession> active;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().info.isActive()) {
            active.append(it.value().info);
        }
    }
    return active;
}

QList<RecordingSession> MemoryRecordingManager::getSessionsByRoom(const QString& roomId) const
{
    QList<RecordingSession> roomSessions;
    for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
        if (it.value().info.roomId == roomId) {
            roomSessions.append(it.value().info);
        }
    }
    return roomSessions;
}

RecordingSession MemoryRecordingManager::getSession(const QString& sessionId) const
{
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? it.value().info : RecordingSession();
}

bool MemoryRecordingManager::configure(const QJsonObject& config)
{
    if (config.contains("maxItemsPerSession")) {
        maxItemsPerSession_ = config["maxItemsPerSession"].toInt();
    }
    return true;
}

QJsonObject MemoryRecordingManager::getConfiguration() const
{
    QJsonObject config;
    config["maxItemsPerSession"] = maxItemsPerSession_;
    return config;
}

QJsonArray MemoryRecordingManager::getRecordedData(const QString& sessionId) const
{
    auto it = sessions_.find(sessionId);
    return it != sessions_.end() ? it.value().data : QJsonArray();
}

void MemoryRecordingManager::clearSession(const QString& sessionId)
{
    sessions_.remove(sessionId);
}

void MemoryRecordingManager::clearAll()
{
    sessions_.clear();
}

QString MemoryRecordingManager::generateSessionId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}