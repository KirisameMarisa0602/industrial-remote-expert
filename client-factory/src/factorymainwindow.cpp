#include "factorymainwindow.h"
#include "clientconn.h"
#include <QApplication>
#include <QSplitter>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFormLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QProgressBar>
#include <QTimer>
#include <QRandomGenerator>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>

using namespace QtCharts;

FactoryMainWindow::FactoryMainWindow(const QString& username, const QString& role, QWidget *parent)
    : QMainWindow(parent)
    , username_(username)
    , role_(role)
    , connection_(nullptr)
{
    setupUI();
    setWindowTitle(QString("Industrial Remote Expert - 工厂端 (%1)").arg(username_));
    resize(1200, 800);
    
    // Start dashboard refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &FactoryMainWindow::updateDashboardData);
    refreshTimer_->start(2000); // Update every 2 seconds
}

void FactoryMainWindow::setupUI()
{
    setupToolbar();
    setupStatusBar();
    
    // Create main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal);
    setCentralWidget(mainSplitter_);
    
    setupNavigation();
    setupViews();
}

void FactoryMainWindow::setupNavigation()
{
    navigationList_ = new QListWidget();
    navigationList_->setMaximumWidth(200);
    navigationList_->setMinimumWidth(180);
    
    // Add navigation items with icons (future enhancement)
    QListWidgetItem* dashboardItem = new QListWidgetItem("🏭 仪表盘", navigationList_);
    dashboardItem->setData(Qt::UserRole, 0);
    
    QListWidgetItem* ticketsItem = new QListWidgetItem("📋 工单管理", navigationList_);
    ticketsItem->setData(Qt::UserRole, 1);
    
    QListWidgetItem* meetingItem = new QListWidgetItem("📹 远程会议", navigationList_);
    meetingItem->setData(Qt::UserRole, 2);
    
    QListWidgetItem* telemetryItem = new QListWidgetItem("📊 设备监控", navigationList_);
    telemetryItem->setData(Qt::UserRole, 3);
    
    QListWidgetItem* chatItem = new QListWidgetItem("💬 消息中心", navigationList_);
    chatItem->setData(Qt::UserRole, 4);
    
    QListWidgetItem* knowledgeItem = new QListWidgetItem("📚 知识库", navigationList_);
    knowledgeItem->setData(Qt::UserRole, 5);
    
    navigationList_->setCurrentRow(0); // Default to dashboard
    
    connect(navigationList_, &QListWidget::currentRowChanged, 
            this, &FactoryMainWindow::onNavigationChanged);
    
    mainSplitter_->addWidget(navigationList_);
}

void FactoryMainWindow::setupViews()
{
    contentStack_ = new QStackedWidget();
    
    // Create all views
    dashboardView_ = createDashboardView();
    ticketsView_ = createTicketsView();
    meetingView_ = createMeetingView();
    telemetryView_ = createTelemetryView();
    chatView_ = createChatView();
    knowledgeBaseView_ = createKnowledgeBaseView();
    
    // Add to stack
    contentStack_->addWidget(dashboardView_);
    contentStack_->addWidget(ticketsView_);
    contentStack_->addWidget(meetingView_);
    contentStack_->addWidget(telemetryView_);
    contentStack_->addWidget(chatView_);
    contentStack_->addWidget(knowledgeBaseView_);
    
    mainSplitter_->addWidget(contentStack_);
    mainSplitter_->setStretchFactor(1, 1); // Content area gets most space
}

void FactoryMainWindow::setupToolbar()
{
    toolbar_ = addToolBar("主工具栏");
    
    QPushButton* createTicketBtn = new QPushButton("创建工单");
    createTicketBtn->setProperty("class", "primary");
    connect(createTicketBtn, &QPushButton::clicked, this, &FactoryMainWindow::onCreateTicket);
    toolbar_->addWidget(createTicketBtn);
    
    toolbar_->addSeparator();
    
    QPushButton* refreshBtn = new QPushButton("刷新");
    connect(refreshBtn, &QPushButton::clicked, this, &FactoryMainWindow::onRefreshDashboard);
    toolbar_->addWidget(refreshBtn);
    
    toolbar_->addSeparator();
    
    QPushButton* logoutBtn = new QPushButton("登出");
    logoutBtn->setProperty("class", "danger");
    connect(logoutBtn, &QPushButton::clicked, this, &FactoryMainWindow::onLogout);
    toolbar_->addWidget(logoutBtn);
}

void FactoryMainWindow::setupStatusBar()
{
    statusBar_ = statusBar();
    statusLabel_ = new QLabel("就绪");
    statusBar_->addWidget(statusLabel_);
    
    QLabel* userLabel = new QLabel(QString("用户: %1 | 角色: 工厂").arg(username_));
    statusBar_->addPermanentWidget(userLabel);
}

QWidget* FactoryMainWindow::createDashboardView()
{
    QWidget* widget = new QWidget();
    widget->setProperty("class", "dashboard-card");
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    // Header
    QLabel* header = new QLabel("工厂仪表盘");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Create charts layout
    QHBoxLayout* chartsLayout = new QHBoxLayout();
    
    // Temperature chart
    tempSeries_ = new QLineSeries();
    tempSeries_->setName("温度 (°C)");
    QChart* tempChartObj = new QChart();
    tempChartObj->addSeries(tempSeries_);
    tempChartObj->setTitle("设备温度监控");
    tempChartObj->createDefaultAxes();
    tempChart_ = new QChartView(tempChartObj);
    tempChart_->setRenderHint(QPainter::Antialiasing);
    chartsLayout->addWidget(tempChart_);
    
    // Pressure chart
    pressureSeries_ = new QLineSeries();
    pressureSeries_->setName("压力 (Pa)");
    QChart* pressureChartObj = new QChart();
    pressureChartObj->addSeries(pressureSeries_);
    pressureChartObj->setTitle("系统压力监控");
    pressureChartObj->createDefaultAxes();
    pressureChart_ = new QChartView(pressureChartObj);
    pressureChart_->setRenderHint(QPainter::Antialiasing);
    chartsLayout->addWidget(pressureChart_);
    
    layout->addLayout(chartsLayout);
    
    // Status indicators
    QHBoxLayout* statusLayout = new QHBoxLayout();
    
    QGroupBox* statusGroup = new QGroupBox("系统状态");
    QFormLayout* statusForm = new QFormLayout(statusGroup);
    
    QLabel* temp = new QLabel("23.5°C");
    temp->setProperty("class", "success");
    statusForm->addRow("当前温度:", temp);
    
    QLabel* pressure = new QLabel("101.3 kPa");
    pressure->setProperty("class", "success");
    statusForm->addRow("当前压力:", pressure);
    
    QProgressBar* efficiency = new QProgressBar();
    efficiency->setValue(85);
    statusForm->addRow("运行效率:", efficiency);
    
    statusLayout->addWidget(statusGroup);
    
    QGroupBox* alertsGroup = new QGroupBox("告警信息");
    QVBoxLayout* alertsLayout = new QVBoxLayout(alertsGroup);
    QLabel* alertsLabel = new QLabel("✅ 所有系统正常运行");
    alertsLabel->setProperty("class", "success");
    alertsLayout->addWidget(alertsLabel);
    statusLayout->addWidget(alertsGroup);
    
    layout->addLayout(statusLayout);
    
    // Activity log
    QGroupBox* logGroup = new QGroupBox("活动日志");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    logView_ = new QTextEdit();
    logView_->setMaximumHeight(150);
    logView_->setReadOnly(true);
    logView_->append("系统启动完成");
    logView_->append("用户登录: " + username_);
    logLayout->addWidget(logView_);
    layout->addWidget(logGroup);
    
    // Initialize chart data
    updateDashboardData();
    
    return widget;
}

QWidget* FactoryMainWindow::createTicketsView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("工单管理");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Create ticket form
    QGroupBox* createGroup = new QGroupBox("创建新工单");
    QFormLayout* createForm = new QFormLayout(createGroup);
    
    QLineEdit* titleEdit = new QLineEdit();
    titleEdit->setPlaceholderText("工单标题");
    createForm->addRow("标题:", titleEdit);
    
    QTextEdit* descEdit = new QTextEdit();
    descEdit->setMaximumHeight(100);
    descEdit->setPlaceholderText("问题描述");
    createForm->addRow("描述:", descEdit);
    
    QPushButton* createBtn = new QPushButton("创建工单");
    createBtn->setProperty("class", "primary");
    createForm->addRow("", createBtn);
    
    layout->addWidget(createGroup);
    
    // Tickets table
    QGroupBox* listGroup = new QGroupBox("现有工单");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);
    
    QTableWidget* ticketsTable = new QTableWidget(0, 4);
    QStringList headers = {"工单号", "标题", "状态", "创建时间"};
    ticketsTable->setHorizontalHeaderLabels(headers);
    ticketsTable->horizontalHeader()->setStretchLastSection(true);
    
    // Sample data
    ticketsTable->insertRow(0);
    ticketsTable->setItem(0, 0, new QTableWidgetItem("T001"));
    ticketsTable->setItem(0, 1, new QTableWidgetItem("设备温度异常"));
    ticketsTable->setItem(0, 2, new QTableWidgetItem("已解决"));
    ticketsTable->setItem(0, 3, new QTableWidgetItem("2024-01-15 10:30"));
    
    listLayout->addWidget(ticketsTable);
    layout->addWidget(listGroup);
    
    return widget;
}

QWidget* FactoryMainWindow::createMeetingView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("远程会议");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Meeting controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    QPushButton* startMeetingBtn = new QPushButton("开始会议");
    startMeetingBtn->setProperty("class", "primary");
    QPushButton* joinMeetingBtn = new QPushButton("加入会议");
    QPushButton* endMeetingBtn = new QPushButton("结束会议");
    endMeetingBtn->setProperty("class", "danger");
    
    controlsLayout->addWidget(startMeetingBtn);
    controlsLayout->addWidget(joinMeetingBtn);
    controlsLayout->addWidget(endMeetingBtn);
    controlsLayout->addStretch();
    
    layout->addLayout(controlsLayout);
    
    // Video area placeholder
    QLabel* videoPlaceholder = new QLabel("视频会议区域\n(暂未实现)");
    videoPlaceholder->setAlignment(Qt::AlignCenter);
    videoPlaceholder->setProperty("class", "video-preview");
    videoPlaceholder->setMinimumHeight(400);
    layout->addWidget(videoPlaceholder);
    
    return widget;
}

QWidget* FactoryMainWindow::createTelemetryView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("设备监控");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    QLabel* placeholder = new QLabel("设备遥测数据显示区域\n(暂未实现)");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setMinimumHeight(300);
    layout->addWidget(placeholder);
    
    return widget;
}

QWidget* FactoryMainWindow::createChatView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("消息中心");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    QLabel* placeholder = new QLabel("聊天消息区域\n(暂未实现)");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setMinimumHeight(300);
    layout->addWidget(placeholder);
    
    return widget;
}

QWidget* FactoryMainWindow::createKnowledgeBaseView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("知识库");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    QLabel* placeholder = new QLabel("知识库搜索和浏览\n(暂未实现)");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setMinimumHeight(300);
    layout->addWidget(placeholder);
    
    return widget;
}

void FactoryMainWindow::onNavigationChanged()
{
    int index = navigationList_->currentRow();
    contentStack_->setCurrentIndex(index);
    
    // Update status based on current view
    QStringList viewNames = {"仪表盘", "工单管理", "远程会议", "设备监控", "消息中心", "知识库"};
    if (index >= 0 && index < viewNames.size()) {
        statusLabel_->setText(QString("当前视图: %1").arg(viewNames[index]));
    }
}

void FactoryMainWindow::onLogout()
{
    // TODO: Implement logout logic
    QApplication::quit();
}

void FactoryMainWindow::onCreateTicket()
{
    // Switch to tickets view
    navigationList_->setCurrentRow(1);
    statusLabel_->setText("准备创建新工单");
}

void FactoryMainWindow::onRefreshDashboard()
{
    updateDashboardData();
    statusLabel_->setText("仪表盘数据已刷新");
}

void FactoryMainWindow::updateDashboardData()
{
    // Generate simulated data for charts
    static int timePoint = 0;
    timePoint++;
    
    // Add temperature data point (20-30°C range)
    qreal temp = 25.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
    tempSeries_->append(timePoint, temp);
    
    // Add pressure data point (95-105 kPa range)
    qreal pressure = 100.0 + (QRandomGenerator::global()->generateDouble() - 0.5) * 10.0;
    pressureSeries_->append(timePoint, pressure);
    
    // Keep only last 20 points
    if (tempSeries_->count() > 20) {
        tempSeries_->remove(0);
        pressureSeries_->remove(0);
    }
    
    // Update chart axes
    if (tempSeries_->count() > 1) {
        tempChart_->chart()->axes(Qt::Horizontal).first()->setRange(
            tempSeries_->at(0).x(), tempSeries_->at(tempSeries_->count()-1).x());
        pressureChart_->chart()->axes(Qt::Horizontal).first()->setRange(
            pressureSeries_->at(0).x(), pressureSeries_->at(pressureSeries_->count()-1).x());
    }
}