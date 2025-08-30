#pragma once

#include <QtWidgets>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QMenuBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QListWidget>
#include <QGroupBox>
#include <QTabWidget>
#include <QProgressBar>

#include "../../common/sidebar/sidebarwidget.h"
#include "../../common/dashboard/dashboardwidget.h"
#include "../../common/protocol.h"
#include "clientconn.h"

/**
 * 专家客户端主窗口 - 现代化设计
 * 包含侧边栏导航、仪表盘、视频通信、工单管理等功能
 */
class ModernMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ModernMainWindow(QWidget *parent = nullptr);

private slots:
    void onNavigationChanged(const QString &page);
    void onConnected();
    void onDisconnected();
    void onPacketReceived(Packet pkt);
    void onJoinRoom();
    void onSendMessage();
    void onToggleCamera();
    void onToggleAudio();

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupMainContent();
    void setupDashboardPage();
    void setupCommunicationPage();
    void setupVideoWidget();
    void setupChatWidget();
    void setupWorkOrderPage();
    void setupSettingsPage();
    
    void switchToPage(const QString &pageName);
    void updateConnectionStatus(bool connected);
    void loadTheme();
    
    // UI组件
    SidebarWidget *sidebar_;
    QStackedWidget *stackedWidget_;
    QStatusBar *statusBar_;
    
    // 页面组件
    QWidget *dashboardPage_;
    QWidget *communicationPage_;
    QWidget *workOrderPage_;
    QWidget *settingsPage_;
    
    // 仪表盘组件
    DashboardWidget *dashboard_;
    
    // 通信页面组件
    QSplitter *communicationSplitter_;
    QWidget *videoWidget_;
    QWidget *chatWidget_;
    QLabel *localVideoLabel_;
    QLabel *remoteVideoLabel_;
    QTextEdit *chatDisplay_;
    QLineEdit *messageInput_;
    QPushButton *sendButton_;
    QPushButton *cameraButton_;
    QPushButton *audioButton_;
    
    // 工单页面组件
    QTableWidget *workOrderTable_;
    QPushButton *joinRoomButton_;
    QLineEdit *roomCodeInput_;
    
    // 状态标签
    QLabel *connectionStatusLabel_;
    QLabel *roomStatusLabel_;
    QLabel *participantCountLabel_;
    
    // 网络连接
    ClientConn conn_;
    
    // 状态信息
    bool isConnected_;
    bool isInRoom_;
    bool isCameraOn_;
    bool isAudioOn_;
    QString currentRoom_;
    QString username_;
};