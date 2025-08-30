#include "mainwindow.h"
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

// 假设这些宏和类在其他地方定义
// #define MSG_JOIN_WORKORDER 100
// #define MSG_TEXT 101
// #define MSG_VIDEO_FRAME 102
// class Packet { ... };
// class ClientConn { ... };

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , camera_(nullptr)
    , probe_(nullptr)
    , settings_("irexp", "client-factory") // 使用指定的组织和应用名
    , isConnected_(false)
    , isJoinedRoom_(false)
{
    QWidget *w  = new QWidget(this);
    QVBoxLayout *lay = new QVBoxLayout(w);

    /* 连接行 */
    QHBoxLayout *row1 = new QHBoxLayout;
    edHost = new QLineEdit("127.0.0.1");
    edPort = new QLineEdit("9000");
    edPort->setMaximumWidth(80);
    QPushButton *btnConn = new QPushButton("连接");
    row1->addWidget(new QLabel("Host:")); row1->addWidget(edHost);
    row1->addWidget(new QLabel("Port:")); row1->addWidget(edPort);
    row1->addWidget(btnConn);
    lay->addLayout(row1);

    /* 加入房间行 */
    QHBoxLayout *row2 = new QHBoxLayout;
    edUser = new QLineEdit("client-A");
    edRoom = new QLineEdit("R123");
    QPushButton *btnJoin = new QPushButton("加入工单");
    row2->addWidget(new QLabel("User:"));  row2->addWidget(edUser);
    row2->addWidget(new QLabel("Room:"));  row2->addWidget(edRoom);
    row2->addWidget(btnJoin);
    lay->addLayout(row2);

    /* 日志 */
    txtLog = new QTextEdit; txtLog->setReadOnly(true);
    lay->addWidget(txtLog);

    /* 视频区 - 本地和远端视频并排显示 */
    QHBoxLayout *videoRow = new QHBoxLayout;
    
    // 本地视频预览 (左侧)
    QVBoxLayout *localVideoLayout = new QVBoxLayout;
    videoLabel_ = new QLabel("本地视频预览");
    videoLabel_->setFixedSize(320, 240);
    videoLabel_->setStyleSheet("border:1px solid black;");
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setScaledContents(true);
    QLabel *localLabel = new QLabel("Local Preview");
    localLabel->setAlignment(Qt::AlignCenter);
    localVideoLayout->addWidget(localLabel);
    localVideoLayout->addWidget(videoLabel_);
    
    // 远端视频显示 (右侧)
    QVBoxLayout *remoteVideoLayout = new QVBoxLayout;
    remoteLabel_ = new QLabel("远端视频");
    remoteLabel_->setFixedSize(320, 240);
    remoteLabel_->setStyleSheet("border:1px solid blue;");
    remoteLabel_->setAlignment(Qt::AlignCenter);
    remoteLabel_->setScaledContents(true);
    QLabel *remoteHeaderLabel = new QLabel("Remote Video");
    remoteHeaderLabel->setAlignment(Qt::AlignCenter);
    remoteVideoLayout->addWidget(remoteHeaderLabel);
    remoteVideoLayout->addWidget(remoteLabel_);
    
    videoRow->addLayout(localVideoLayout);
    videoRow->addLayout(remoteVideoLayout);
    lay->addLayout(videoRow);

    /* 摄像头开关 */
    btnCamera_ = new QPushButton("开启摄像头");
    lay->addWidget(btnCamera_);
    
    /* 自动启动摄像头选项 */
    chkAutoStart_ = new QCheckBox("Auto start camera after join");
    chkAutoStart_->setChecked(loadAutoStartPreference()); // 加载保存的偏好
    lay->addWidget(chkAutoStart_);

    /* 发送文本 */
    QHBoxLayout *row3 = new QHBoxLayout;
    edInput = new QLineEdit;
    QPushButton *btnSend = new QPushButton("发送文本");
    row3->addWidget(edInput); row3->addWidget(btnSend);
    lay->addLayout(row3);

    setCentralWidget(w);
    setWindowTitle("Client (含视频)");

    connect(btnConn,   &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(btnJoin,   &QPushButton::clicked, this, &MainWindow::onJoin);
    connect(btnSend,   &QPushButton::clicked, this, &MainWindow::onSendText);
    connect(btnCamera_,&QPushButton::clicked,this,&MainWindow::onToggleCamera);
    connect(chkAutoStart_, &QCheckBox::toggled, this, &MainWindow::onAutoStartToggled);
    connect(&conn_,    &ClientConn::packetArrived, this, &MainWindow::onPkt);
    connect(&conn_,    &ClientConn::connected, this, &MainWindow::onConnected);
    connect(&conn_,    &ClientConn::disconnected, this, &MainWindow::onDisconnected);
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
