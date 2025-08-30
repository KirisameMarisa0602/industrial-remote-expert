#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>
#include <QVideoProbe>
#include <QSettings>
#include "clientconn.h"
#include "loginregisterdialog.h"

// Forward declarations
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QVideoFrame;
class QCheckBox;
class QListWidget;
class QDockWidget;
class QSplitter;
class QTreeWidget;
class QTabWidget;

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

private:
    void setupExpertMainUI();
    void createMenuBar();
    void createNavigationPanel();
    void createVideoGrid();
    void createParticipantPanel();
    void createChatPanel();
    void showLoginDialog();
    
    void stopCamera();
    void tryAutoStartCamera();
    void saveAutoStartPreference(bool enabled);
    bool loadAutoStartPreference();

    // Legacy UI components (will be removed/replaced)
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
    
    // Modern Expert UI components
    QDockWidget *navigationDock_;
    QDockWidget *participantDock_;
    QDockWidget *chatDock_;
    QTreeWidget *workOrderList_;
    QListWidget *participantList_;
    QWidget *videoGrid_;
    QTabWidget *chatTabs_;
    QTextEdit *chatDisplay_;
    QLineEdit *chatInput_;
    QPushButton *joinButton_;
    
    // Login/authentication
    QLineEdit *edLoginUser;
    QLineEdit *edLoginPass;
    QPushButton *btnLogin;
    QPushButton *btnRegister;
    QPushButton *btnJoin_;
    
    UserRole currentUserRole_;
    QString authenticatedUsername_;

    ClientConn conn_;

    QCamera *camera_;
    QVideoProbe *probe_;
    QSettings settings_;
    QString currentRoom_;
    bool isConnected_;
    bool isJoinedRoom_;
    bool isAuthenticated_;
    QString sessionToken_;
};

#endif // MAINWINDOW_H
