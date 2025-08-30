#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>      // 包含 QCamera
#include <QVideoProbe>  // 包含 QVideoProbe
#include "clientconn.h" // 假设你的 clientconn.h 在这里

// 前向声明
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QVideoFrame; // 确保声明 QVideoFrame

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

    void onToggleCamera();
    void onVideoFrame(const QVideoFrame &frame); // 接收视频帧的槽函数

private:
    // 这两个函数是内部实现细节，保持 private

    void stopCamera();

    QLineEdit *edHost;
    QLineEdit *edPort;
    QLineEdit *edUser;
    QLineEdit *edRoom;
    QLineEdit *edInput;
    QTextEdit *txtLog;
    QLabel *videoLabel_;
    QPushButton *btnCamera_;

    ClientConn conn_; // 你的网络连接类

    QCamera *camera_;       // 摄像头对象
    QVideoProbe *probe_;    // 视频探头，用于捕获帧
};

#endif // MAINWINDOW_H
