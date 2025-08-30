#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QSplitter>
#include "clientconn.h"

class ExpertMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit ExpertMain(ClientConn* conn, QWidget *parent = nullptr);

private slots:
    void onJoinWorkOrder();
    void onSendMessage();
    void onRefreshWorkOrders();
    void onWorkOrderSelected();
    void onPkt(Packet p);

private:
    void setupUI();
    void setupWorkOrdersTab();
    void setupCommunicationTab();
    void updateWorkOrdersList();
    
    // UI Components
    QTabWidget *tabWidget;
    
    // Work Orders Tab
    QWidget *workOrdersTab;
    QTableWidget *workOrdersTable;
    QPushButton *btnRefreshWorkOrders;
    QPushButton *btnJoinSelected;
    
    // Communication Tab
    QWidget *commTab;
    QTextEdit *txtChat;
    QLineEdit *edMessage;
    QPushButton *btnSendMessage;
    QLabel *videoLabel;
    QLabel *currentRoomLabel;
    
    // Connection
    ClientConn* conn_;
    QString currentRoom_;
    bool isInRoom_;
};