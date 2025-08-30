#pragma once

#include <QtWidgets>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QStackedWidget>
#include <QFrame>
#include <QProgressBar>
#include <QTimer>
#include <QProcess>

#include "../../common/auth/authwidget.h"
#include "../../common/protocol.h"
#include "clientconn.h"

/**
 * 启动器主窗口
 * 显示统一的登录界面，根据用户角色启动相应的客户端程序
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnectToServer();
    void onConnected();
    void onDisconnected();
    void onPacketReceived(Packet pkt);
    void onLoginRequested(const QString& username, const QString& password, const QString& role);
    void onRegisterRequested(const QString& username, const QString& password, const QString& role);
    void onConnectionTimeout();

private:
    void setupUI();
    void setupConnectionPage();
    void setupAuthPage();
    void setupLoadingPage();
    void switchToPage(int pageIndex);
    void connectToServer();
    void launchClientApplication(const QString& role);
    void showMessage(const QString& title, const QString& message, bool isError = false);

    // UI组件
    QStackedWidget *stackedWidget_;
    QWidget *connectionPage_;
    QWidget *authPage_;
    QWidget *loadingPage_;
    
    // 连接页面
    QLineEdit *hostEdit_;
    QLineEdit *portEdit_;
    QPushButton *connectButton_;
    QLabel *connectionStatus_;
    
    // 认证页面
    AuthWidget *authWidget_;
    
    // 加载页面
    QProgressBar *progressBar_;
    QLabel *loadingLabel_;
    
    // 网络连接
    ClientConn conn_;
    QTimer *connectionTimer_;
    
    // 状态
    bool isConnected_;
    QString currentUsername_;
    QString currentRole_;
};