#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTabWidget>
#include <QCamera>
#include <QVideoProbe>
#include <QCheckBox>
#include "clientconn.h"

class FactoryMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit FactoryMain(ClientConn* conn, QWidget *parent = nullptr);

private slots:
    void onCreateWorkOrder();
    void onJoinWorkOrder();
    void onSendMessage();
    void onToggleCamera();
    void onVideoFrame(const QVideoFrame &frame);
    void onPkt(Packet p);

private:
    void setupUI();
    void setupWorkOrderTab();
    void setupCommunicationTab();
    void startCamera();
    void stopCamera();
    
    // UI Components
    QTabWidget *tabWidget;
    
    // Work Order Creation Tab
    QWidget *workOrderTab;
    QLineEdit *edWorkOrderTitle;
    QTextEdit *edWorkOrderDescription;
    QPushButton *btnCreateWorkOrder;
    
    // Communication Tab
    QWidget *commTab;
    QTextEdit *txtChat;
    QLineEdit *edMessage;
    QPushButton *btnSendMessage;
    QLabel *videoLabel;
    QPushButton *btnCamera;
    QCheckBox *chkAutoStart;
    QLabel *currentRoomLabel;
    QLineEdit *edRoomId;
    QPushButton *btnJoinRoom;
    
    // Connection and Camera
    ClientConn* conn_;
    QCamera *camera_;
    QVideoProbe *probe_;
    QString currentRoom_;
    bool isInRoom_;
    bool cameraActive_;
};