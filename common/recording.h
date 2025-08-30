#pragma once
// ===============================================
// common/recording.h
// Recording manager interfaces for MVP foundation
// Provides: IRecordingManager, FileRecordingManager for messages and device data
// ===============================================

#include <QtCore>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "protocol.h"
#include "device.h"

// Recording types
enum class RecordingType {
    Messages,
    DeviceData,
    AudioVideo  // Future implementation
};

// Recording session information
struct RecordingSession {
    QString sessionId;
    QString roomId;
    RecordingType type;
    QString filename;
    QDateTime startTime;
    QDateTime endTime;
    qint64 fileSize = 0;
    int itemCount = 0;
    QJsonObject metadata;
    
    RecordingSession() = default;
    
    RecordingSession(const QString& id, const QString& room, RecordingType t)
        : sessionId(id), roomId(room), type(t), startTime(QDateTime::currentDateTime())
    {}
    
    bool isActive() const {
        return endTime.isNull();
    }
    
    qint64 getDurationMs() const {
        QDateTime end = isActive() ? QDateTime::currentDateTime() : endTime;
        return startTime.msecsTo(end);
    }
};

// Base interface for recording managers
class IRecordingManager : public QObject
{
    Q_OBJECT
    
public:
    explicit IRecordingManager(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IRecordingManager() = default;
    
    // Session management
    virtual QString startRecording(const QString& roomId, RecordingType type, const QJsonObject& metadata = QJsonObject()) = 0;
    virtual bool stopRecording(const QString& sessionId) = 0;
    virtual bool isRecording(const QString& sessionId) const = 0;
    
    // Data recording
    virtual bool recordMessage(const QString& sessionId, const Packet& packet) = 0;
    virtual bool recordDeviceSample(const QString& sessionId, const DeviceSample& sample) = 0;
    
    // Session query
    virtual QList<RecordingSession> getActiveSessions() const = 0;
    virtual QList<RecordingSession> getSessionsByRoom(const QString& roomId) const = 0;
    virtual RecordingSession getSession(const QString& sessionId) const = 0;
    
    // Configuration
    virtual bool configure(const QJsonObject& config) = 0;
    virtual QJsonObject getConfiguration() const = 0;

signals:
    void recordingStarted(const QString& sessionId);
    void recordingStopped(const QString& sessionId);
    void recordingError(const QString& sessionId, const QString& error);
};

// File-based recording manager (JSONL format)
class FileRecordingManager : public IRecordingManager
{
    Q_OBJECT
    
public:
    explicit FileRecordingManager(QObject* parent = nullptr);
    ~FileRecordingManager() override;
    
    // IRecordingManager interface
    QString startRecording(const QString& roomId, RecordingType type, const QJsonObject& metadata = QJsonObject()) override;
    bool stopRecording(const QString& sessionId) override;
    bool isRecording(const QString& sessionId) const override;
    bool recordMessage(const QString& sessionId, const Packet& packet) override;
    bool recordDeviceSample(const QString& sessionId, const DeviceSample& sample) override;
    QList<RecordingSession> getActiveSessions() const override;
    QList<RecordingSession> getSessionsByRoom(const QString& roomId) const override;
    RecordingSession getSession(const QString& sessionId) const override;
    bool configure(const QJsonObject& config) override;
    QJsonObject getConfiguration() const override;
    
    // File manager specific
    void setRecordingDirectory(const QString& path);
    QString getRecordingDirectory() const { return recordingDir_; }
    void setMaxFileSize(qint64 bytes) { maxFileSize_ = bytes; }
    void setCompressionEnabled(bool enabled) { compressionEnabled_ = enabled; }

private:
    struct ActiveSession {
        RecordingSession info;
        QFile* file;
        QTextStream* stream;
        
        ActiveSession() : file(nullptr), stream(nullptr) {}
        ~ActiveSession() {
            cleanup();
        }
        
        void cleanup() {
            if (stream) {
                delete stream;
                stream = nullptr;
            }
            if (file) {
                file->close();
                delete file;
                file = nullptr;
            }
        }
    };
    
    QString recordingDir_;
    qint64 maxFileSize_;
    bool compressionEnabled_;
    QHash<QString, ActiveSession*> activeSessions_;
    QList<RecordingSession> completedSessions_;
    mutable QMutex sessionsMutex_;
    
    QString generateSessionId();
    QString generateFilename(const QString& roomId, RecordingType type);
    QString getFileExtension(RecordingType type) const;
    bool openSessionFile(ActiveSession* session);
    void closeSessionFile(ActiveSession* session);
    bool writeJsonLine(ActiveSession* session, const QJsonObject& data);
    void updateSessionStats(ActiveSession* session);
};

// Simple in-memory recording manager for testing
class MemoryRecordingManager : public IRecordingManager
{
    Q_OBJECT
    
public:
    explicit MemoryRecordingManager(QObject* parent = nullptr);
    ~MemoryRecordingManager() override = default;
    
    // IRecordingManager interface
    QString startRecording(const QString& roomId, RecordingType type, const QJsonObject& metadata = QJsonObject()) override;
    bool stopRecording(const QString& sessionId) override;
    bool isRecording(const QString& sessionId) const override;
    bool recordMessage(const QString& sessionId, const Packet& packet) override;
    bool recordDeviceSample(const QString& sessionId, const DeviceSample& sample) override;
    QList<RecordingSession> getActiveSessions() const override;
    QList<RecordingSession> getSessionsByRoom(const QString& roomId) const override;
    RecordingSession getSession(const QString& sessionId) const override;
    bool configure(const QJsonObject& config) override;
    QJsonObject getConfiguration() const override;
    
    // Memory manager specific
    QJsonArray getRecordedData(const QString& sessionId) const;
    void clearSession(const QString& sessionId);
    void clearAll();

private:
    struct MemorySession {
        RecordingSession info;
        QJsonArray data;
    };
    
    QHash<QString, MemorySession> sessions_;
    int maxItemsPerSession_;
    
    QString generateSessionId();
};