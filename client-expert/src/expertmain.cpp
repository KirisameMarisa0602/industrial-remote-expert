#include "expertmain.h"
#include <QApplication>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>

ExpertMain::ExpertMain(ClientConn* conn, QWidget *parent)
    : QMainWindow(parent)
    , conn_(conn)
    , isInRoom_(false)
{
    setupUI();
    
    // Connect signals
    connect(conn_, &ClientConn::packetArrived, this, &ExpertMain::onPkt);
    connect(btnRefreshWorkOrders, &QPushButton::clicked, this, &ExpertMain::onRefreshWorkOrders);
    connect(btnJoinSelected, &QPushButton::clicked, this, &ExpertMain::onJoinWorkOrder);
    connect(btnSendMessage, &QPushButton::clicked, this, &ExpertMain::onSendMessage);
    connect(workOrdersTable, &QTableWidget::itemSelectionChanged, this, &ExpertMain::onWorkOrderSelected);
    
    // Initial refresh
    onRefreshWorkOrders();
}

void ExpertMain::setupUI()
{
    setWindowTitle("Expert Main - Remote Assistance");
    setMinimumSize(900, 700);
    
    // Apply modern styling
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
        QTableWidget {
            gridline-color: #dee2e6;
            background-color: white;
            alternate-background-color: #f8f9fa;
        }
        QTableWidget::item {
            padding: 8px;
        }
        QTableWidget::item:selected {
            background-color: #007bff;
            color: white;
        }
        QPushButton {
            background-color: #007bff;
            border: none;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #0056b3;
        }
        QPushButton:disabled {
            background-color: #6c757d;
        }
        QLineEdit {
            border: 1px solid #ced4da;
            border-radius: 4px;
            padding: 8px;
        }
        QLineEdit:focus {
            border-color: #007bff;
            box-shadow: 0 0 0 0.2rem rgba(0,123,255,.25);
        }
        QTextEdit {
            border: 1px solid #ced4da;
            border-radius: 4px;
            background-color: white;
        }
        QLabel {
            color: #495057;
        }
    )");
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    tabWidget = new QTabWidget(centralWidget);
    
    // Work Orders Tab
    workOrdersTab = new QWidget();
    setupWorkOrdersTab();
    tabWidget->addTab(workOrdersTab, "Work Orders");
    
    // Communication Tab  
    commTab = new QWidget();
    setupCommunicationTab();
    tabWidget->addTab(commTab, "Communication");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(tabWidget);
}

void ExpertMain::setupWorkOrdersTab()
{
    QVBoxLayout *layout = new QVBoxLayout(workOrdersTab);
    
    // Header
    QLabel *headerLabel = new QLabel("Available Work Orders");
    headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px;");
    layout->addWidget(headerLabel);
    
    // Controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    btnRefreshWorkOrders = new QPushButton("Refresh");
    btnJoinSelected = new QPushButton("Join Selected Work Order");
    btnJoinSelected->setEnabled(false);
    
    controlsLayout->addWidget(btnRefreshWorkOrders);
    controlsLayout->addStretch();
    controlsLayout->addWidget(btnJoinSelected);
    layout->addLayout(controlsLayout);
    
    // Work Orders Table
    workOrdersTable = new QTableWidget(0, 5);
    QStringList headers = {"ID", "Title", "Description", "Status", "Created"};
    workOrdersTable->setHorizontalHeaderLabels(headers);
    workOrdersTable->setAlternatingRowColors(true);
    workOrdersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    workOrdersTable->horizontalHeader()->setStretchLastSection(true);
    workOrdersTable->setColumnWidth(0, 50);
    workOrdersTable->setColumnWidth(1, 200);
    workOrdersTable->setColumnWidth(2, 300);
    workOrdersTable->setColumnWidth(3, 100);
    
    layout->addWidget(workOrdersTable);
}

void ExpertMain::setupCommunicationTab()
{
    QVBoxLayout *layout = new QVBoxLayout(commTab);
    
    // Current room info
    currentRoomLabel = new QLabel("Not connected to any work order");
    currentRoomLabel->setStyleSheet("font-weight: bold; color: #6c757d; margin-bottom: 10px;");
    layout->addWidget(currentRoomLabel);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // Left side - Chat
    QWidget *chatWidget = new QWidget();
    QVBoxLayout *chatLayout = new QVBoxLayout(chatWidget);
    
    QLabel *chatLabel = new QLabel("Communication");
    chatLabel->setStyleSheet("font-weight: bold;");
    chatLayout->addWidget(chatLabel);
    
    txtChat = new QTextEdit();
    txtChat->setReadOnly(true);
    txtChat->setMaximumHeight(300);
    chatLayout->addWidget(txtChat);
    
    QHBoxLayout *messageLayout = new QHBoxLayout();
    edMessage = new QLineEdit();
    edMessage->setPlaceholderText("Type your message here...");
    btnSendMessage = new QPushButton("Send");
    btnSendMessage->setEnabled(false);
    
    messageLayout->addWidget(edMessage);
    messageLayout->addWidget(btnSendMessage);
    chatLayout->addLayout(messageLayout);
    
    splitter->addWidget(chatWidget);
    
    // Right side - Video
    QWidget *videoWidget = new QWidget();
    QVBoxLayout *videoLayout = new QVBoxLayout(videoWidget);
    
    QLabel *videoHeaderLabel = new QLabel("Remote Video Feed");
    videoHeaderLabel->setStyleSheet("font-weight: bold;");
    videoLayout->addWidget(videoHeaderLabel);
    
    videoLabel = new QLabel("No video feed");
    videoLabel->setMinimumSize(320, 240);
    videoLabel->setStyleSheet("border: 2px dashed #ced4da; background-color: #f8f9fa;");
    videoLabel->setAlignment(Qt::AlignCenter);
    videoLabel->setScaledContents(true);
    videoLayout->addWidget(videoLabel);
    
    videoLayout->addStretch();
    
    splitter->addWidget(videoWidget);
    splitter->setSizes({400, 300});
    
    layout->addWidget(splitter);
}

void ExpertMain::onRefreshWorkOrders()
{
    // Request work orders list from server
    QJsonObject request;
    conn_->send(MSG_LIST_WORKORDERS, request);
}

void ExpertMain::onJoinWorkOrder()
{
    int currentRow = workOrdersTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Selection Error", "Please select a work order to join");
        return;
    }
    
    QString workOrderId = workOrdersTable->item(currentRow, 0)->text();
    QString title = workOrdersTable->item(currentRow, 1)->text();
    
    // Join the work order room
    QJsonObject joinData{
        {"roomId", workOrderId},
        {"user", "expert"}
    };
    conn_->send(MSG_JOIN_WORKORDER, joinData);
    
    currentRoom_ = workOrderId;
    currentRoomLabel->setText(QString("Connected to Work Order: %1 - %2").arg(workOrderId, title));
    btnSendMessage->setEnabled(true);
    isInRoom_ = true;
    
    // Switch to communication tab
    tabWidget->setCurrentIndex(1);
}

void ExpertMain::onSendMessage()
{
    QString message = edMessage->text().trimmed();
    if (message.isEmpty() || !isInRoom_) {
        return;
    }
    
    QJsonObject messageData{
        {"roomId", currentRoom_},
        {"message", message},
        {"sender", "expert"},
        {"timestamp", QDateTime::currentMSecsSinceEpoch()}
    };
    conn_->send(MSG_TEXT, messageData);
    
    // Add to chat display
    txtChat->append(QString("[%1] Expert: %2")
                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                   .arg(message));
    
    edMessage->clear();
}

void ExpertMain::onWorkOrderSelected()
{
    btnJoinSelected->setEnabled(workOrdersTable->currentRow() >= 0);
}

void ExpertMain::onPkt(Packet p)
{
    if (p.type == MSG_SERVER_EVENT) {
        int code = p.json.value("code").toInt();
        QString message = p.json.value("message").toString();
        
        if (code == 0 && message.contains("joined")) {
            txtChat->append("Successfully joined work order room");
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
    } else if (p.type == MSG_VIDEO_FRAME) {
        QString roomId = p.json.value("roomId").toString();
        if (roomId == currentRoom_ && !p.bin.isEmpty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(p.bin, "JPEG")) {
                videoLabel->setPixmap(pixmap);
            }
        }
    }
    // Handle work orders list response would go here
}