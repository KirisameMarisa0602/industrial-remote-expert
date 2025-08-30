#include "protocol.h"
#include <QDateTime>
#include <QDataStream>
#include <QMutexLocker>

// Define logging categories
Q_LOGGING_CATEGORY(logProtocol, "protocol")
Q_LOGGING_CATEGORY(logNetwork, "network") 
Q_LOGGING_CATEGORY(logRoomHub, "roomhub")
Q_LOGGING_CATEGORY(logDevice, "device")
Q_LOGGING_CATEGORY(logRecording, "recording")

// Global sequence counter for packet ordering
static QAtomicInt g_sequenceCounter(0);

QByteArray buildPacket(quint16 type,
                       const QJsonObject& json,
                       const QByteArray& bin,
                       const QString& roomId,
                       const QString& senderId,
                       quint16 flags,
                       quint32 seq)
{
    // Prepare JSON payload
    QByteArray jsonBytes = toJsonBytes(json);
    if (jsonBytes.size() > static_cast<int>(MAX_JSON_SIZE)) {
        qCWarning(logProtocol) << "JSON payload too large:" << jsonBytes.size() << "bytes";
        return QByteArray();
    }
    
    // Calculate total frame size
    quint32 totalSize = sizeof(FrameHeader) + jsonBytes.size() + bin.size();
    if (totalSize > MAX_FRAME_SIZE) {
        qCWarning(logProtocol) << "Frame too large:" << totalSize << "bytes";
        return QByteArray();
    }
    
    // Create frame header
    FrameHeader header = {};
    header.magic = qToBigEndian(PROTOCOL_MAGIC);
    header.version = qToBigEndian(PROTOCOL_VERSION);
    header.msgType = qToBigEndian(type);
    header.flags = qToBigEndian(flags);
    header.reserved = 0;
    header.length = qToBigEndian(totalSize);
    header.timestampMs = qToBigEndian(static_cast<quint64>(QDateTime::currentMSecsSinceEpoch()));
    header.seq = qToBigEndian(seq == 0 ? g_sequenceCounter.fetchAndAddOrdered(1) : seq);
    header.jsonSize = qToBigEndian(static_cast<quint32>(jsonBytes.size()));
    
    // Copy room ID and sender ID (truncate if necessary)
    QByteArray roomIdBytes = roomId.toLatin1();
    int roomIdLen = qMin(roomIdBytes.size(), static_cast<int>(ROOM_ID_SIZE - 1));
    memcpy(header.roomId, roomIdBytes.constData(), roomIdLen);
    header.roomId[roomIdLen] = '\0';
    
    QByteArray senderIdBytes = senderId.toLatin1();
    int senderIdLen = qMin(senderIdBytes.size(), static_cast<int>(SENDER_ID_SIZE - 1));
    memcpy(header.senderId, senderIdBytes.constData(), senderIdLen);
    header.senderId[senderIdLen] = '\0';
    
    // Build final packet
    QByteArray packet;
    packet.reserve(totalSize);
    
    // Append header
    packet.append(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Append JSON payload
    if (!jsonBytes.isEmpty()) {
        packet.append(jsonBytes);
    }
    
    // Append binary payload
    if (!bin.isEmpty()) {
        packet.append(bin);
    }
    
    qCDebug(logProtocol) << "Built packet: type=" << type 
                        << "size=" << totalSize
                        << "room=" << roomId
                        << "sender=" << senderId;
    
    return packet;
}

bool drainPackets(QByteArray& buffer, QVector<Packet>& out, QString* error)
{
    bool producedPackets = false;
    
    while (buffer.size() >= static_cast<int>(sizeof(FrameHeader))) {
        // Parse header (without consuming buffer yet)
        FrameHeader header;
        memcpy(&header, buffer.constData(), sizeof(header));
        
        // Convert from network byte order
        header.magic = qFromBigEndian(header.magic);
        header.version = qFromBigEndian(header.version);
        header.msgType = qFromBigEndian(header.msgType);
        header.flags = qFromBigEndian(header.flags);
        header.length = qFromBigEndian(header.length);
        header.timestampMs = qFromBigEndian(header.timestampMs);
        header.seq = qFromBigEndian(header.seq);
        header.jsonSize = qFromBigEndian(header.jsonSize);
        
        // Validate header
        QString validationError;
        if (!validateFrameHeader(header, &validationError)) {
            if (error) *error = validationError;
            qCWarning(logProtocol) << "Invalid frame header:" << validationError;
            buffer.clear(); // Discard buffer on validation error
            return producedPackets;
        }
        
        // Check if we have the complete frame
        if (buffer.size() < static_cast<int>(header.length)) {
            // Incomplete frame, wait for more data
            break;
        }
        
        // Extract complete frame
        QByteArray frameData = buffer.left(header.length);
        buffer.remove(0, header.length);
        
        // Parse JSON payload
        QByteArray jsonBytes;
        if (header.jsonSize > 0) {
            if (sizeof(FrameHeader) + header.jsonSize > header.length) {
                if (error) *error = "JSON size exceeds frame size";
                continue; // Skip invalid frame
            }
            jsonBytes = frameData.mid(sizeof(FrameHeader), header.jsonSize);
        }
        
        // Parse binary payload
        QByteArray binData;
        quint32 binSize = header.length - sizeof(FrameHeader) - header.jsonSize;
        if (binSize > 0) {
            binData = frameData.right(binSize);
        }
        
        // Create packet
        Packet packet(header);
        if (!jsonBytes.isEmpty()) {
            packet.json = fromJsonBytes(jsonBytes);
            if (packet.json.isEmpty() && !jsonBytes.isEmpty()) {
                qCWarning(logProtocol) << "Failed to parse JSON payload";
                continue; // Skip packet with invalid JSON
            }
        }
        packet.bin = binData;
        
        out.push_back(std::move(packet));
        producedPackets = true;
        
        qCDebug(logProtocol) << "Parsed packet: type=" << packet.type
                            << "room=" << packet.roomId
                            << "sender=" << packet.senderId;
    }
    
    return producedPackets;
}

bool validateFrameHeader(const FrameHeader& header, QString* error)
{
    // Check magic number
    if (header.magic != PROTOCOL_MAGIC) {
        if (error) *error = QString("Invalid magic number: 0x%1").arg(header.magic, 8, 16, QChar('0'));
        return false;
    }
    
    // Check version
    if (header.version != PROTOCOL_VERSION) {
        if (error) *error = QString("Unsupported protocol version: %1").arg(header.version);
        return false;
    }
    
    // Check frame size
    if (header.length < sizeof(FrameHeader) || header.length > MAX_FRAME_SIZE) {
        if (error) *error = QString("Invalid frame size: %1").arg(header.length);
        return false;
    }
    
    // Check JSON size
    if (header.jsonSize > MAX_JSON_SIZE) {
        if (error) *error = QString("JSON payload too large: %1").arg(header.jsonSize);
        return false;
    }
    
    // Check that JSON size doesn't exceed frame size
    if (sizeof(FrameHeader) + header.jsonSize > header.length) {
        if (error) *error = "JSON size exceeds frame size";
        return false;
    }
    
    return true;
}

QString errorCodeToString(ErrorCode code)
{
    switch (code) {
        case ERR_NONE: return "No error";
        case ERR_PROTOCOL_VERSION: return "Unsupported protocol version";
        case ERR_INVALID_FRAME: return "Malformed frame";
        case ERR_FRAME_TOO_LARGE: return "Frame exceeds maximum size";
        case ERR_JSON_PARSE: return "JSON parsing failed";
        case ERR_UNAUTHORIZED: return "Authentication required";
        case ERR_ROOM_NOT_FOUND: return "Room doesn't exist";
        case ERR_NOT_IN_ROOM: return "User not in any room";
        case ERR_RATE_LIMITED: return "Too many requests";
        case ERR_INTERNAL: return "Internal server error";
        default: return QString("Unknown error: %1").arg(static_cast<int>(code));
    }
}

// RateLimiter implementation
RateLimiter::RateLimiter(int maxRequests, int windowMs)
    : maxRequests_(maxRequests), windowMs_(windowMs)
{
}

bool RateLimiter::checkRateLimit(const QString& clientId)
{
    QMutexLocker locker(&mutex_);
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 windowStart = now - windowMs_;
    
    ClientStats& stats = clients_[clientId];
    
    // Remove old timestamps outside the window
    while (!stats.timestamps.isEmpty() && stats.timestamps.head() < windowStart) {
        stats.timestamps.dequeue();
        if (stats.requests > 0) {
            stats.requests--;
        }
    }
    
    // Check if we're within rate limits
    if (stats.requests >= maxRequests_) {
        return false; // Rate limited
    }
    
    // Add current request
    stats.timestamps.enqueue(now);
    stats.requests++;
    
    return true; // Request allowed
}

void RateLimiter::reset()
{
    QMutexLocker locker(&mutex_);
    clients_.clear();
}