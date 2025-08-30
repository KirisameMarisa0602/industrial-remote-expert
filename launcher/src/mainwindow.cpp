#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isConnected_(false)
{
    setupUI();
    
    // 连接网络信号
    connect(&conn_, &ClientConn::connected, this, &MainWindow::onConnected);
    connect(&conn_, &ClientConn::disconnected, this, &MainWindow::onDisconnected);
    connect(&conn_, &ClientConn::packetArrived, this, &MainWindow::onPacketReceived);
    
    // 连接超时定时器
    connectionTimer_ = new QTimer(this);
    connectionTimer_->setSingleShot(true);
    connectionTimer_->setInterval(10000); // 10秒超时
    connect(connectionTimer_, &QTimer::timeout, this, &MainWindow::onConnectionTimeout);
}

void MainWindow::setupUI()
{
    setWindowTitle("工业现场远程专家支持系统 - 启动器");
    setFixedSize(450, 350);
    
    // 设置中央部件
    stackedWidget_ = new QStackedWidget(this);
    setCentralWidget(stackedWidget_);
    
    setupConnectionPage();
    setupAuthPage();
    setupLoadingPage();
    
    // 默认显示连接页面
    switchToPage(0);
}

void MainWindow::setupConnectionPage()
{
    connectionPage_ = new QWidget();
    auto *layout = new QVBoxLayout(connectionPage_);
    layout->setSpacing(20);
    layout->setContentsMargins(40, 40, 40, 40);
    
    // 标题
    auto *titleLabel = new QLabel("连接到服务器");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #2c3e50; }");
    layout->addWidget(titleLabel);
    
    // 连接表单
    auto *formLayout = new QHBoxLayout();
    
    auto *hostLabel = new QLabel("服务器地址:");
    hostEdit_ = new QLineEdit("127.0.0.1");
    hostEdit_->setFixedWidth(120);
    
    auto *portLabel = new QLabel("端口:");
    portEdit_ = new QLineEdit("9000");
    portEdit_->setFixedWidth(60);
    
    connectButton_ = new QPushButton("连接");
    connectButton_->setFixedWidth(80);
    
    formLayout->addWidget(hostLabel);
    formLayout->addWidget(hostEdit_);
    formLayout->addWidget(portLabel);
    formLayout->addWidget(portEdit_);
    formLayout->addWidget(connectButton_);
    formLayout->addStretch();
    
    layout->addLayout(formLayout);
    
    // 连接状态
    connectionStatus_ = new QLabel("等待连接...");
    connectionStatus_->setAlignment(Qt::AlignCenter);
    connectionStatus_->setStyleSheet("QLabel { color: #7f8c8d; font-style: italic; }");
    layout->addWidget(connectionStatus_);
    
    layout->addStretch();
    
    // 连接按钮事件
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::onConnectToServer);
    
    stackedWidget_->addWidget(connectionPage_);
}

void MainWindow::setupAuthPage()
{
    authPage_ = new QWidget();
    auto *layout = new QVBoxLayout(authPage_);
    layout->setContentsMargins(50, 50, 50, 50);
    
    // 创建认证组件
    authWidget_ = new AuthWidget();
    layout->addWidget(authWidget_, 0, Qt::AlignCenter);
    
    // 连接认证信号
    connect(authWidget_, &AuthWidget::loginRequested, 
            this, &MainWindow::onLoginRequested);
    connect(authWidget_, &AuthWidget::registerRequested, 
            this, &MainWindow::onRegisterRequested);
    
    stackedWidget_->addWidget(authPage_);
}

void MainWindow::setupLoadingPage()
{
    loadingPage_ = new QWidget();
    auto *layout = new QVBoxLayout(loadingPage_);
    layout->setContentsMargins(50, 100, 50, 100);
    
    loadingLabel_ = new QLabel("正在启动客户端应用程序...");
    loadingLabel_->setAlignment(Qt::AlignCenter);
    loadingLabel_->setStyleSheet("QLabel { font-size: 14px; color: #2c3e50; }");
    layout->addWidget(loadingLabel_);
    
    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 0); // 无限进度条
    layout->addWidget(progressBar_);
    
    layout->addStretch();
    
    stackedWidget_->addWidget(loadingPage_);
}

void MainWindow::onConnectToServer()
{
    QString host = hostEdit_->text().trimmed();
    QString portStr = portEdit_->text().trimmed();
    
    if (host.isEmpty()) {
        showMessage("连接错误", "请输入服务器地址", true);
        return;
    }
    
    bool ok;
    quint16 port = portStr.toUShort(&ok);
    if (!ok || port == 0) {
        showMessage("连接错误", "请输入有效的端口号", true);
        return;
    }
    
    connectButton_->setEnabled(false);
    connectionStatus_->setText("正在连接...");;
    connectionStatus_->setStyleSheet("QLabel { color: #f39c12; font-style: italic; }");
    
    connectionTimer_->start();
    conn_.connectTo(host, port);
}

void MainWindow::onConnected()
{
    connectionTimer_->stop();
    isConnected_ = true;
    
    connectionStatus_->setText("连接成功！");
    connectionStatus_->setStyleSheet("QLabel { color: #27ae60; font-style: italic; }");
    
    // 延迟一秒后切换到认证页面
    QTimer::singleShot(1000, [this]() {
        switchToPage(1);
    });
}

void MainWindow::onDisconnected()
{
    connectionTimer_->stop();
    isConnected_ = false;
    connectButton_->setEnabled(true);
    
    connectionStatus_->setText("连接断开");
    connectionStatus_->setStyleSheet("QLabel { color: #e74c3c; font-style: italic; }");
    
    // 如果不在连接页面，则切换回连接页面
    if (stackedWidget_->currentIndex() != 0) {
        showMessage("连接断开", "与服务器的连接已断开，请重新连接", true);
        switchToPage(0);
    }
}

void MainWindow::onConnectionTimeout()
{
    connectButton_->setEnabled(true);
    connectionStatus_->setText("连接超时");
    connectionStatus_->setStyleSheet("QLabel { color: #e74c3c; font-style: italic; }");
    
    showMessage("连接超时", "无法连接到服务器，请检查网络设置", true);
}

void MainWindow::onPacketReceived(Packet pkt)
{
    if (pkt.type == MSG_SERVER_EVENT) {
        int code = pkt.json.value("code").toInt();
        QString message = pkt.json.value("message").toString();
        
        if (code == 0) {
            // 登录/注册成功
            QString role = pkt.json.value("role").toString();
            if (!role.isEmpty()) {
                currentRole_ = role;
                launchClientApplication(role);
            }
        } else {
            // 登录/注册失败
            showMessage("认证失败", message, true);
        }
    }
}

void MainWindow::onLoginRequested(const QString& username, const QString& password, const QString& role)
{
    if (!isConnected_) {
        showMessage("连接错误", "请先连接到服务器", true);
        return;
    }
    
    currentUsername_ = username;
    QJsonObject loginData{
        {"username", username}, 
        {"password", password}, 
        {"role", role}
    };
    conn_.send(MSG_LOGIN, loginData);
}

void MainWindow::onRegisterRequested(const QString& username, const QString& password, const QString& role)
{
    if (!isConnected_) {
        showMessage("连接错误", "请先连接到服务器", true);
        return;
    }
    
    currentUsername_ = username;
    QJsonObject registerData{
        {"username", username}, 
        {"password", password}, 
        {"role", role}
    };
    conn_.send(MSG_REGISTER, registerData);
}

void MainWindow::switchToPage(int pageIndex)
{
    stackedWidget_->setCurrentIndex(pageIndex);
}

void MainWindow::launchClientApplication(const QString& role)
{
    switchToPage(2); // 切换到加载页面
    
    QString applicationPath;
    QString applicationName;
    
    if (role == "工厂") {
        applicationPath = "../client-factory/client-factory";
        applicationName = "工厂客户端";
    } else if (role == "专家") {
        applicationPath = "../client-expert/client-expert";
        applicationName = "专家客户端";
    } else {
        showMessage("启动错误", "未知的用户角色: " + role, true);
        return;
    }
    
    loadingLabel_->setText(QString("正在启动%1...").arg(applicationName));
    
    // 启动客户端应用程序
    QProcess *process = new QProcess(this);
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, applicationName](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitCode)
                Q_UNUSED(exitStatus)
                
                // 客户端程序结束后关闭启动器
                showMessage("程序结束", applicationName + " 已结束运行");
                QTimer::singleShot(2000, this, &QWidget::close);
            });
    
    connect(process, &QProcess::errorOccurred, 
            [this, applicationName, applicationPath](QProcess::ProcessError error) {
                Q_UNUSED(error)
                showMessage("启动错误", 
                           QString("无法启动%1\n程序路径: %2\n请确保程序文件存在并具有执行权限")
                           .arg(applicationName).arg(applicationPath), true);
                switchToPage(1); // 返回认证页面
            });
    
    // 启动程序
    process->start(applicationPath);
    
    if (!process->waitForStarted(5000)) {
        showMessage("启动错误", 
                   QString("启动%1超时\n程序路径: %2")
                   .arg(applicationName).arg(applicationPath), true);
        switchToPage(1); // 返回认证页面
        return;
    }
    
    // 成功启动后延迟关闭启动器
    QTimer::singleShot(3000, [this]() {
        loadingLabel_->setText("客户端已启动，启动器即将关闭...");
        QTimer::singleShot(2000, this, &QWidget::close);
    });
}

void MainWindow::showMessage(const QString& title, const QString& message, bool isError)
{
    if (isError) {
        QMessageBox::warning(this, title, message);
    } else {
        QMessageBox::information(this, title, message);
    }
}