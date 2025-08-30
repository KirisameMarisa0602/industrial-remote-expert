#include "loginwindow.h"
#include "expertmain.h"
#include "factorymain.h"
#include <QApplication>
#include <QStyleFactory>

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent)
    , isConnected_(false)
    , expertMain_(nullptr)
    , factoryMain_(nullptr)
{
    setupUI();
    
    // Connect signals
    connect(&conn_, &ClientConn::connected, this, &LoginWindow::onConnected);
    connect(&conn_, &ClientConn::disconnected, this, &LoginWindow::onDisconnected);
    connect(&conn_, &ClientConn::packetArrived, this, &LoginWindow::onPkt);
    
    connect(btnConnect, &QPushButton::clicked, this, &LoginWindow::onConnect);
    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegister);
    connect(roleGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &LoginWindow::onRoleChanged);
    
    updateFormState();
}

void LoginWindow::setupUI()
{
    setWindowTitle("Industrial Remote Expert - Login");
    setFixedSize(500, 700);
    
    // Apply modern styling
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f5f5f5;
        }
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 5px;
            margin-top: 1ex;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QLineEdit {
            border: 2px solid #ddd;
            border-radius: 4px;
            padding: 8px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #4CAF50;
        }
        QPushButton {
            background-color: #4CAF50;
            border: none;
            color: white;
            padding: 10px;
            text-align: center;
            font-size: 14px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        QRadioButton {
            font-size: 14px;
            spacing: 5px;
        }
        QLabel {
            font-size: 14px;
        }
        QTextEdit {
            border: 1px solid #ddd;
            border-radius: 4px;
            font-family: monospace;
        }
    )");
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setMargin(20);
    
    // Title
    QLabel *titleLabel = new QLabel("Industrial Remote Expert System");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333; margin-bottom: 20px;");
    mainLayout->addWidget(titleLabel);
    
    // Connection Group
    QGroupBox *connGroup = new QGroupBox("Server Connection");
    QVBoxLayout *connLayout = new QVBoxLayout(connGroup);
    
    QHBoxLayout *hostLayout = new QHBoxLayout;
    hostLayout->addWidget(new QLabel("Host:"));
    edHost = new QLineEdit("127.0.0.1");
    hostLayout->addWidget(edHost);
    
    hostLayout->addWidget(new QLabel("Port:"));
    edPort = new QLineEdit("9000");
    edPort->setMaximumWidth(80);
    hostLayout->addWidget(edPort);
    
    btnConnect = new QPushButton("Connect");
    hostLayout->addWidget(btnConnect);
    
    connLayout->addLayout(hostLayout);
    mainLayout->addWidget(connGroup);
    
    // Role Selection Group
    QGroupBox *roleGroup = new QGroupBox("Select Role");
    QVBoxLayout *roleLayout = new QVBoxLayout(roleGroup);
    
    this->roleGroup = new QButtonGroup(this);
    rbExpert = new QRadioButton("Expert - Remote assistance provider");
    rbFactory = new QRadioButton("Factory - Equipment operator requiring assistance");
    
    this->roleGroup->addButton(rbExpert);
    this->roleGroup->addButton(rbFactory);
    
    roleLayout->addWidget(rbExpert);
    roleLayout->addWidget(rbFactory);
    
    lblRoleWarning = new QLabel("未选择身份");
    lblRoleWarning->setAlignment(Qt::AlignCenter);
    lblRoleWarning->setStyleSheet("color: #888; font-style: italic; margin-top: 10px;");
    roleLayout->addWidget(lblRoleWarning);
    
    mainLayout->addWidget(roleGroup);
    
    // Authentication Group
    QGroupBox *authGroup = new QGroupBox("Authentication");
    QVBoxLayout *authLayout = new QVBoxLayout(authGroup);
    
    QHBoxLayout *userLayout = new QHBoxLayout;
    userLayout->addWidget(new QLabel("Username:"));
    edLoginUser = new QLineEdit();
    edLoginUser->setPlaceholderText("Enter username");
    userLayout->addWidget(edLoginUser);
    authLayout->addLayout(userLayout);
    
    QHBoxLayout *passLayout = new QHBoxLayout;
    passLayout->addWidget(new QLabel("Password:"));
    edLoginPass = new QLineEdit();
    edLoginPass->setPlaceholderText("Enter password");
    edLoginPass->setEchoMode(QLineEdit::Password);
    userLayout->addWidget(edLoginPass);
    authLayout->addLayout(passLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLogin = new QPushButton("Login");
    btnRegister = new QPushButton("Register");
    btnLayout->addWidget(btnLogin);
    btnLayout->addWidget(btnRegister);
    authLayout->addLayout(btnLayout);
    
    mainLayout->addWidget(authGroup);
    
    // Log Area
    QLabel *logLabel = new QLabel("Connection Log:");
    mainLayout->addWidget(logLabel);
    
    txtLog = new QTextEdit();
    txtLog->setMaximumHeight(150);
    txtLog->setReadOnly(true);
    mainLayout->addWidget(txtLog);
    
    mainLayout->addStretch();
}

void LoginWindow::updateFormState()
{
    bool hasRole = rbExpert->isChecked() || rbFactory->isChecked();
    
    if (!hasRole) {
        lblRoleWarning->setText("未选择身份");
        lblRoleWarning->setStyleSheet("color: #888; font-style: italic;");
    } else {
        lblRoleWarning->setText("");
    }
    
    btnLogin->setEnabled(isConnected_ && hasRole);
    btnRegister->setEnabled(isConnected_ && hasRole);
    btnConnect->setEnabled(!isConnected_);
}

void LoginWindow::onConnect()
{
    QString host = edHost->text().trimmed();
    quint16 port = edPort->text().toUShort();
    
    if (host.isEmpty() || port == 0) {
        QMessageBox::warning(this, "Connection Error", "Please enter valid host and port");
        return;
    }
    
    txtLog->append(QString("Connecting to %1:%2...").arg(host).arg(port));
    conn_.connectTo(host, port);
    btnConnect->setEnabled(false);
}

void LoginWindow::onLogin()
{
    QString username = edLoginUser->text().trimmed();
    QString password = edLoginPass->text();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login Error", "Please enter both username and password");
        return;
    }
    
    if (!rbExpert->isChecked() && !rbFactory->isChecked()) {
        QMessageBox::warning(this, "Role Error", "Please select a role before logging in");
        return;
    }
    
    QJsonObject loginData{
        {"username", username}, 
        {"password", password}
    };
    conn_.send(MSG_LOGIN, loginData);
    txtLog->append(QString("Attempting to login as: %1").arg(username));
}

void LoginWindow::onRegister()
{
    QString username = edLoginUser->text().trimmed();
    QString password = edLoginPass->text();
    QString role = rbExpert->isChecked() ? "expert" : "factory";
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Registration Error", "Please enter both username and password");
        return;
    }
    
    if (!rbExpert->isChecked() && !rbFactory->isChecked()) {
        QMessageBox::warning(this, "Role Error", "Please select a role before registering");
        return;
    }
    
    QJsonObject registerData{
        {"username", username}, 
        {"password", password},
        {"role", role}
    };
    conn_.send(MSG_REGISTER, registerData);
    txtLog->append(QString("Attempting to register as: %1 (%2)").arg(username, role));
}

void LoginWindow::onPkt(Packet p)
{
    if (p.type == MSG_SERVER_EVENT) {
        int code = p.json.value("code").toInt();
        QString message = p.json.value("message").toString();
        
        txtLog->append(QString("Server: %1 (code: %2)").arg(message).arg(code));
        
        if (code == 0) {
            // Success response
            if (message.contains("login successful")) {
                QString role = p.json.value("role").toString();
                QString token = p.json.value("token").toString();
                
                txtLog->append(QString("Login successful! Role: %1").arg(role));
                showMainWindow(role);
            } else if (message.contains("registration successful")) {
                QMessageBox::information(this, "Registration", "Registration successful! You can now login.");
            }
        } else {
            // Error response
            QMessageBox::warning(this, "Error", message);
        }
    }
}

void LoginWindow::onConnected()
{
    isConnected_ = true;
    txtLog->append("Connected to server successfully!");
    updateFormState();
}

void LoginWindow::onDisconnected()
{
    isConnected_ = false;
    txtLog->append("Disconnected from server");
    updateFormState();
}

void LoginWindow::onRoleChanged()
{
    updateFormState();
}

void LoginWindow::showMainWindow(const QString& role)
{
    if (role == "expert") {
        if (!expertMain_) {
            expertMain_ = new ExpertMain(&conn_, this);
        }
        expertMain_->show();
    } else if (role == "factory") {
        if (!factoryMain_) {
            factoryMain_ = new FactoryMain(&conn_, this);
        }
        factoryMain_->show();
    }
    
    // Hide login window
    hide();
}