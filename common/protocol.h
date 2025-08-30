#pragma once
// ===============================================
// common/protocol.h
// Enhanced protocol with binary frame header + JSON payload
// Structure: [FrameHeader][jsonPayload][binaryPayload]
// FrameHeader: magic('REXP') + version + msgType + flags + length + roomId + senderId + timestampMs + seq
// - Provides versioning, extensibility, frame validation, and routing information
// - Maximum frame size enforced for security and memory management
// ===============================================

#include <QtCore>
#include <QtNetwork>
#include <QLoggingCategory>

// Logging categories for structured logging
Q_DECLARE_LOGGING_CATEGORY(logProtocol)
Q_DECLARE_LOGGING_CATEGORY(logNetwork)
Q_DECLARE_LOGGING_CATEGORY(logRoomHub)
Q_DECLARE_LOGGING_CATEGORY(logDevice)
Q_DECLARE_LOGGING_CATEGORY(logRecording)

// Protocol constants
static const quint32 PROTOCOL_MAGIC = 0x52455850; // 'REXP' in big-endian
static const quint16 PROTOCOL_VERSION = 1;
static const quint32 MAX_FRAME_SIZE = 16 * 1024 * 1024; // 16MB max frame size
static const quint32 MAX_JSON_SIZE = 1 * 1024 * 1024;   // 1MB max JSON payload
static const quint32 ROOM_ID_SIZE = 16;
static const quint32 SENDER_ID_SIZE = 16;

// Frame flags
enum FrameFlags : quint16 {
    FLAG_NONE           = 0x0000,
    FLAG_COMPRESSED     = 0x0001,  // Payload is compressed
    FLAG_ENCRYPTED      = 0x0002,  // Payload is encrypted (future)
    FLAG_FRAGMENTED     = 0x0004,  // Multi-part message (future)
    FLAG_ACK_REQUIRED   = 0x0008,  // Requires acknowledgment
    FLAG_PRIORITY       = 0x0010   // High priority message
};

// Enhanced message types with proper categorization and backward compatibility
enum MsgType : quint16 {
    // Authentication and session management (1-19)
    MSG_REGISTER         = 1,   // User registration
    MSG_LOGIN            = 2,   // User login
    MSG_LOGOUT           = 3,   // User logout
    MSG_CREATE_WORKORDER = 4,   // Create work order
    MSG_JOIN_WORKORDER   = 4,   // Join work order (room) - KEEPING OLD VALUE FOR COMPATIBILITY
    MSG_LEAVE_WORKORDER  = 6,   // Leave work order (room)

    // Communication (10-19) - keeping old numbers for compatibility
    MSG_TEXT             = 10,  // Text message - KEEPING OLD VALUE FOR COMPATIBILITY
    
    // Device and control (20-39)  
    MSG_DEVICE_DATA      = 20,  // Device sensor data - same as before
    MSG_AUDIO_FRAME      = 30,  // Audio data (PCM, compressed) - KEEPING OLD VALUE
    MSG_VIDEO_FRAME      = 40,  // Video data (JPEG, H.264) - KEEPING OLD VALUE  
    MSG_CONTROL_CMD      = 50,  // Device control command - KEEPING OLD VALUE

    // Protocol management (60-79)
    MSG_HEARTBEAT        = 60,  // Keep-alive heartbeat
    MSG_ACK              = 61,  // Acknowledgment
    MSG_NACK             = 62,  // Negative acknowledgment
    MSG_ERROR            = 63,  // Error notification

    // Server events and room management (80-99)
    MSG_SERVER_EVENT     = 90,  // Server notifications - KEEPING OLD VALUE FOR COMPATIBILITY
    MSG_ROOM_MEMBER_JOIN = 81,  // Member joined room
    MSG_ROOM_MEMBER_LEAVE= 82,  // Member left room
    MSG_ROOM_STATE       = 83,  // Room state update
    
    // Device status
    MSG_DEVICE_STATUS    = 42   // Device status update
};

// Error codes for consistent error handling
enum ErrorCode : quint32 {
    ERR_NONE             = 0,
    ERR_PROTOCOL_VERSION = 1,   // Unsupported protocol version
    ERR_INVALID_FRAME    = 2,   // Malformed frame
    ERR_FRAME_TOO_LARGE  = 3,   // Frame exceeds maximum size
    ERR_JSON_PARSE       = 4,   // JSON parsing failed
    ERR_UNAUTHORIZED     = 5,   // Authentication required
    ERR_ROOM_NOT_FOUND   = 6,   // Room doesn't exist
    ERR_NOT_IN_ROOM      = 7,   // User not in any room
    ERR_RATE_LIMITED     = 8,   // Too many requests
    ERR_INTERNAL         = 9    // Internal server error
};

// Binary frame header structure (64 bytes total)
struct FrameHeader {
    quint32 magic;          // Protocol magic 'REXP' (4 bytes)
    quint16 version;        // Protocol version (2 bytes)
    quint16 msgType;        // Message type (enum MsgType) (2 bytes)
    quint16 flags;          // Frame flags (enum FrameFlags) (2 bytes)
    quint16 reserved;       // Reserved for future use (2 bytes)
    quint32 length;         // Total frame length (header + JSON + binary) (4 bytes)
    char roomId[ROOM_ID_SIZE];     // Room identifier (16 bytes, null-terminated)
    char senderId[SENDER_ID_SIZE]; // Sender identifier (16 bytes, null-terminated)
    quint64 timestampMs;    // Timestamp in milliseconds since epoch (8 bytes)
    quint32 seq;            // Sequence number (4 bytes)
    quint32 jsonSize;       // JSON payload size (4 bytes)
    // Total: 4+2+2+2+2+4+16+16+8+4+4 = 64 bytes
    // Followed by: [jsonPayload][binaryPayload]
} __attribute__((packed));

static_assert(sizeof(FrameHeader) == 64, "FrameHeader must be 64 bytes");

// Enhanced packet structure with routing and metadata
struct Packet {
    // Header information
    quint16 type = 0;
    quint16 flags = FLAG_NONE;
    QString roomId;
    QString senderId;
    quint64 timestampMs = 0;
    quint32 seq = 0;
    
    // Payload
    QJsonObject json;
    QByteArray bin;
    
    // Default constructor
    Packet() = default;
    
    // Constructor from header
    explicit Packet(const FrameHeader& header) 
        : type(header.msgType)
        , flags(header.flags)
        , roomId(QString::fromLatin1(header.roomId, strnlen(header.roomId, ROOM_ID_SIZE)))
        , senderId(QString::fromLatin1(header.senderId, strnlen(header.senderId, SENDER_ID_SIZE)))
        , timestampMs(header.timestampMs)
        , seq(header.seq)
    {}
};

// Utility functions for JSON encoding/decoding (compact format for bandwidth efficiency)
inline QByteArray toJsonBytes(const QJsonObject& j) {
    return QJsonDocument(j).toJson(QJsonDocument::Compact);
}

inline QJsonObject fromJsonBytes(const QByteArray& b) {
    auto doc = QJsonDocument::fromJson(b);
    return doc.isObject() ? doc.object() : QJsonObject{};
}

// Enhanced packet building with new frame header format
QByteArray buildPacket(quint16 type,
                       const QJsonObject& json,
                       const QByteArray& bin = QByteArray(),
                       const QString& roomId = QString(),
                       const QString& senderId = QString(),
                       quint16 flags = FLAG_NONE,
                       quint32 seq = 0);

// Enhanced packet parsing with frame validation and error handling
bool drainPackets(QByteArray& buffer, QVector<Packet>& out, QString* error = nullptr);

// Helper functions for protocol validation
bool validateFrameHeader(const FrameHeader& header, QString* error = nullptr);
QString errorCodeToString(ErrorCode code);

// Rate limiting helper
class RateLimiter {
public:
    RateLimiter(int maxRequests = 100, int windowMs = 60000); // 100 requests per minute by default
    bool checkRateLimit(const QString& clientId);
    void reset();
    
private:
    struct ClientStats {
        QQueue<qint64> timestamps;
        int requests = 0;
    };
    
    QHash<QString, ClientStats> clients_;
    int maxRequests_;
    int windowMs_;
    QMutex mutex_;
};