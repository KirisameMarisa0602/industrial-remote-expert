#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>
#include <QVideoProbe>
#include <QSettings>
#include "clientconn.h"
#include "loginregisterdialog.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

// Forward declarations
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QVideoFrame;
class QCheckBox;
class QDockWidget;
class QSplitter;
class QTabWidget;
class QTimer;

using namespace QtCharts;

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
    void onPkt(Packet p);
    void onConnected();
    void onDisconnected();
    
    // Authentication (using new dialog)
    void onLoginSuccess(const QString& username, const QString& password, UserRole role);
    void onRegisterSuccess(const QString& username, const QString& password, UserRole role);
    
    void onToggleCamera();
    void onVideoFrame(const QVideoFrame &frame);
    void onAutoStartToggled(bool checked);
    
    // Factory-specific slots
    void onCreateWorkOrder();
    void updateDeviceData();

private:
    void setupFactoryMainUI();
    void createMenuBar();
    void createQuickActionsPanel();
    void createDeviceDataDashboard();
    void createCameraPreviewPanel();
    void createChatPanel();
    void showLoginDialog();
    void initializeDeviceSimulation();
    
    void stopCamera();
    void tryAutoStartCamera();
    void saveAutoStartPreference(bool enabled);
    bool loadAutoStartPreference();

    // Legacy UI components (compatibility)
    QLineEdit *edHost;
    QLineEdit *edPort;
    QLineEdit *edUser;
    QLineEdit *edRoom;
    QLineEdit *edInput;
    QTextEdit *txtLog;
    
    // Video components
    QLabel *videoLabel_;
    QLabel *remoteLabel_;
    QPushButton *btnCamera_;
    QCheckBox *chkAutoStart_;
    
    // Factory-specific UI components
    QDockWidget *quickActionsDock_;
    QDockWidget *deviceDataDock_;
    QDockWidget *cameraPreviewDock_;
    QDockWidget *chatDock_;
    
    QPushButton *createWorkOrderBtn_;
    QPushButton *joinWorkOrderBtn_;
    QPushButton *recordingToggleBtn_;
    
    // Device data visualization
    QTabWidget *dashboardTabs_;
    QChartView *pressureChart_;
    QChartView *temperatureChart_;
    QLineSeries *pressureSeries_;
    QLineSeries *temperatureSeries_;
    QTimer *deviceSimulationTimer_;
    
    // Chat components
    QTabWidget *chatTabs_;
    QTextEdit *chatDisplay_;
    QLineEdit *chatInput_;
    
    // Authentication
    UserRole currentUserRole_;
    QString authenticatedUsername_;
    
    ClientConn conn_;

    QCamera *camera_;       // 摄像头对象
    QVideoProbe *probe_;    // 视频探头，用于捕获帧
    QSettings settings_;    // 设置存储
    QString currentRoom_;   // 当前加入的房间
    bool isConnected_;      // 连接状态
    bool isJoinedRoom_;     // 是否已加入房间
};

#endif // MAINWINDOW_H
