#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QGroupBox>
#include "clientconn.h"

class ExpertMain;
class FactoryMain;

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

private slots:
    void onConnect();
    void onLogin();
    void onRegister();
    void onPkt(Packet p);
    void onConnected();
    void onDisconnected();
    void onRoleChanged();

private:
    void setupUI();
    void updateFormState();
    void showMainWindow(const QString& role);
    
    // UI Components
    QLineEdit *edHost;
    QLineEdit *edPort;
    QPushButton *btnConnect;
    
    QLineEdit *edLoginUser;
    QLineEdit *edLoginPass;
    QPushButton *btnLogin;
    QPushButton *btnRegister;
    
    QRadioButton *rbExpert;
    QRadioButton *rbFactory;
    QButtonGroup *roleGroup;
    QLabel *lblRoleWarning;
    
    QTextEdit *txtLog;
    
    // Connection and state
    ClientConn conn_;
    bool isConnected_;
    
    // Main windows
    ExpertMain *expertMain_;
    FactoryMain *factoryMain_;
};