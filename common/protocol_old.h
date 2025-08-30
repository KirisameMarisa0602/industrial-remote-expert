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

// 统一的消息类型定义 —— 以后扩展只“新增值”，不要修改已存在的值
enum MsgType : quint16 {
    MSG_REGISTER         = 1,   // 注册（示例骨架暂未实现）
    MSG_LOGIN            = 2,   // 登录（示例骨架暂未实现）
    MSG_CREATE_WORKORDER = 3,   // 创建工单（示例骨架暂未实现）
    MSG_JOIN_WORKORDER   = 4,   // 加入工单（设置roomId + username）

    MSG_TEXT             = 10,  // 文本聊天（先跑通端到端）
    // 设备/音视频后续添加：
    MSG_DEVICE_DATA      = 20,  // 设备数据（纯JSON即可）
    MSG_VIDEO_FRAME      = 30,  // bin: JPEG
    MSG_AUDIO_FRAME      = 40,  // bin: PCM S16LE
    MSG_CONTROL          = 50,  // 控制指令（可选加分）

    MSG_SERVER_EVENT     = 90   // 服务器提示/错误/房间事件等
};

// 一条完整消息
struct Packet {
    quint16 type = 0;
    QJsonObject json;
    QByteArray bin; // 可为空
};

// 工具：JSON编解码（使用紧凑格式，节约带宽）
inline QByteArray toJsonBytes(const QJsonObject& j) {
    return QJsonDocument(j).toJson(QJsonDocument::Compact);
}
inline QJsonObject fromJsonBytes(const QByteArray& b) {
    auto doc = QJsonDocument::fromJson(b);
    return doc.isObject() ? doc.object() : QJsonObject{};
}

// 打包（发送前调用）
QByteArray buildPacket(quint16 type,
                       const QJsonObject& json,
                       const QByteArray& bin = QByteArray());

// 拆包（在QTcpSocket::readyRead里，把readAll追加到buffer，然后调用drainPackets）
// - 解决粘包/半包；只要buffer里有完整包就会解析出来放进out
// - 返回是否至少解析出1个完整包
bool drainPackets(QByteArray& buffer, QVector<Packet>& out);
