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
#include <QDockWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QSplitter>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>
#include <QRandomGenerator>

using namespace QtCharts;

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
    , settings_("irexp", "client-factory")
    , isConnected_(false)
    , isJoinedRoom_(false)
{
    // Apply modern dark theme
    setStyleSheet(ModernStyle::getDarkThemeStyleSheet());
    
    // Set window properties
    setWindowTitle("工厂客户端 - Industrial Remote Expert");
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
        
        if (currentUserRole_ == UserRole::Factory) {
            setupFactoryMainUI();
            statusBar()->showMessage(QString("已登录用户: %1 (工厂用户)").arg(authenticatedUsername_));
        } else {
            QMessageBox::warning(this, "角色错误", "工厂客户端只能使用工厂身份登录！");
            QApplication::quit();
        }
    } else {
        QApplication::quit();
    }
    
    loginDialog->deleteLater();
}

void MainWindow::setupFactoryMainUI()
{
    // Create central splitter with dashboard and camera preview
    auto* centralSplitter = new QSplitter(Qt::Horizontal);
    setCentralWidget(centralSplitter);
    
    // Create device data dashboard
    createDeviceDataDashboard();
    centralSplitter->addWidget(dashboardTabs_);
    
    // Create camera preview widget
    createCameraPreviewPanel();
    centralSplitter->addWidget(cameraPreviewDock_->widget());
    centralSplitter->setSizes({800, 600});
    
    // Create dockable panels
    createQuickActionsPanel();
    createChatPanel();
    
    // Initialize device simulation
    initializeDeviceSimulation();
    
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
    workOrderMenu->addAction("创建工单(&N)", this, &MainWindow::onCreateWorkOrder, QKeySequence("Ctrl+N"));
    workOrderMenu->addAction("加入工单(&J)", this, &MainWindow::onJoin, QKeySequence("Ctrl+J"));
    
    auto* videoMenu = menuBar()->addMenu("视频(&V)");
    videoMenu->addAction("开启/关闭摄像头(&T)", this, &MainWindow::onToggleCamera, QKeySequence("Ctrl+T"));
    videoMenu->addAction("开始/停止录制(&R)", [this]() {
        // TODO: Implement recording toggle
        if (recordingToggleBtn_) {
            recordingToggleBtn_->click();
        }
    });
    
    auto* helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于(&A)", [this]() {
        QMessageBox::about(this, "关于", "Industrial Remote Expert\n工厂客户端 v1.0");
    });
}

void MainWindow::createQuickActionsPanel()
{
    quickActionsDock_ = new QDockWidget("快速操作", this);
    quickActionsDock_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    
    auto* actionsWidget = new QWidget();
    auto* actionsLayout = new QVBoxLayout(actionsWidget);
    
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
    actionsLayout->addWidget(connectionGroup);
    
    // Work order actions
    auto* workOrderGroup = new QGroupBox("工单操作");
    auto* woLayout = new QVBoxLayout(workOrderGroup);
    
    createWorkOrderBtn_ = new QPushButton("创建新工单");
    createWorkOrderBtn_->setEnabled(false);
    createWorkOrderBtn_->setProperty("class", "primary");
    
    auto* joinLayout = new QHBoxLayout();
    edRoom = new QLineEdit("R123");
    edRoom->setPlaceholderText("输入工单号");
    joinWorkOrderBtn_ = new QPushButton("加入工单");
    joinWorkOrderBtn_->setEnabled(false);
    
    joinLayout->addWidget(edRoom);
    joinLayout->addWidget(joinWorkOrderBtn_);
    
    woLayout->addWidget(createWorkOrderBtn_);
    woLayout->addLayout(joinLayout);
    actionsLayout->addWidget(workOrderGroup);
    
    // Recording controls
    auto* recordingGroup = new QGroupBox("录制控制");
    auto* recordingLayout = new QVBoxLayout(recordingGroup);
    
    recordingToggleBtn_ = new QPushButton("开始录制");
    recordingToggleBtn_->setEnabled(false);
    recordingLayout->addWidget(recordingToggleBtn_);
    
    actionsLayout->addWidget(recordingGroup);
    actionsLayout->addStretch();
    
    quickActionsDock_->setWidget(actionsWidget);
    addDockWidget(Qt::LeftDockWidgetArea, quickActionsDock_);
    
    // Connect signals
    connect(btnConn, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(createWorkOrderBtn_, &QPushButton::clicked, this, &MainWindow::onCreateWorkOrder);
    connect(joinWorkOrderBtn_, &QPushButton::clicked, this, &MainWindow::onJoin);
    
    // Initialize edUser for compatibility
    edUser = new QLineEdit(authenticatedUsername_);
    edUser->setVisible(false);
}

void MainWindow::createDeviceDataDashboard()
{
    dashboardTabs_ = new QTabWidget();
    
    // Overview tab with KPI cards
    auto* overviewTab = new QWidget();
    auto* overviewLayout = new QGridLayout(overviewTab);
    
    // KPI Cards
    auto createKPICard = [](const QString& title, const QString& value, const QString& unit, const QString& status) {
        auto* card = new QGroupBox(title);
        card->setStyleSheet(R"(
            QGroupBox {
                font-weight: bold;
                border: 2px solid #555555;
                border-radius: 8px;
                margin-top: 10px;
                padding-top: 10px;
                background-color: #404040;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px 0 5px;
            }
        )");
        card->setFixedHeight(120);
        
        auto* cardLayout = new QVBoxLayout(card);
        auto* valueLabel = new QLabel(value + " " + unit);
        valueLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #0078d4;");
        valueLabel->setAlignment(Qt::AlignCenter);
        
        auto* statusLabel = new QLabel(status);
        statusLabel->setAlignment(Qt::AlignCenter);
        if (status.contains("正常")) {
            statusLabel->setStyleSheet("color: #28a745;");
        } else if (status.contains("警告")) {
            statusLabel->setStyleSheet("color: #ffc107;");
        } else {
            statusLabel->setStyleSheet("color: #dc3545;");
        }
        
        cardLayout->addWidget(valueLabel);
        cardLayout->addWidget(statusLabel);
        return card;
    };
    
    overviewLayout->addWidget(createKPICard("压力", "8.5", "bar", "正常"), 0, 0);
    overviewLayout->addWidget(createKPICard("温度", "75", "°C", "正常"), 0, 1);
    overviewLayout->addWidget(createKPICard("流量", "120", "L/min", "警告"), 0, 2);
    overviewLayout->addWidget(createKPICard("振动", "0.8", "mm/s", "正常"), 1, 0);
    overviewLayout->addWidget(createKPICard("转速", "1450", "RPM", "正常"), 1, 1);
    overviewLayout->addWidget(createKPICard("功率", "85", "kW", "正常"), 1, 2);
    
    // Add recent alerts section
    auto* alertsGroup = new QGroupBox("最近警报");
    auto* alertsLayout = new QVBoxLayout(alertsGroup);
    auto* alertsList = new QTextEdit();
    alertsList->setMaximumHeight(100);
    alertsList->setReadOnly(true);
    alertsList->append("⚠️ 流量传感器读数偏高 - 10:30");
    alertsList->append("✅ 压力传感器校准完成 - 09:15");
    alertsList->append("ℹ️ 系统维护计划已安排 - 昨天");
    alertsLayout->addWidget(alertsList);
    
    overviewLayout->addWidget(alertsGroup, 2, 0, 1, 3);
    overviewLayout->setRowStretch(2, 1);
    
    dashboardTabs_->addTab(overviewTab, "概览");
    
    // Pressure chart tab
    auto* pressureTab = new QWidget();
    auto* pressureLayout = new QVBoxLayout(pressureTab);
    
    pressureSeries_ = new QLineSeries();
    pressureSeries_->setName("压力 (bar)");
    pressureSeries_->setColor(QColor("#0078d4"));
    
    auto* pressureChart = new QChart();
    pressureChart->addSeries(pressureSeries_);
    pressureChart->setTitle("实时压力监控");
    pressureChart->setTheme(QChart::ChartThemeDark);
    
    auto* pressureAxisX = new QValueAxis();
    pressureAxisX->setTitleText("时间 (秒)");
    pressureAxisX->setRange(0, 60);
    pressureChart->addAxis(pressureAxisX, Qt::AlignBottom);
    pressureSeries_->attachAxis(pressureAxisX);
    
    auto* pressureAxisY = new QValueAxis();
    pressureAxisY->setTitleText("压力 (bar)");
    pressureAxisY->setRange(0, 15);
    pressureChart->addAxis(pressureAxisY, Qt::AlignLeft);
    pressureSeries_->attachAxis(pressureAxisY);
    
    pressureChart_ = new QChartView(pressureChart);
    pressureChart_->setRenderHint(QPainter::Antialiasing);
    pressureLayout->addWidget(pressureChart_);
    
    dashboardTabs_->addTab(pressureTab, "压力趋势");
    
    // Temperature chart tab
    auto* temperatureTab = new QWidget();
    auto* temperatureLayout = new QVBoxLayout(temperatureTab);
    
    temperatureSeries_ = new QLineSeries();
    temperatureSeries_->setName("温度 (°C)");
    temperatureSeries_->setColor(QColor("#dc3545"));
    
    auto* temperatureChart = new QChart();
    temperatureChart->addSeries(temperatureSeries_);
    temperatureChart->setTitle("实时温度监控");
    temperatureChart->setTheme(QChart::ChartThemeDark);
    
    auto* tempAxisX = new QValueAxis();
    tempAxisX->setTitleText("时间 (秒)");
    tempAxisX->setRange(0, 60);
    temperatureChart->addAxis(tempAxisX, Qt::AlignBottom);
    temperatureSeries_->attachAxis(tempAxisX);
    
    auto* tempAxisY = new QValueAxis();
    tempAxisY->setTitleText("温度 (°C)");
    tempAxisY->setRange(0, 100);
    temperatureChart->addAxis(tempAxisY, Qt::AlignLeft);
    temperatureSeries_->attachAxis(tempAxisY);
    
    temperatureChart_ = new QChartView(temperatureChart);
    temperatureChart_->setRenderHint(QPainter::Antialiasing);
    temperatureLayout->addWidget(temperatureChart_);
    
    dashboardTabs_->addTab(temperatureTab, "温度趋势");
}

void MainWindow::createCameraPreviewPanel()
{
    cameraPreviewDock_ = new QDockWidget("摄像头预览", this);
    auto* previewWidget = new QWidget();
    auto* previewLayout = new QVBoxLayout(previewWidget);
    
    videoLabel_ = new QLabel("摄像头未开启");
    videoLabel_->setMinimumSize(400, 300);
    videoLabel_->setStyleSheet(R"(
        QLabel {
            border: 2px solid #555555;
            border-radius: 8px;
            background-color: #404040;
            color: #ffffff;
        }
    )");
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setScaledContents(true);
    
    remoteLabel_ = new QLabel("远端视频");
    remoteLabel_->setMinimumSize(200, 150);
    remoteLabel_->setStyleSheet(videoLabel_->styleSheet());
    remoteLabel_->setAlignment(Qt::AlignCenter);
    remoteLabel_->setScaledContents(true);
    
    auto* controlsLayout = new QHBoxLayout();
    btnCamera_ = new QPushButton("开启摄像头");
    chkAutoStart_ = new QCheckBox("自动开启摄像头");
    chkAutoStart_->setChecked(loadAutoStartPreference());
    
    controlsLayout->addWidget(btnCamera_);
    controlsLayout->addWidget(chkAutoStart_);
    controlsLayout->addStretch();
    
    previewLayout->addWidget(new QLabel("本地视频:"));
    previewLayout->addWidget(videoLabel_);
    previewLayout->addWidget(new QLabel("远端视频:"));
    previewLayout->addWidget(remoteLabel_);
    previewLayout->addLayout(controlsLayout);
    
    cameraPreviewDock_->setWidget(previewWidget);
    
    connect(btnCamera_, &QPushButton::clicked, this, &MainWindow::onToggleCamera);
    connect(chkAutoStart_, &QCheckBox::toggled, this, &MainWindow::onAutoStartToggled);
}

void MainWindow::createChatPanel()
{
    chatDock_ = new QDockWidget("聊天", this);
    auto* chatWidget = new QWidget();
    auto* chatLayout = new QVBoxLayout(chatWidget);
    
    chatDisplay_ = new QTextEdit();
    chatDisplay_->setReadOnly(true);
    chatDisplay_->append("系统: 欢迎使用工厂客户端");
    txtLog = chatDisplay_; // Keep reference for compatibility
    
    chatInput_ = new QLineEdit();
    chatInput_->setPlaceholderText("输入消息...");
    edInput = chatInput_; // Keep reference for compatibility
    
    auto* sendButton = new QPushButton("发送");
    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(chatInput_);
    inputLayout->addWidget(sendButton);
    
    chatLayout->addWidget(chatDisplay_);
    chatLayout->addLayout(inputLayout);
    
    chatDock_->setWidget(chatWidget);
    addDockWidget(Qt::BottomDockWidgetArea, chatDock_);
    
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendText);
    connect(chatInput_, &QLineEdit::returnPressed, this, &MainWindow::onSendText);
}

void MainWindow::initializeDeviceSimulation()
{
    deviceSimulationTimer_ = new QTimer(this);
    connect(deviceSimulationTimer_, &QTimer::timeout, this, &MainWindow::updateDeviceData);
    deviceSimulationTimer_->start(1000); // Update every second
}

void MainWindow::updateDeviceData()
{
    static int timeCounter = 0;
    
    // Generate realistic sensor data with some variation
    double pressure = 8.5 + (QRandomGenerator::global()->bounded(200) - 100) / 100.0;
    double temperature = 75.0 + (QRandomGenerator::global()->bounded(100) - 50) / 10.0;
    
    // Add data to charts
    if (pressureSeries_ && temperatureSeries_) {
        pressureSeries_->append(timeCounter, pressure);
        temperatureSeries_->append(timeCounter, temperature);
        
        // Keep only last 60 points
        if (pressureSeries_->count() > 60) {
            pressureSeries_->removePoints(0, pressureSeries_->count() - 60);
            temperatureSeries_->removePoints(0, temperatureSeries_->count() - 60);
        }
        
        // Update chart axes
        if (pressureChart_ && temperatureChart_) {
            pressureChart_->chart()->axes(Qt::Horizontal).first()->setRange(
                qMax(0, timeCounter - 60), timeCounter);
            temperatureChart_->chart()->axes(Qt::Horizontal).first()->setRange(
                qMax(0, timeCounter - 60), timeCounter);
        }
    }
    
    // Send device data to server if connected and in a room
    if (conn_.isConnected() && isJoinedRoom_) {
        QJsonObject deviceData;
        deviceData["pressure"] = pressure;
        deviceData["temperature"] = temperature;
        deviceData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        
        QJsonObject packet;
        packet["roomId"] = currentRoom_;
        packet["sender"] = authenticatedUsername_;
        packet["data"] = deviceData;
        
        conn_.send(MSG_DEVICE_DATA, packet);
    }
    
    timeCounter++;
}

// Authentication methods
void MainWindow::onLoginSuccess(const QString& username, const QString& password, UserRole role)
{
    if (!isConnected_) {
        conn_.connectTo("127.0.0.1", 9000);
        authenticatedUsername_ = username;
        currentUserRole_ = role;
        return;
    }
    
    QString roleStr = (role == UserRole::Factory) ? "factory" : "expert";
    QJsonObject loginData{
        {"username", username}, 
        {"password", password},
        {"role", roleStr}
    };
    
    conn_.send(MSG_LOGIN, loginData);
    chatDisplay_->append(QString("正在登录: %1 (%2)").arg(username).arg(roleStr == "factory" ? "工厂用户" : "技术专家"));
}

void MainWindow::onRegisterSuccess(const QString& username, const QString& password, UserRole role)
{
    if (!isConnected_) {
        conn_.connectTo("127.0.0.1", 9000);
        authenticatedUsername_ = username;
        currentUserRole_ = role;
        return;
    }
    
    QString roleStr = (role == UserRole::Factory) ? "factory" : "expert";
    QJsonObject registerData{
        {"username", username}, 
        {"password", password},
        {"role", roleStr}
    };
    
    conn_.send(MSG_REGISTER, registerData);
    chatDisplay_->append(QString("正在注册: %1 (%2)").arg(username).arg(roleStr == "factory" ? "工厂用户" : "技术专家"));
}

void MainWindow::onCreateWorkOrder()
{
    if (!isConnected_) {
        QMessageBox::warning(this, "连接错误", "请先连接到服务器！");
        return;
    }
    
    // Generate a random work order code
    QString workOrderCode = QString("WO%1").arg(QRandomGenerator::global()->bounded(100000, 999999));
    
    QJsonObject createData;
    createData["title"] = QString("工单 - %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    createData["code"] = workOrderCode;
    createData["creator"] = authenticatedUsername_;
    
    conn_.send(MSG_CREATE_WORKORDER, createData);
    chatDisplay_->append(QString("正在创建工单: %1").arg(workOrderCode));
    
    // Auto-fill the room field with the created work order code
    edRoom->setText(workOrderCode);
}

/* ---------- 网络 ---------- */
void MainWindow::onConnect()
{
    conn_.connectTo(edHost->text(), edPort->text().toUShort());
    txtLog->append("Connecting...");
}
void MainWindow::onJoin()
{
    QJsonObject j{{"roomId", edRoom->text()}, {"user", edUser->text()}};
    conn_.send(MSG_JOIN_WORKORDER, j);
    currentRoom_ = edRoom->text(); // 记录尝试加入的房间
}
void MainWindow::onSendText()
{
    QJsonObject j{{"roomId",  edRoom->text()},
                  {"sender",  edUser->text()},
                  {"content", edInput->text()},
                  {"ts",      QDateTime::currentMSecsSinceEpoch()}};
    txtLog->append(QString("[%1] %2: %3")
                   .arg(edRoom->text(), edUser->text(), edInput->text()));
    conn_.send(MSG_TEXT, j);
    edInput->clear();
}
void MainWindow::onPkt(Packet p)
{
    switch (p.type)
    {
    case MSG_TEXT:
        txtLog->append(QString("[%1] %2: %3")
                       .arg(p.json["roomId"].toString(),
                            p.json["sender"].toString(),
                            p.json["content"].toString()));
        break;
    case MSG_VIDEO_FRAME:
    {
        QString sender = p.json["sender"].toString();
        QString roomId = p.json["roomId"].toString();
        
        // 只显示来自其他用户且在当前房间的视频
        if (sender != edUser->text() && roomId == currentRoom_ && isJoinedRoom_) {
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
        txtLog->append(QString("[server] %1")
                       .arg(QString::fromUtf8(QJsonDocument(p.json).toJson())));
        
        // 检查是否是房间加入成功的响应
        if (p.json.contains("code") && p.json["code"].toInt() == 0 && 
            p.json.contains("message") && p.json["message"].toString() == "joined") {
            isJoinedRoom_ = true;
            txtLog->append(QString("成功加入房间: %1").arg(currentRoom_));
            
            // 尝试自动启动摄像头
            tryAutoStartCamera();
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

    QJsonObject j{{"roomId", edRoom->text()},
                  {"sender", edUser->text()},
                  {"ts",     QDateTime::currentMSecsSinceEpoch()}};
    conn_.send(MSG_VIDEO_FRAME, j, jpeg);
}

/* ---------- 连接状态处理 ---------- */
void MainWindow::onConnected()
{
    isConnected_ = true;
    txtLog->append("已连接到服务器");
}

void MainWindow::onDisconnected()
{
    isConnected_ = false;
    isJoinedRoom_ = false;
    currentRoom_.clear();
    txtLog->append("与服务器断开连接");
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
