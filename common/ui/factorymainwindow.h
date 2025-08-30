#pragma once

#include <QMainWindow>
#include <QCamera>
#include <QVideoProbe>
#include <QSettings>
#include "clientconn.h"

// Forward declarations
class QLineEdit;
class QPushButton;
class QLabel;
class QTextEdit;
class QVideoFrame;
class QCheckBox;

class FactoryMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FactoryMainWindow(const QString& sessionToken, const QString& username, QWidget *parent = nullptr);
    void startCamera();

private slots:
    void onConnect();
    void onJoin();
    void onSendText();
    void onPkt(Packet p);
    void onConnected();
    void onDisconnected();
    void onToggleCamera();
    void onVideoFrame(const QVideoFrame &frame);
    void onAutoStartToggled(bool checked);

private:
    void setupUI();
    void stopCamera();
    void tryAutoStartCamera();
    void saveAutoStartPreference(bool enabled);
    bool loadAutoStartPreference();

    QLineEdit *edHost;
    QLineEdit *edPort;
    QLineEdit *edUser;
    QLineEdit *edRoom;
    QLineEdit *edInput;
    QTextEdit *txtLog;
    QLabel *videoLabel_;
    QLabel *remoteLabel_;
    QPushButton *btnCamera_;
    QCheckBox *chkAutoStart_;

    ClientConn conn_;
    QCamera *camera_;
    QVideoProbe *probe_;
    QSettings settings_;
    QString currentRoom_;
    QString sessionToken_;
    QString username_;
    bool isConnected_;
    bool isJoinedRoom_;
};