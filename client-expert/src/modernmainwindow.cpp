#include "modernmainwindow.h"
#include <QApplication>

ModernMainWindow::ModernMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isConnected_(false)
    , isInRoom_(false)
    , isCameraOn_(false)
    , isAudioOn_(false)
{
    setupUI();
    loadTheme();
    
    // 连接网络信号
    connect(&conn_, &ClientConn::connected, this, &ModernMainWindow::onConnected);
    connect(&conn_, &ClientConn::disconnected, this, &ModernMainWindow::onDisconnected);
    connect(&conn_, &ClientConn::packetArrived, this, &ModernMainWindow::onPacketReceived);
    
    // 设置默认页面
    switchToPage("dashboard");
    
    // 模拟连接到服务器 (在实际应用中应该通过认证流程)
    conn_.connectTo("127.0.0.1", 9000);
}

void ModernMainWindow::setupUI()
{
    setWindowTitle("专家客户端 - 工业远程专家系统");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    setupMenuBar();
    setupStatusBar();
    setupMainContent();
}

void ModernMainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction("连接服务器", [this]() {
        // TODO: 显示连接对话框
    });
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close);
    
    auto *viewMenu = menuBar()->addMenu("视图(&V)");
    viewMenu->addAction("全屏", this, &QWidget::showFullScreen);
    viewMenu->addAction("恢复窗口", this, &QWidget::showNormal);
    
    auto *helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于", [this]() {
        QMessageBox::about(this, "关于", "工业远程专家系统 - 专家客户端\\nVersion 1.0");
    });
}

void ModernMainWindow::setupStatusBar()
{
    statusBar_ = statusBar();
    
    connectionStatusLabel_ = new QLabel("未连接");
    connectionStatusLabel_->setStyleSheet("color: #e74c3c;");
    statusBar_->addWidget(new QLabel("连接状态:"));
    statusBar_->addWidget(connectionStatusLabel_);
    
    statusBar_->addPermanentWidget(new QLabel("|"));
    
    roomStatusLabel_ = new QLabel("未加入房间");
    roomStatusLabel_->setStyleSheet("color: #95a5a6;");
    statusBar_->addPermanentWidget(new QLabel("房间状态:"));
    statusBar_->addPermanentWidget(roomStatusLabel_);
    
    statusBar_->addPermanentWidget(new QLabel("|"));
    
    participantCountLabel_ = new QLabel("参与者: 0");
    statusBar_->addPermanentWidget(participantCountLabel_);
}

void ModernMainWindow::setupMainContent()
{
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建侧边栏
    sidebar_ = new SidebarWidget();
    sidebar_->addNavigationItem("dashboard", "仪表盘");
    sidebar_->addNavigationItem("communication", "视频通信");
    sidebar_->addNavigationItem("workorders", "工单管理");
    sidebar_->addNavigationItem("settings", "设置");
    
    connect(sidebar_, &SidebarWidget::navigationChanged, 
            this, &ModernMainWindow::onNavigationChanged);
    
    mainLayout->addWidget(sidebar_);
    
    // 创建主内容区域
    stackedWidget_ = new QStackedWidget();
    mainLayout->addWidget(stackedWidget_, 1);
    
    // 设置页面
    setupDashboardPage();
    setupCommunicationPage();
    setupWorkOrderPage();
    setupSettingsPage();
}

void ModernMainWindow::setupDashboardPage()
{
    dashboardPage_ = new QWidget();
    auto *layout = new QVBoxLayout(dashboardPage_);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    // 页面标题
    auto *titleLabel = new QLabel("系统仪表盘");
    titleLabel->setProperty("class", "title");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db; margin-bottom: 20px;");
    layout->addWidget(titleLabel);
    
    // 仪表盘组件
    dashboard_ = new DashboardWidget();
    layout->addWidget(dashboard_);
    
    // 快速操作区域
    auto *quickActionsGroup = new QGroupBox("快速操作");
    auto *quickLayout = new QHBoxLayout(quickActionsGroup);
    
    auto *connectButton = new QPushButton("连接服务器");
    auto *joinRoomButton = new QPushButton("加入房间");
    auto *startVideoButton = new QPushButton("开始视频");
    
    quickLayout->addWidget(connectButton);
    quickLayout->addWidget(joinRoomButton);
    quickLayout->addWidget(startVideoButton);
    quickLayout->addStretch();
    
    layout->addWidget(quickActionsGroup);
    layout->addStretch();
    
    stackedWidget_->addWidget(dashboardPage_);
}

void ModernMainWindow::setupCommunicationPage()
{
    communicationPage_ = new QWidget();
    auto *layout = new QVBoxLayout(communicationPage_);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    // 页面标题
    auto *titleLabel = new QLabel("视频通信");
    titleLabel->setProperty("class", "title");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db; margin-bottom: 20px;");
    layout->addWidget(titleLabel);
    
    // 房间控制区域
    auto *roomControlGroup = new QGroupBox("房间控制");
    auto *roomLayout = new QHBoxLayout(roomControlGroup);
    
    roomCodeInput_ = new QLineEdit();
    roomCodeInput_->setPlaceholderText("输入房间代码...");
    joinRoomButton_ = new QPushButton("加入房间");
    connect(joinRoomButton_, &QPushButton::clicked, this, &ModernMainWindow::onJoinRoom);
    
    roomLayout->addWidget(new QLabel("房间代码:"));
    roomLayout->addWidget(roomCodeInput_);
    roomLayout->addWidget(joinRoomButton_);
    roomLayout->addStretch();
    
    layout->addWidget(roomControlGroup);
    
    // 主通信区域
    communicationSplitter_ = new QSplitter(Qt::Horizontal);
    
    // 视频区域
    setupVideoWidget();
    communicationSplitter_->addWidget(videoWidget_);
    
    // 聊天区域
    setupChatWidget();
    communicationSplitter_->addWidget(chatWidget_);
    
    communicationSplitter_->setSizes({800, 400});
    layout->addWidget(communicationSplitter_, 1);
    
    stackedWidget_->addWidget(communicationPage_);
}

void ModernMainWindow::setupVideoWidget()
{
    videoWidget_ = new QWidget();
    auto *layout = new QVBoxLayout(videoWidget_);
    layout->setContentsMargins(10, 10, 10, 10);
    
    // 主视频显示区域
    remoteVideoLabel_ = new QLabel("等待远程视频...");
    remoteVideoLabel_->setProperty("class", "video-main");
    remoteVideoLabel_->setMinimumSize(640, 480);
    remoteVideoLabel_->setAlignment(Qt::AlignCenter);
    remoteVideoLabel_->setStyleSheet(R"(
        QLabel {
            border: 2px solid #27ae60;
            border-radius: 8px;
            background-color: #2c3e50;
            color: #95a5a6;
        }
    )");
    layout->addWidget(remoteVideoLabel_, 1);
    
    // 本地视频预览区域
    auto *localVideoGroup = new QGroupBox("本地预览");
    auto *localLayout = new QHBoxLayout(localVideoGroup);
    
    localVideoLabel_ = new QLabel("本地视频预览");
    localVideoLabel_->setProperty("class", "video-preview");
    localVideoLabel_->setFixedSize(240, 180);
    localVideoLabel_->setAlignment(Qt::AlignCenter);
    localVideoLabel_->setStyleSheet(R"(
        QLabel {
            border: 2px solid #3498db;
            border-radius: 8px;
            background-color: #2c3e50;
            color: #95a5a6;
        }
    )");
    localLayout->addWidget(localVideoLabel_);
    
    // 控制按钮
    auto *controlLayout = new QVBoxLayout();
    cameraButton_ = new QPushButton("开启摄像头");
    audioButton_ = new QPushButton("开启麦克风");
    
    connect(cameraButton_, &QPushButton::clicked, this, &ModernMainWindow::onToggleCamera);
    connect(audioButton_, &QPushButton::clicked, this, &ModernMainWindow::onToggleAudio);
    
    controlLayout->addWidget(cameraButton_);
    controlLayout->addWidget(audioButton_);
    controlLayout->addStretch();
    
    localLayout->addLayout(controlLayout);
    layout->addWidget(localVideoGroup);
}

void ModernMainWindow::setupChatWidget()
{
    chatWidget_ = new QWidget();
    auto *layout = new QVBoxLayout(chatWidget_);
    layout->setContentsMargins(10, 10, 10, 10);
    
    auto *chatGroup = new QGroupBox("实时聊天");
    auto *chatLayout = new QVBoxLayout(chatGroup);
    
    // 聊天显示区域
    chatDisplay_ = new QTextEdit();
    chatDisplay_->setReadOnly(true);
    chatDisplay_->setMinimumHeight(300);
    chatLayout->addWidget(chatDisplay_, 1);
    
    // 消息输入区域
    auto *inputLayout = new QHBoxLayout();
    messageInput_ = new QLineEdit();
    messageInput_->setPlaceholderText("输入消息...");
    sendButton_ = new QPushButton("发送");
    
    connect(messageInput_, &QLineEdit::returnPressed, this, &ModernMainWindow::onSendMessage);
    connect(sendButton_, &QPushButton::clicked, this, &ModernMainWindow::onSendMessage);
    
    inputLayout->addWidget(messageInput_);
    inputLayout->addWidget(sendButton_);
    
    chatLayout->addLayout(inputLayout);
    layout->addWidget(chatGroup);
}

void ModernMainWindow::setupWorkOrderPage()
{
    workOrderPage_ = new QWidget();
    auto *layout = new QVBoxLayout(workOrderPage_);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    // 页面标题
    auto *titleLabel = new QLabel("工单管理");
    titleLabel->setProperty("class", "title");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db; margin-bottom: 20px;");
    layout->addWidget(titleLabel);
    
    // 工单列表
    auto *tableGroup = new QGroupBox("可用工单");
    auto *tableLayout = new QVBoxLayout(tableGroup);
    
    workOrderTable_ = new QTableWidget(0, 5);
    workOrderTable_->setHorizontalHeaderLabels({"工单号", "标题", "创建时间", "状态", "操作"});
    workOrderTable_->horizontalHeader()->setStretchLastSection(true);
    workOrderTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    workOrderTable_->setAlternatingRowColors(true);
    
    tableLayout->addWidget(workOrderTable_);
    layout->addWidget(tableGroup, 1);
    
    stackedWidget_->addWidget(workOrderPage_);
}

void ModernMainWindow::setupSettingsPage()
{
    settingsPage_ = new QWidget();
    auto *layout = new QVBoxLayout(settingsPage_);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);
    
    // 页面标题
    auto *titleLabel = new QLabel("系统设置");
    titleLabel->setProperty("class", "title");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #3498db; margin-bottom: 20px;");
    layout->addWidget(titleLabel);
    
    // 设置选项
    auto *connectionGroup = new QGroupBox("连接设置");
    auto *connectionLayout = new QFormLayout(connectionGroup);
    
    auto *serverEdit = new QLineEdit("127.0.0.1");
    auto *portEdit = new QLineEdit("9000");
    
    connectionLayout->addRow("服务器地址:", serverEdit);
    connectionLayout->addRow("端口:", portEdit);
    
    layout->addWidget(connectionGroup);
    
    auto *videoGroup = new QGroupBox("视频设置");
    auto *videoLayout = new QFormLayout(videoGroup);
    
    auto *qualityCombo = new QComboBox();
    qualityCombo->addItems({"高质量", "中等质量", "低质量"});
    qualityCombo->setCurrentIndex(1);
    
    videoLayout->addRow("视频质量:", qualityCombo);
    
    layout->addWidget(videoGroup);
    layout->addStretch();
    
    stackedWidget_->addWidget(settingsPage_);
}

void ModernMainWindow::onNavigationChanged(const QString &page)
{
    switchToPage(page);
}

void ModernMainWindow::switchToPage(const QString &pageName)
{
    sidebar_->setActiveItem(pageName);
    
    if (pageName == "dashboard") {
        stackedWidget_->setCurrentWidget(dashboardPage_);
    } else if (pageName == "communication") {
        stackedWidget_->setCurrentWidget(communicationPage_);
    } else if (pageName == "workorders") {
        stackedWidget_->setCurrentWidget(workOrderPage_);
    } else if (pageName == "settings") {
        stackedWidget_->setCurrentWidget(settingsPage_);
    }
}

void ModernMainWindow::onConnected()
{
    isConnected_ = true;
    updateConnectionStatus(true);
    
    if (dashboard_) {
        dashboard_->updateConnectionStatus(true);
    }
    
    chatDisplay_->append("<b>系统:</b> 已连接到服务器");
}

void ModernMainWindow::onDisconnected()
{
    isConnected_ = false;
    isInRoom_ = false;
    updateConnectionStatus(false);
    
    if (dashboard_) {
        dashboard_->updateConnectionStatus(false);
        dashboard_->updateActiveRoom("");
        dashboard_->updateParticipantCount(0);
    }
    
    chatDisplay_->append("<b>系统:</b> 与服务器连接断开");
}

void ModernMainWindow::onPacketReceived(Packet pkt)
{
    // 处理接收到的数据包
    if (pkt.type == MSG_TEXT) {
        QString sender = pkt.json.value("sender").toString();
        QString message = pkt.json.value("message").toString();
        chatDisplay_->append(QString("<b>%1:</b> %2").arg(sender).arg(message));
    }
    // TODO: 处理其他类型的数据包
}

void ModernMainWindow::onJoinRoom()
{
    QString roomCode = roomCodeInput_->text().trimmed();
    if (roomCode.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入房间代码");
        return;
    }
    
    if (!isConnected_) {
        QMessageBox::warning(this, "警告", "请先连接到服务器");
        return;
    }
    
    // 发送加入房间请求
    QJsonObject joinData{
        {"roomId", roomCode},
        {"user", username_.isEmpty() ? "Expert" : username_}
    };
    conn_.send(MSG_JOIN_WORKORDER, joinData);
    
    currentRoom_ = roomCode;
    isInRoom_ = true;
    
    roomStatusLabel_->setText(QString("房间: %1").arg(roomCode));
    roomStatusLabel_->setStyleSheet("color: #27ae60;");
    
    if (dashboard_) {
        dashboard_->updateActiveRoom(roomCode);
    }
    
    chatDisplay_->append(QString("<b>系统:</b> 正在加入房间 %1...").arg(roomCode));
}

void ModernMainWindow::onSendMessage()
{
    QString message = messageInput_->text().trimmed();
    if (message.isEmpty() || !isInRoom_) {
        return;
    }
    
    QJsonObject messageData{
        {"roomId", currentRoom_},
        {"sender", username_.isEmpty() ? "Expert" : username_},
        {"message", message}
    };
    conn_.send(MSG_TEXT, messageData);
    
    chatDisplay_->append(QString("<b>我:</b> %1").arg(message));
    messageInput_->clear();
}

void ModernMainWindow::onToggleCamera()
{
    isCameraOn_ = !isCameraOn_;
    cameraButton_->setText(isCameraOn_ ? "关闭摄像头" : "开启摄像头");
    
    if (isCameraOn_) {
        localVideoLabel_->setText("摄像头已开启");
        localVideoLabel_->setStyleSheet(R"(
            QLabel {
                border: 2px solid #27ae60;
                border-radius: 8px;
                background-color: #2c3e50;
                color: #27ae60;
            }
        )");
    } else {
        localVideoLabel_->setText("本地视频预览");
        localVideoLabel_->setStyleSheet(R"(
            QLabel {
                border: 2px solid #3498db;
                border-radius: 8px;
                background-color: #2c3e50;
                color: #95a5a6;
            }
        )");
    }
}

void ModernMainWindow::onToggleAudio()
{
    isAudioOn_ = !isAudioOn_;
    audioButton_->setText(isAudioOn_ ? "关闭麦克风" : "开启麦克风");
    audioButton_->setStyleSheet(isAudioOn_ ? 
        "background-color: #27ae60;" : 
        "background-color: #3498db;");
}

void ModernMainWindow::updateConnectionStatus(bool connected)
{
    if (connected) {
        connectionStatusLabel_->setText("已连接");
        connectionStatusLabel_->setStyleSheet("color: #27ae60;");
    } else {
        connectionStatusLabel_->setText("未连接");
        connectionStatusLabel_->setStyleSheet("color: #e74c3c;");
        
        roomStatusLabel_->setText("未加入房间");
        roomStatusLabel_->setStyleSheet("color: #95a5a6;");
    }
}

void ModernMainWindow::loadTheme()
{
    // 应用深色主题
    QFile themeFile(":/styles/theme-dark.qss");
    if (themeFile.open(QFile::ReadOnly)) {
        QString theme = QLatin1String(themeFile.readAll());
        setStyleSheet(theme);
    }
}