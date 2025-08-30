#include "factorymain.h"
#include <QApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QCameraInfo>
#include <QBuffer>
#include <QVideoFrame>

FactoryMain::FactoryMain(ClientConn* conn, QWidget *parent)
    : QMainWindow(parent)
    , conn_(conn)
    , camera_(nullptr)
    , probe_(nullptr)
    , isInRoom_(false)
    , cameraActive_(false)
{
    setupUI();
    
    // Connect signals
    connect(conn_, &ClientConn::packetArrived, this, &FactoryMain::onPkt);
    connect(btnCreateWorkOrder, &QPushButton::clicked, this, &FactoryMain::onCreateWorkOrder);
    connect(btnJoinRoom, &QPushButton::clicked, this, &FactoryMain::onJoinWorkOrder);
    connect(btnSendMessage, &QPushButton::clicked, this, &FactoryMain::onSendMessage);
    connect(btnCamera, &QPushButton::clicked, this, &FactoryMain::onToggleCamera);
}

void FactoryMain::setupUI()
{
    setWindowTitle("Factory Main - Request Assistance");
    setMinimumSize(900, 700);
    
    // Apply modern styling similar to ExpertMain
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f8f9fa;
        }
        QTabWidget::pane {
            border: 1px solid #dee2e6;
            background-color: white;
        }
        QTabBar::tab {
            background-color: #e9ecef;
            border: 1px solid #dee2e6;
            padding: 8px 16px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: white;
            border-bottom: 1px solid white;
        }
        QPushButton {
            background-color: #28a745;
            border: none;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #218838;
        }
        QPushButton:disabled {
            background-color: #6c757d;
        }
        QLineEdit, QTextEdit {
            border: 1px solid #ced4da;
            border-radius: 4px;
            padding: 8px;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #28a745;
        }
        QLabel {
            color: #495057;
        }
    )");
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    tabWidget = new QTabWidget(centralWidget);
    
    // Work Order Creation Tab
    workOrderTab = new QWidget();
    setupWorkOrderTab();
    tabWidget->addTab(workOrderTab, "Create Work Order");
    
    // Communication Tab  
    commTab = new QWidget();
    setupCommunicationTab();
    tabWidget->addTab(commTab, "Communication");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(tabWidget);
}

void FactoryMain::setupWorkOrderTab()
{
    QVBoxLayout *layout = new QVBoxLayout(workOrderTab);
    layout->setSpacing(20);
    layout->setMargin(20);
    
    // Header
    QLabel *headerLabel = new QLabel("Create New Work Order");
    headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(headerLabel);
    
    QLabel *descLabel = new QLabel("Request remote assistance by creating a work order. An expert will be able to join and help you.");
    descLabel->setStyleSheet("color: #6c757d; margin-bottom: 20px;");
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    // Title field
    QLabel *titleLabel = new QLabel("Work Order Title:");
    titleLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(titleLabel);
    
    edWorkOrderTitle = new QLineEdit();
    edWorkOrderTitle->setPlaceholderText("Brief description of the issue (e.g., 'Machine calibration needed')");
    layout->addWidget(edWorkOrderTitle);
    
    // Description field
    QLabel *descriptionLabel = new QLabel("Detailed Description:");
    descriptionLabel->setStyleSheet("font-weight: bold; margin-top: 15px;");
    layout->addWidget(descriptionLabel);
    
    edWorkOrderDescription = new QTextEdit();
    edWorkOrderDescription->setPlaceholderText("Provide detailed information about the problem, what you've tried, and what kind of assistance you need...");
    edWorkOrderDescription->setMaximumHeight(150);
    layout->addWidget(edWorkOrderDescription);
    
    // Create button
    btnCreateWorkOrder = new QPushButton("Create Work Order & Request Assistance");
    btnCreateWorkOrder->setMinimumHeight(40);
    btnCreateWorkOrder->setStyleSheet("font-size: 14px;");
    layout->addWidget(btnCreateWorkOrder);
    
    layout->addStretch();
}

void FactoryMain::setupCommunicationTab()
{
    QVBoxLayout *layout = new QVBoxLayout(commTab);
    layout->setSpacing(15);
    layout->setMargin(20);
    
    // Room connection section
    QLabel *roomLabel = new QLabel("Join Work Order Room:");
    roomLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(roomLabel);
    
    QHBoxLayout *roomLayout = new QHBoxLayout();
    edRoomId = new QLineEdit();
    edRoomId->setPlaceholderText("Enter Work Order ID");
    btnJoinRoom = new QPushButton("Join Room");
    roomLayout->addWidget(new QLabel("Room ID:"));
    roomLayout->addWidget(edRoomId);
    roomLayout->addWidget(btnJoinRoom);
    layout->addLayout(roomLayout);
    
    // Current room info
    currentRoomLabel = new QLabel("Not connected to any work order");
    currentRoomLabel->setStyleSheet("color: #6c757d; font-style: italic;");
    layout->addWidget(currentRoomLabel);
    
    // Video section
    QLabel *videoLabel2 = new QLabel("Local Video Feed:");
    videoLabel2->setStyleSheet("font-weight: bold; margin-top: 15px;");
    layout->addWidget(videoLabel2);
    
    QHBoxLayout *cameraControlLayout = new QHBoxLayout();
    btnCamera = new QPushButton("Start Camera");
    chkAutoStart = new QCheckBox("Auto-start camera");
    cameraControlLayout->addWidget(btnCamera);
    cameraControlLayout->addWidget(chkAutoStart);
    cameraControlLayout->addStretch();
    layout->addLayout(cameraControlLayout);
    
    videoLabel = new QLabel("Camera not active");
    videoLabel->setMinimumSize(320, 240);
    videoLabel->setMaximumSize(640, 480);
    videoLabel->setStyleSheet("border: 2px dashed #ced4da; background-color: #f8f9fa;");
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setScaledContents(true);
    layout->addWidget(videoLabel);
    
    // Chat section
    QLabel *chatLabel = new QLabel("Communication:");
    chatLabel->setStyleSheet("font-weight: bold; margin-top: 15px;");
    layout->addWidget(chatLabel);
    
    txtChat = new QTextEdit();
    txtChat->setReadOnly(true);
    txtChat->setMaximumHeight(150);
    layout->addWidget(txtChat);
    
    QHBoxLayout *messageLayout = new QHBoxLayout();
    edMessage = new QLineEdit();
    edMessage->setPlaceholderText("Type your message here...");
    btnSendMessage = new QPushButton("Send");
    btnSendMessage->setEnabled(false);
    
    messageLayout->addWidget(edMessage);
    messageLayout->addWidget(btnSendMessage);
    layout->addLayout(messageLayout);
}

void FactoryMain::onCreateWorkOrder()
{
    QString title = edWorkOrderTitle->text().trimmed();
    QString description = edWorkOrderDescription->toPlainText().trimmed();
    
    if (title.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a title for the work order");
        return;
    }
    
    if (description.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a description for the work order");
        return;
    }
    
    QJsonObject workOrderData{
        {"title", title},
        {"description", description}
    };
    conn_->send(MSG_CREATE_WORKORDER, workOrderData);
    
    // Clear fields
    edWorkOrderTitle->clear();
    edWorkOrderDescription->clear();
    
    // Switch to communication tab
    tabWidget->setCurrentIndex(1);
    
    QMessageBox::information(this, "Work Order Created", 
                           "Work order has been created. You can now join the room to communicate with experts.");
}

void FactoryMain::onJoinWorkOrder()
{
    QString roomId = edRoomId->text().trimmed();
    if (roomId.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a room ID");
        return;
    }
    
    QJsonObject joinData{
        {"roomId", roomId},
        {"user", "factory"}
    };
    conn_->send(MSG_JOIN_WORKORDER, joinData);
    
    currentRoom_ = roomId;
    currentRoomLabel->setText(QString("Connected to Work Order: %1").arg(roomId));
    btnSendMessage->setEnabled(true);
    isInRoom_ = true;
}

void FactoryMain::onSendMessage()
{
    QString message = edMessage->text().trimmed();
    if (message.isEmpty() || !isInRoom_) {
        return;
    }
    
    QJsonObject messageData{
        {"roomId", currentRoom_},
        {"message", message},
        {"sender", "factory"},
        {"timestamp", QDateTime::currentMSecsSinceEpoch()}
    };
    conn_->send(MSG_TEXT, messageData);
    
    // Add to chat display
    txtChat->append(QString("[%1] Factory: %2")
                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                   .arg(message));
    
    edMessage->clear();
}

void FactoryMain::onToggleCamera()
{
    if (cameraActive_) {
        stopCamera();
    } else {
        startCamera();
    }
}

void FactoryMain::startCamera()
{
    if (camera_) {
        return; // Already started
    }
    
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
        QMessageBox::warning(this, "Camera Error", "No cameras found");
        return;
    }
    
    camera_ = new QCamera(cameras.first(), this);
    probe_ = new QVideoProbe(this);
    
    connect(probe_, &QVideoProbe::videoFrameProbed, this, &FactoryMain::onVideoFrame);
    probe_->setSource(camera_);
    
    camera_->start();
    cameraActive_ = true;
    btnCamera->setText("Stop Camera");
    videoLabel->setText("Camera starting...");
}

void FactoryMain::stopCamera()
{
    if (camera_) {
        camera_->stop();
        camera_->deleteLater();
        camera_ = nullptr;
    }
    
    if (probe_) {
        probe_->deleteLater();
        probe_ = nullptr;
    }
    
    cameraActive_ = false;
    btnCamera->setText("Start Camera");
    videoLabel->setText("Camera not active");
}

void FactoryMain::onVideoFrame(const QVideoFrame &frame)
{
    if (!camera_ || !frame.isValid() || !isInRoom_) {
        return;
    }
    
    QVideoFrame clone(frame);
    if (!clone.map(QAbstractVideoBuffer::ReadOnly)) {
        return;
    }
    
    // Convert frame to QImage and then to JPEG
    QImage img;
    QVideoFrame::PixelFormat pixelFormat = clone.pixelFormat();
    
    if (pixelFormat == QVideoFrame::Format_ARGB32 ||
        pixelFormat == QVideoFrame::Format_ARGB32_Premultiplied) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_ARGB32);
    } else if (pixelFormat == QVideoFrame::Format_RGB32) {
        img = QImage(clone.bits(), clone.width(), clone.height(),
                     clone.bytesPerLine(), QImage::Format_RGB32);
    }
    
    clone.unmap();
    
    if (img.isNull()) {
        return;
    }
    
    // Display locally
    QPixmap pixmap = QPixmap::fromImage(img.scaled(videoLabel->size(), Qt::KeepAspectRatio));
    videoLabel->setPixmap(pixmap);
    
    // Send to server
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    if (img.save(&buffer, "JPEG", 60)) {
        QJsonObject frameData{
            {"roomId", currentRoom_},
            {"sender", "factory"},
            {"ts", QDateTime::currentMSecsSinceEpoch()},
            {"width", img.width()},
            {"height", img.height()}
        };
        conn_->send(MSG_VIDEO_FRAME, frameData, jpeg);
    }
}

void FactoryMain::onPkt(Packet p)
{
    if (p.type == MSG_SERVER_EVENT) {
        int code = p.json.value("code").toInt();
        QString message = p.json.value("message").toString();
        
        if (code == 0) {
            if (message.contains("work order created")) {
                QString workOrderId = p.json.value("workOrderId").toString();
                if (!workOrderId.isEmpty()) {
                    edRoomId->setText(workOrderId);
                    QMessageBox::information(this, "Success", 
                                           QString("Work order created with ID: %1").arg(workOrderId));
                }
            } else if (message.contains("joined")) {
                txtChat->append("Successfully joined work order room");
            }
        } else {
            QMessageBox::warning(this, "Error", message);
        }
    } else if (p.type == MSG_TEXT) {
        QString roomId = p.json.value("roomId").toString();
        QString message = p.json.value("message").toString();
        QString sender = p.json.value("sender").toString();
        qint64 timestamp = p.json.value("timestamp").toVariant().toLongLong();
        
        if (roomId == currentRoom_) {
            QDateTime time = QDateTime::fromMSecsSinceEpoch(timestamp);
            txtChat->append(QString("[%1] %2: %3")
                           .arg(time.toString("hh:mm:ss"))
                           .arg(sender)
                           .arg(message));
        }
    }
}