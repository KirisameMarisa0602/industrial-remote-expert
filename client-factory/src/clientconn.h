#pragma once
// ===============================================
// 客户端连接封装（两端共用一份拷贝）
// 提供：connectTo(host,port)、sendPacket(type,json,bin)、信号 packetArrived
// ===============================================
#include <QtCore>
#include <QtNetwork>
#include "../../common/protocol.h"



class ClientConn : public QObject {
    Q_OBJECT
public:
    explicit ClientConn(QObject* parent=nullptr); // 构造：初始化QTcpSocket并连接信号
    void connectTo(const QString& host, quint16 port); // 主动发起到服务器的TCP连接
    void connect2(const QString& host, quint16 port); // Alternative connect method name
    void send(quint16 type, const QJsonObject& json, const QByteArray& bin = QByteArray()); // 发送一个协议包
    bool isConnected() const; // 检查是否已连接到服务器
    void setSessionToken(const QString& token); // Set session token for authenticated requests
    QString sessionToken() const; // Get current session token
signals: // 对外信号（供UI层连接）
    void connected();
    void disconnected();
    void packetArrived(Packet pkt);
    void sigConnected();    // Alternative signal name
    void sigDisconnected(); // Alternative signal name
    void sigPkt(Packet pkt); // Alternative signal name
private slots: // 内部槽函数（socket事件）
    void onReadyRead();
    void onConnected();
    void onDisconnected();
private:
    QTcpSocket sock_;
    QByteArray buf_;
    QString sessionToken_;
};
