#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>      // 包含 QCamera
#include <QVideoProbe>  // 包含 QVideoProbe
#include <QSettings>    // 包含 QSettings for auto-start preference
#include "clientconn.h" // 假设你的 clientconn.h 在这里

// 前向声明
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QVideoFrame; // 确保声明 QVideoFrame
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void startCamera();

private slots:
    void onConnect();
    void onJoin();
    void onSendText();
    void onPkt(Packet p); // 假设 Packet 类型已定义
    void onConnected();   // 处理连接建立
    void onDisconnected(); // 处理连接断开
    
    void onLogin();       // 处理登录
    void onRegister();    // 处理注册

    void onToggleCamera();
    void onVideoFrame(const QVideoFrame &frame); // 接收视频帧的槽函数
    void onAutoStartToggled(bool checked); // 处理自动启动选项切换

private:
    // 这两个函数是内部实现细节，保持 private

    void stopCamera();
    void tryAutoStartCamera(); // 尝试自动启动摄像头
    void saveAutoStartPreference(bool enabled); // 保存自动启动偏好
    bool loadAutoStartPreference(); // 加载自动启动偏好

    QLineEdit *edHost;
    QLineEdit *edPort;
    QLineEdit *edUser;
    QLineEdit *edRoom;
    QLineEdit *edInput;
    QTextEdit *txtLog;
    QLabel *videoLabel_;        // 本地视频预览
    QLabel *remoteLabel_;       // 远端视频显示
    QPushButton *btnCamera_;
    QCheckBox *chkAutoStart_; // 自动启动摄像头复选框
    
    // 登录/注册UI
    QLineEdit *edLoginUser;     // 登录用户名
    QLineEdit *edLoginPass;     // 登录密码
    QPushButton *btnLogin;      // 登录按钮
    QPushButton *btnRegister;   // 注册按钮
    QPushButton *btnJoin_;      // 加入房间按钮

    ClientConn conn_; // 你的网络连接类

    QCamera *camera_;       // 摄像头对象
    QVideoProbe *probe_;    // 视频探头，用于捕获帧
    QSettings settings_;    // 设置存储
    QString currentRoom_;   // 当前加入的房间
    bool isConnected_;      // 连接状态
    bool isJoinedRoom_;     // 是否已加入房间
    bool isAuthenticated_;  // 是否已认证
    QString sessionToken_;  // 会话令牌
};

#endif // MAINWINDOW_H
