#include "mainwindow.h"
#include "loginregisterdialog.h"
#include "modernstyle.h"
#include <QCameraInfo>
#include <QBuffer>
#include <QCameraViewfinderSettings>
#include <QVideoFrame>
#include <QVideoProbe>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QPixmap>
#include <QCheckBox>
#include <QMessageBox>
#include <QListWidget>
#include <QDockWidget>
#include <QSplitter>
#include <QGridLayout>
#include <QTreeWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QGroupBox>

// 假设这些宏和类在其他地方定义
// #define MSG_JOIN_WORKORDER 100
// #define MSG_TEXT 101
// #define MSG_VIDEO_FRAME 102
// class Packet { ... };
// class ClientConn { ... };

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentUserRole_(UserRole::None)
    , camera_(nullptr)
    , probe_(nullptr)
    , settings_("irexp", "client-expert")
    , isConnected_(false)
    , isJoinedRoom_(false)
    , isAuthenticated_(false)
{
    // Apply modern dark theme
    setStyleSheet(ModernStyle::getDarkThemeStyleSheet());
    
    // Set window properties
    setWindowTitle("技术专家客户端 - Industrial Remote Expert");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // Create menu bar
    createMenuBar();
    
    // Create status bar
    statusBar()->showMessage("未连接到服务器");
    
    // Show login dialog first
    showLoginDialog();
}

void MainWindow::showLoginDialog()
{
    LoginRegisterDialog* loginDialog = new LoginRegisterDialog(this);
    
    connect(loginDialog, &LoginRegisterDialog::loginRequested, 
            this, &MainWindow::onLoginSuccess);
    connect(loginDialog, &LoginRegisterDialog::registerRequested, 
            this, &MainWindow::onRegisterSuccess);
    
    if (loginDialog->exec() == QDialog::Accepted) {
        currentUserRole_ = loginDialog->getSelectedRole();
        authenticatedUsername_ = loginDialog->getUsername();
        
        if (currentUserRole_ == UserRole::Expert) {
            setupExpertMainUI();
            statusBar()->showMessage(QString("已登录用户: %1 (技术专家)").arg(authenticatedUsername_));
        } else {
            QMessageBox::warning(this, "角色错误", "技术专家客户端只能使用技术专家身份登录！");
            QApplication::quit();
        }
    } else {
        QApplication::quit();
    }
    
    loginDialog->deleteLater();
}

void MainWindow::setupExpertMainUI()
{
    // Create central widget with video grid
    createVideoGrid();
    
    // Create dockable panels
    createNavigationPanel();
    createParticipantPanel(); 
    createChatPanel();
    
    // Set up connections
    connect(&conn_, &ClientConn::packetArrived, this, &MainWindow::onPkt);
    connect(&conn_, &ClientConn::connected, this, &MainWindow::onConnected);
    connect(&conn_, &ClientConn::disconnected, this, &MainWindow::onDisconnected);
}

void MainWindow::createMenuBar()
{
    auto* fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction("连接服务器(&C)", this, &MainWindow::onConnect, QKeySequence("Ctrl+C"));
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&X)", this, &QWidget::close, QKeySequence("Ctrl+Q"));
    
    auto* workOrderMenu = menuBar()->addMenu("工单(&W)");
    workOrderMenu->addAction("加入工单(&J)", this, &MainWindow::onJoin, QKeySequence("Ctrl+J"));
    workOrderMenu->addAction("离开工单(&L)", [this]() {
        // TODO: Implement leave work order
    });
    
    auto* videoMenu = menuBar()->addMenu("视频(&V)");
    videoMenu->addAction("开启/关闭摄像头(&T)", this, &MainWindow::onToggleCamera, QKeySequence("Ctrl+T"));
    
    auto* helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于(&A)", [this]() {
        QMessageBox::about(this, "关于", "Industrial Remote Expert\n技术专家客户端 v1.0");
    });
}

void MainWindow::createNavigationPanel()
{
    navigationDock_ = new QDockWidget("导航", this);
    navigationDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    auto* navWidget = new QWidget();
    auto* navLayout = new QVBoxLayout(navWidget);
    
    // Connection controls
    auto* connectionGroup = new QGroupBox("服务器连接");
    auto* connLayout = new QFormLayout(connectionGroup);
    
    edHost = new QLineEdit("127.0.0.1");
    edPort = new QLineEdit("9000");
    edPort->setMaximumWidth(80);
    auto* btnConn = new QPushButton("连接");
    
    connLayout->addRow("主机:", edHost);
    connLayout->addRow("端口:", edPort);
    connLayout->addRow("", btnConn);
    navLayout->addWidget(connectionGroup);
    
    // Work orders
    auto* workOrderGroup = new QGroupBox("工单列表");
    auto* woLayout = new QVBoxLayout(workOrderGroup);
    
    workOrderList_ = new QTreeWidget();
    workOrderList_->setHeaderLabels({"工单", "状态", "创建时间"});
    woLayout->addWidget(workOrderList_);
    
    auto* joinLayout = new QHBoxLayout();
    edRoom = new QLineEdit("R123");
    edRoom->setPlaceholderText("输入工单号");
    edUser = new QLineEdit(authenticatedUsername_);
    edUser->setVisible(false); // Hidden in modern UI, using authenticated username
    joinButton_ = new QPushButton("加入工单");
    joinButton_->setEnabled(false);
    
    joinLayout->addWidget(edRoom);
    joinLayout->addWidget(joinButton_);
    woLayout->addLayout(joinLayout);
    
    navLayout->addWidget(workOrderGroup);
    
    // Device data section
    auto* deviceGroup = new QGroupBox("设备数据");
    auto* deviceLayout = new QVBoxLayout(deviceGroup);
    
    auto* deviceList = new QListWidget();
    deviceList->addItem("温度传感器 - 正常");
    deviceList->addItem("压力传感器 - 正常");  
    deviceList->addItem("流量计 - 警告");
    deviceLayout->addWidget(deviceList);
    
    navLayout->addWidget(deviceGroup);
    
    navLayout->addStretch();
    
    navigationDock_->setWidget(navWidget);
    addDockWidget(Qt::LeftDockWidgetArea, navigationDock_);
    
    // Connect signals
    connect(btnConn, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(joinButton_, &QPushButton::clicked, this, &MainWindow::onJoin);
}

void MainWindow::createVideoGrid()
{
    videoGrid_ = new QWidget();
    auto* gridLayout = new QGridLayout(videoGrid_);
    
    // Create video display areas for up to 6 participants
    for (int i = 0; i < 6; ++i) {
        auto* videoFrame = new QLabel();
        videoFrame->setMinimumSize(320, 240);
        videoFrame->setStyleSheet(R"(
            QLabel {
                border: 2px solid #555555;
                border-radius: 8px;
                background-color: #404040;
                color: #ffffff;
            }
        )");
        videoFrame->setAlignment(Qt::AlignCenter);
        videoFrame->setText(i == 0 ? "本地视频\n(未开启)" : QString("参与者 %1\n(未连接)").arg(i));
        videoFrame->setScaledContents(true);
        
        gridLayout->addWidget(videoFrame, i / 3, i % 3);
        
        if (i == 0) {
            videoLabel_ = videoFrame; // Keep reference to local video
        } else if (i == 1) {
            remoteLabel_ = videoFrame; // Keep reference for compatibility
        }
    }
    
    setCentralWidget(videoGrid_);
}

void MainWindow::createParticipantPanel()
{
    participantDock_ = new QDockWidget("参与者", this);
    participantDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    auto* participantWidget = new QWidget();
    auto* participantLayout = new QVBoxLayout(participantWidget);
    
    auto* participantLabel = new QLabel("在线参与者:");
    participantLabel->setProperty("class", "heading");
    participantLayout->addWidget(participantLabel);
    
    participantList_ = new QListWidget();
    participantList_->addItem("🎥 " + authenticatedUsername_ + " (你)");
    participantLayout->addWidget(participantList_);
    
    // Control buttons
    auto* controlGroup = new QGroupBox("控制");
    auto* controlLayout = new QVBoxLayout(controlGroup);
    
    btnCamera_ = new QPushButton("开启摄像头");
    auto* btnMute = new QPushButton("静音");
    auto* btnRecord = new QPushButton("开始录制");
    
    controlLayout->addWidget(btnCamera_);
    controlLayout->addWidget(btnMute);
    controlLayout->addWidget(btnRecord);
    
    chkAutoStart_ = new QCheckBox("自动开启摄像头");
    chkAutoStart_->setChecked(loadAutoStartPreference());
    controlLayout->addWidget(chkAutoStart_);
    
    participantLayout->addWidget(controlGroup);
    participantLayout->addStretch();
    
    participantDock_->setWidget(participantWidget);
    addDockWidget(Qt::RightDockWidgetArea, participantDock_);
    
    // Connect signals
    connect(btnCamera_, &QPushButton::clicked, this, &MainWindow::onToggleCamera);
    connect(chkAutoStart_, &QCheckBox::toggled, this, &MainWindow::onAutoStartToggled);
}

void MainWindow::createChatPanel()
{
    chatDock_ = new QDockWidget("聊天", this);
    chatDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    auto* chatWidget = new QWidget();
    auto* chatLayout = new QVBoxLayout(chatWidget);
    
    // Chat tabs for different conversations
    chatTabs_ = new QTabWidget();
    
    // Main chat tab
    auto* mainChatTab = new QWidget();
    auto* mainChatLayout = new QVBoxLayout(mainChatTab);
    
    chatDisplay_ = new QTextEdit();
    chatDisplay_->setReadOnly(true);
    chatDisplay_->append("系统: 欢迎使用技术专家客户端");
    txtLog = chatDisplay_; // Keep reference for compatibility
    
    chatInput_ = new QLineEdit();
    chatInput_->setPlaceholderText("输入消息...");
    edInput = chatInput_; // Keep reference for compatibility
    
    auto* sendButton = new QPushButton("发送");
    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(chatInput_);
    inputLayout->addWidget(sendButton);
    
    mainChatLayout->addWidget(chatDisplay_);
    mainChatLayout->addLayout(inputLayout);
    
    chatTabs_->addTab(mainChatTab, "主聊天");
    chatLayout->addWidget(chatTabs_);
    
    chatDock_->setWidget(chatWidget);
    addDockWidget(Qt::BottomDockWidgetArea, chatDock_);
    
    // Connect signals
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendText);
    connect(chatInput_, &QLineEdit::returnPressed, this, &MainWindow::onSendText);
    
    // Auto-start camera if enabled
    tryAutoStartCamera();
}

/* ---------- 网络 ---------- */
void MainWindow::onConnect()
{
    conn_.connectTo(edHost->text(), edPort->text().toUShort());
    txtLog->append("Connecting...");
}
void MainWindow::onJoin()
{
    QJsonObject j{{"roomId", edRoom->text()}, {"user", authenticatedUsername_}};
    conn_.send(MSG_JOIN_WORKORDER, j);
    currentRoom_ = edRoom->text();
    chatDisplay_->append(QString("正在加入工单: %1").arg(edRoom->text()));
}

void MainWindow::onSendText()
{
    if (edInput->text().trimmed().isEmpty()) return;
    
    QJsonObject j{{"roomId",  currentRoom_},
                  {"sender",  authenticatedUsername_},
                  {"content", edInput->text()},
                  {"ts",      QDateTime::currentMSecsSinceEpoch()}};
    
    chatDisplay_->append(QString("[我] %1").arg(edInput->text()));
    conn_.send(MSG_TEXT, j);
    edInput->clear();
}
void MainWindow::onPkt(Packet p)
{
    switch (p.type)
    {
    case MSG_TEXT:
    {
        QString sender = p.json["sender"].toString();
        QString content = p.json["content"].toString();
        QString roomId = p.json["roomId"].toString();
        
        if (roomId == currentRoom_ && sender != authenticatedUsername_) {
            chatDisplay_->append(QString("[%1] %2").arg(sender, content));
        }
        break;
    }
    case MSG_VIDEO_FRAME:
    {
        QString sender = p.json["sender"].toString();
        QString roomId = p.json["roomId"].toString();
        
        // Only show video from other users in current room
        if (sender != authenticatedUsername_ && roomId == currentRoom_ && isJoinedRoom_) {
            QPixmap pix; 
            pix.loadFromData(p.bin);
            if (!pix.isNull()) {
                remoteLabel_->setPixmap(pix.scaled(remoteLabel_->size(), Qt::KeepAspectRatio));
            }
        }
        break;
    }
    case MSG_SERVER_EVENT:
    {
        chatDisplay_->append(QString("[服务器] %1")
                       .arg(QString::fromUtf8(QJsonDocument(p.json).toJson())));
        
        int code = p.json.value("code").toInt();
        QString message = p.json.value("message").toString();
        // Handle authentication responses
        if (code == 0 && message == "login successful") {
            isAuthenticated_ = true;
            sessionToken_ = p.json.value("token").toString();
            joinButton_->setEnabled(true);
            
            chatDisplay_->append("✅ 登录成功！可以加入工单了。");
            statusBar()->showMessage(QString("已认证用户: %1").arg(authenticatedUsername_));
        }
        // Handle registration success
        else if (code == 0 && message == "registration successful") {
            chatDisplay_->append("✅ 注册成功！请重新登录。");
        }
        // Handle room join success
        else if (code == 0 && message == "joined") {
            isJoinedRoom_ = true;
            chatDisplay_->append(QString("✅ 成功加入工单: %1").arg(currentRoom_));
            statusBar()->showMessage(QString("已加入工单: %1").arg(currentRoom_));
            
            // Update participant list
            participantList_->clear();
            participantList_->addItem("🎥 " + authenticatedUsername_ + " (你)");
            
            // Try auto-start camera
            tryAutoStartCamera();
        }
        // Handle error responses
        else if (code != 0) {
            if (message.contains("authentication required")) {
                chatDisplay_->append("❌ 错误: 请先登录再加入工单");
            } else if (message.contains("invalid username or password")) {
                chatDisplay_->append("❌ 错误: 用户名或密码错误");
            } else if (message.contains("username already exists")) {
                chatDisplay_->append("❌ 错误: 用户名已存在，请尝试其他用户名");
            } else {
                chatDisplay_->append(QString("❌ 错误: %1").arg(message));
            }
        }
        break;
    }
    }
}

/* ---------- 摄像头 ---------- */
void MainWindow::startCamera()
{
    if (camera_) return;

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        txtLog->append("没有可用摄像头");
        QMessageBox::information(this, "Camera Not Found", 
            "No camera device found. Please:\n"
            "• Install qtmultimedia and gstreamer packages\n"
            "• If running in VM, pass through webcam device\n"
            "• Check camera permissions");
        return;
    }

    camera_ = new QCamera(cameras.first(), this);   // 取第一个物理摄像头

    probe_ = new QVideoProbe(this);
    if (probe_->setSource(camera_)) {
        connect(probe_, &QVideoProbe::videoFrameProbed,
                this, &MainWindow::onVideoFrame);
        camera_->start(); // 启动摄像头
    } else {
        txtLog->append("无法设置探头或启动摄像头");
        // 清理已创建的摄像头对象
        camera_->deleteLater();
        camera_ = nullptr;
        probe_->deleteLater();
        probe_ = nullptr;
        return;
    }

    btnCamera_->setText("关闭摄像头");
    txtLog->append("摄像头已启动");
}

void MainWindow::stopCamera()
{
    if (!camera_) return;
    camera_->stop();
    // 确保在删除 QVideoProbe 之前断开连接
    if (probe_) {
        disconnect(probe_, &QVideoProbe::videoFrameProbed,
                   this, &MainWindow::onVideoFrame);
        probe_->deleteLater();
        probe_ = nullptr;
    }
    camera_->deleteLater();
    camera_ = nullptr;

    videoLabel_->setText("本地视频预览");
    btnCamera_->setText("开启摄像头");
    txtLog->append("摄像头已关闭");
}

void MainWindow::onToggleCamera()
{
    if (camera_) stopCamera();
    else         startCamera();
}

/* ---------- 帧采集 + 发送 ---------- */
void MainWindow::onVideoFrame(const QVideoFrame &frame)
{
    if (!camera_ || !frame.isValid()) {
        return;
    }

    QVideoFrame clone(frame);

    if (!clone.map(QAbstractVideoBuffer::ReadOnly)) {
        txtLog->append("onVideoFrame: 无法映射视频帧数据。");
        return;
    }

    QImage img;
    QVideoFrame::PixelFormat pixelFormat = clone.pixelFormat();

    qDebug() << "Video Frame Pixel Format:" << pixelFormat;
    txtLog->append(QString("检测到视频帧像素格式: %1").arg(pixelFormat));


    if (pixelFormat == QVideoFrame::Format_ARGB32 ||
        pixelFormat == QVideoFrame::Format_ARGB32_Premultiplied) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_ARGB32);
    } else if (pixelFormat == QVideoFrame::Format_RGB32) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_RGB32);
    }
    // 处理 BGR32: 加载为 RGB32，然后交换通道
    else if (pixelFormat == QVideoFrame::Format_BGR32) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_RGB32).rgbSwapped();
    }
    else if (pixelFormat == QVideoFrame::Format_RGB24) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_RGB888); // RGB888 对应 24-bit RGB
    }
    // 处理 BGR24: 加载为 RGB888，然后交换通道
    else if (pixelFormat == QVideoFrame::Format_BGR24) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_RGB888).rgbSwapped();
    }
    // **核心修改：处理 YUYV 格式**
    else if (pixelFormat == QVideoFrame::Format_YUYV) {
        const uchar *yuyv_data = clone.bits();
        int width = clone.width();
        int height = clone.height();
        int bytesPerLine = clone.bytesPerLine();

        // 为 RGB 图像分配内存。使用 QImage::Format_RGB32 来简化处理
        // 或者使用 QImage::Format_RGB888 如果你想节省内存
        img = QImage(width, height, QImage::Format_RGB32);

        for (int y = 0; y < height; ++y) {
            const uchar *line_ptr = yuyv_data + y * bytesPerLine;
            QRgb *rgb_line_ptr = (QRgb *)img.scanLine(y);

            for (int x = 0; x < width; x += 2) {
                // YUYV 格式: Y0 U0 Y1 V0 ...
                // 每个宏像素包含 Y0, U, Y1, V
                int y0 = line_ptr[x * 2];
                int u  = line_ptr[x * 2 + 1];
                int y1 = line_ptr[x * 2 + 2];
                int v  = line_ptr[x * 2 + 3];

                // YUV to RGB 转换公式 (BT.601 标准，常见于摄像头)
                // 调整亮度偏移
                y0 = y0 - 16;
                y1 = y1 - 16;
                u  = u  - 128;
                v  = v  - 128;

                // 计算第一个像素 (Y0) 的 RGB
                int r0 = qBound(0, (298 * y0 + 409 * v + 128) >> 8, 255);
                int g0 = qBound(0, (298 * y0 - 100 * u - 208 * v + 128) >> 8, 255);
                int b0 = qBound(0, (298 * y0 + 516 * u + 128) >> 8, 255);
                rgb_line_ptr[x] = qRgb(r0, g0, b0);

                // 计算第二个像素 (Y1) 的 RGB
                int r1 = qBound(0, (298 * y1 + 409 * v + 128) >> 8, 255);
                int g1 = qBound(0, (298 * y1 - 100 * u - 208 * v + 128) >> 8, 255);
                int b1 = qBound(0, (298 * y1 + 516 * u + 128) >> 8, 255);
                rgb_line_ptr[x + 1] = qRgb(r1, g1, b1);
            }
        }
    }
    // 如果像素格式不被直接支持，你可能需要根据实际情况实现转换逻辑
    else {
        txtLog->append(QString("onVideoFrame: 不支持的像素格式 %1，无法直接转换为 QImage。").arg(pixelFormat));
        clone.unmap();
        return;
    }


    if (img.isNull() || img.sizeInBytes() == 0) { // 检查 img 是否被有效构建
        txtLog->append("onVideoFrame: 创建QImage失败或图像数据为空。");
        clone.unmap();
        return;
    }

    clone.unmap(); // 尽早解除映射，释放资源

    // 本地预览
    QPixmap pixmap = QPixmap::fromImage(img);
    if (!pixmap.isNull()) {
        videoLabel_->setPixmap(pixmap.scaled(videoLabel_->size(), Qt::KeepAspectRatio));
    } else {
        txtLog->append("onVideoFrame: 无法从QImage创建QPixmap。");
    }

    // 远端发送
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    if (!img.save(&buffer, "JPEG", 60)) {
        txtLog->append("onVideoFrame: 无法将图像保存为JPEG格式。");
        return;
    }
    buffer.close();

    // 添加防护条件：只有在连接且已加入房间时才发送
    if (!conn_.isConnected() || !isJoinedRoom_) {
        return; // 不发送帧数据
    }

    QJsonObject j{{"roomId", currentRoom_},
                  {"sender", authenticatedUsername_},
                  {"ts",     QDateTime::currentMSecsSinceEpoch()}};
    conn_.send(MSG_VIDEO_FRAME, j, jpeg);
}

/* ---------- 连接状态处理 ---------- */
void MainWindow::onConnected()
{
    isConnected_ = true;
    joinButton_->setEnabled(true);
    statusBar()->showMessage(QString("已连接到服务器 - 用户: %1").arg(authenticatedUsername_));
    chatDisplay_->append("✅ 已连接到服务器");
    
    // If we have stored credentials from login dialog, send them now
    if (!authenticatedUsername_.isEmpty() && currentUserRole_ != UserRole::None) {
        // Auto-authenticate with stored credentials
        // This would be triggered from the login dialog after successful connection
    }
}

void MainWindow::onDisconnected()
{
    isConnected_ = false;
    isJoinedRoom_ = false;
    isAuthenticated_ = false;
    currentRoom_.clear();
    sessionToken_.clear();
    
    joinButton_->setEnabled(false);
    statusBar()->showMessage("与服务器断开连接");
    chatDisplay_->append("❌ 与服务器断开连接");
}

/* ---------- 自动启动功能 ---------- */
void MainWindow::onAutoStartToggled(bool checked)
{
    saveAutoStartPreference(checked);
}

void MainWindow::tryAutoStartCamera()
{
    // 检查是否启用自动启动，且当前没有摄像头正在运行
    if (!chkAutoStart_->isChecked() || camera_) {
        return;
    }
    
    // 检查是否满足启动条件：已连接且已加入房间
    if (!isConnected_ || !isJoinedRoom_) {
        return;
    }
    
    // 尝试启动摄像头
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        QMessageBox::information(this, "Camera Not Found", 
            "No camera device found. Please:\n"
            "• Install qtmultimedia and gstreamer packages\n"
            "• If running in VM, pass through webcam device\n"
            "• Check camera permissions");
        return;
    }
    
    startCamera();
}

void MainWindow::saveAutoStartPreference(bool enabled)
{
    settings_.setValue("autoStartCamera", enabled);
}

bool MainWindow::loadAutoStartPreference()
{
    return settings_.value("autoStartCamera", true).toBool(); // 默认启用
}

/* ---------- 登录/注册功能 ---------- */
void MainWindow::onLoginSuccess(const QString& username, const QString& password, UserRole role)
{
    if (!isConnected_) {
        // Auto-connect to server first
        conn_.connectTo("127.0.0.1", 9000);
        // Store credentials to send after connection
        authenticatedUsername_ = username;
        currentUserRole_ = role;
        return;
    }
    
    QString roleStr = (role == UserRole::Expert) ? "expert" : "factory";
    QJsonObject loginData{
        {"username", username}, 
        {"password", password},
        {"role", roleStr}
    };
    
    conn_.send(MSG_LOGIN, loginData);
    chatDisplay_->append(QString("正在登录: %1 (%2)").arg(username).arg(roleStr == "expert" ? "技术专家" : "工厂用户"));
}

void MainWindow::onRegisterSuccess(const QString& username, const QString& password, UserRole role)
{
    if (!isConnected_) {
        // Auto-connect to server first
        conn_.connectTo("127.0.0.1", 9000);
        // Store credentials to send after connection
        authenticatedUsername_ = username;
        currentUserRole_ = role;
        return;
    }
    
    QString roleStr = (role == UserRole::Expert) ? "expert" : "factory";
    QJsonObject registerData{
        {"username", username}, 
        {"password", password},
        {"role", roleStr}
    };
    
    conn_.send(MSG_REGISTER, registerData);
    chatDisplay_->append(QString("正在注册: %1 (%2)").arg(username).arg(roleStr == "expert" ? "技术专家" : "工厂用户"));
}
