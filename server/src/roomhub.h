#pragma once
// ===============================================
// server/src/roomhub.h
// 最小服务器：监听TCP，维护“房间(roomId) -> 客户端列表”
// 功能：转发同一roomId内的消息（先支持 MSG_JOIN_WORKORDER / MSG_TEXT）
// ===============================================
#include <QtCore>
#include <QtNetwork>
#include <QtSql>
#include "../../common/protocol.h"

struct ClientCtx {
    QTcpSocket* sock = nullptr;
    QString user;       // 用户名，仅用于日志/展示
    QString role;       // 用户角色：factory/expert
    QString roomId;     // 当前加入的房间；空字符串表示未加入任何房间
    QString sessionToken; // 登录会话令牌
    bool authenticated = false; // 是否已认证
};

class RoomHub : public QObject {
    Q_OBJECT
public:
    explicit RoomHub(QObject* parent=nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer server_;
    // 连接索引：socket -> ClientCtx
    QHash<QTcpSocket*, ClientCtx*> clients_;
    // 房间索引：roomId -> sockets（允许多人）
    QMultiHash<QString, QTcpSocket*> rooms_;
    
    // 数据库连接
    QSqlDatabase db_;

    void handlePacket(ClientCtx* c, const Packet& p);
    void joinRoom(ClientCtx* c, const QString& roomId);
    void broadcastToRoom(const QString& roomId,
                         const QByteArray& packet,
                         QTcpSocket* except = nullptr);
    
    // 用户认证相关方法
    bool initDatabase();
    bool registerUser(const QString& username, const QString& password, const QString& role);
    QString loginUser(const QString& username, const QString& password);
    bool validateSessionToken(const QString& token);
    QString generateSessionToken();
    QString getUserRole(const QString& username);
    void handleRegister(ClientCtx* c, const Packet& p);
    void handleLogin(ClientCtx* c, const Packet& p);
};
