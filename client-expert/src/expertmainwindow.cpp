#include "expertmainwindow.h"
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
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTimer>

ExpertMainWindow::ExpertMainWindow(const QString& username, const QString& role, QWidget *parent)
    : QMainWindow(parent)
    , username_(username)
    , role_(role)
    , connection_(nullptr)
{
    setupUI();
    setWindowTitle(QString("Industrial Remote Expert - 专家端 (%1)").arg(username_));
    resize(1200, 800);
    
    // Start tickets refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &ExpertMainWindow::updateTicketsList);
    refreshTimer_->start(5000); // Update every 5 seconds
}

void ExpertMainWindow::setupUI()
{
    setupToolbar();
    setupStatusBar();
    
    // Create main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal);
    setCentralWidget(mainSplitter_);
    
    setupNavigation();
    setupViews();
}

void ExpertMainWindow::setupNavigation()
{
    navigationList_ = new QListWidget();
    navigationList_->setMaximumWidth(200);
    navigationList_->setMinimumWidth(180);
    
    // Add navigation items with icons (future enhancement)
    QListWidgetItem* dashboardItem = new QListWidgetItem("🎯 专家面板", navigationList_);
    dashboardItem->setData(Qt::UserRole, 0);
    
    QListWidgetItem* ticketsItem = new QListWidgetItem("📋 待处理工单", navigationList_);
    ticketsItem->setData(Qt::UserRole, 1);
    
    QListWidgetItem* meetingItem = new QListWidgetItem("📹 远程协助", navigationList_);
    meetingItem->setData(Qt::UserRole, 2);
    
    QListWidgetItem* diagnosticsItem = new QListWidgetItem("🔍 诊断工具", navigationList_);
    diagnosticsItem->setData(Qt::UserRole, 3);
    
    QListWidgetItem* chatItem = new QListWidgetItem("💬 沟通中心", navigationList_);
    chatItem->setData(Qt::UserRole, 4);
    
    QListWidgetItem* knowledgeItem = new QListWidgetItem("📚 技术资料", navigationList_);
    knowledgeItem->setData(Qt::UserRole, 5);
    
    navigationList_->setCurrentRow(0); // Default to dashboard
    
    connect(navigationList_, &QListWidget::currentRowChanged, 
            this, &ExpertMainWindow::onNavigationChanged);
    
    mainSplitter_->addWidget(navigationList_);
}

void ExpertMainWindow::setupViews()
{
    contentStack_ = new QStackedWidget();
    
    // Create all views
    dashboardView_ = createDashboardView();
    ticketsView_ = createTicketsView();
    meetingView_ = createMeetingView();
    diagnosticsView_ = createDiagnosticsView();
    chatView_ = createChatView();
    knowledgeBaseView_ = createKnowledgeBaseView();
    
    // Add to stack
    contentStack_->addWidget(dashboardView_);
    contentStack_->addWidget(ticketsView_);
    contentStack_->addWidget(meetingView_);
    contentStack_->addWidget(diagnosticsView_);
    contentStack_->addWidget(chatView_);
    contentStack_->addWidget(knowledgeBaseView_);
    
    mainSplitter_->addWidget(contentStack_);
    mainSplitter_->setStretchFactor(1, 1); // Content area gets most space
}

void ExpertMainWindow::setupToolbar()
{
    toolbar_ = addToolBar("专家工具栏");
    
    QPushButton* joinTicketBtn = new QPushButton("接受工单");
    joinTicketBtn->setProperty("class", "primary");
    connect(joinTicketBtn, &QPushButton::clicked, this, &ExpertMainWindow::onJoinTicket);
    toolbar_->addWidget(joinTicketBtn);
    
    toolbar_->addSeparator();
    
    QPushButton* refreshBtn = new QPushButton("刷新");
    connect(refreshBtn, &QPushButton::clicked, this, &ExpertMainWindow::onRefreshTickets);
    toolbar_->addWidget(refreshBtn);
    
    toolbar_->addSeparator();
    
    QPushButton* logoutBtn = new QPushButton("登出");
    logoutBtn->setProperty("class", "danger");
    connect(logoutBtn, &QPushButton::clicked, this, &ExpertMainWindow::onLogout);
    toolbar_->addWidget(logoutBtn);
}

void ExpertMainWindow::setupStatusBar()
{
    statusBar_ = statusBar();
    statusLabel_ = new QLabel("就绪 - 等待工单分配");
    statusBar_->addWidget(statusLabel_);
    
    QLabel* userLabel = new QLabel(QString("专家: %1 | 在线").arg(username_));
    statusBar_->addPermanentWidget(userLabel);
}

QWidget* ExpertMainWindow::createDashboardView()
{
    QWidget* widget = new QWidget();
    widget->setProperty("class", "dashboard-card");
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    // Header
    QLabel* header = new QLabel("专家工作台");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Statistics layout
    QHBoxLayout* statsLayout = new QHBoxLayout();
    
    // Today's stats
    QGroupBox* todayGroup = new QGroupBox("今日统计");
    QFormLayout* todayForm = new QFormLayout(todayGroup);
    
    QLabel* todayTickets = new QLabel("3");
    todayTickets->setProperty("class", "title");
    todayForm->addRow("处理工单:", todayTickets);
    
    QLabel* avgTime = new QLabel("45分钟");
    avgTime->setProperty("class", "subtitle");
    todayForm->addRow("平均处理时间:", avgTime);
    
    QLabel* satisfaction = new QLabel("4.8/5.0");
    satisfaction->setProperty("class", "success");
    todayForm->addRow("满意度评分:", satisfaction);
    
    statsLayout->addWidget(todayGroup);
    
    // Current workload
    QGroupBox* workloadGroup = new QGroupBox("当前工作负载");
    QVBoxLayout* workloadLayout = new QVBoxLayout(workloadGroup);
    
    QLabel* activeTickets = new QLabel("🔧 2个活跃工单");
    activeTickets->setProperty("class", "warning");
    workloadLayout->addWidget(activeTickets);
    
    QLabel* pendingTickets = new QLabel("⏳ 5个待处理工单");
    pendingTickets->setProperty("class", "muted");
    workloadLayout->addWidget(pendingTickets);
    
    QLabel* availability = new QLabel("✅ 可接受新工单");
    availability->setProperty("class", "success");
    workloadLayout->addWidget(availability);
    
    statsLayout->addWidget(workloadGroup);
    
    layout->addLayout(statsLayout);
    
    // Recent activity
    QGroupBox* activityGroup = new QGroupBox("最近活动");
    QVBoxLayout* activityLayout = new QVBoxLayout(activityGroup);
    
    logView_ = new QTextEdit();
    logView_->setMaximumHeight(200);
    logView_->setReadOnly(true);
    logView_->append("系统启动 - 专家端已连接");
    logView_->append("用户登录: " + username_);
    logView_->append("等待工单分配...");
    
    activityLayout->addWidget(logView_);
    layout->addWidget(activityGroup);
    
    // Expertise areas
    QGroupBox* expertiseGroup = new QGroupBox("专业领域");
    QVBoxLayout* expertiseLayout = new QVBoxLayout(expertiseGroup);
    
    QLabel* skills = new QLabel("🔧 机械故障诊断\n⚡ 电气系统维护\n💻 自动化控制\n🌡️ 传感器校准");
    skills->setProperty("class", "subtitle");
    expertiseLayout->addWidget(skills);
    
    layout->addWidget(expertiseGroup);
    
    return widget;
}

QWidget* ExpertMainWindow::createTicketsView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("待处理工单");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Filter controls
    QHBoxLayout* filterLayout = new QHBoxLayout();
    
    QLabel* filterLabel = new QLabel("筛选:");
    filterLayout->addWidget(filterLabel);
    
    QComboBox* priorityFilter = new QComboBox();
    priorityFilter->addItems({"全部优先级", "高", "中", "低"});
    filterLayout->addWidget(priorityFilter);
    
    QComboBox* categoryFilter = new QComboBox();
    categoryFilter->addItems({"全部类别", "机械", "电气", "软件", "其他"});
    filterLayout->addWidget(categoryFilter);
    
    filterLayout->addStretch();
    
    QPushButton* refreshBtn = new QPushButton("刷新列表");
    filterLayout->addWidget(refreshBtn);
    
    layout->addLayout(filterLayout);
    
    // Tickets table
    ticketsTable_ = new QTableWidget(0, 6);
    QStringList headers = {"工单号", "标题", "优先级", "类别", "创建时间", "操作"};
    ticketsTable_->setHorizontalHeaderLabels(headers);
    ticketsTable_->horizontalHeader()->setStretchLastSection(true);
    
    // Sample data
    updateTicketsList();
    
    layout->addWidget(ticketsTable_);
    
    return widget;
}

QWidget* ExpertMainWindow::createMeetingView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("远程协助");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Meeting controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    QPushButton* joinMeetingBtn = new QPushButton("加入协助会话");
    joinMeetingBtn->setProperty("class", "primary");
    QPushButton* startScreenShareBtn = new QPushButton("开始屏幕共享");
    QPushButton* endMeetingBtn = new QPushButton("结束协助");
    endMeetingBtn->setProperty("class", "danger");
    
    controlsLayout->addWidget(joinMeetingBtn);
    controlsLayout->addWidget(startScreenShareBtn);
    controlsLayout->addWidget(endMeetingBtn);
    controlsLayout->addStretch();
    
    layout->addLayout(controlsLayout);
    
    // Video grid placeholder
    QGroupBox* videoGroup = new QGroupBox("视频会议区域");
    QVBoxLayout* videoLayout = new QVBoxLayout(videoGroup);
    
    QLabel* videoPlaceholder = new QLabel("多方视频协助界面\n等待实现...");
    videoPlaceholder->setAlignment(Qt::AlignCenter);
    videoPlaceholder->setProperty("class", "video-preview");
    videoPlaceholder->setMinimumHeight(300);
    videoLayout->addWidget(videoPlaceholder);
    
    layout->addWidget(videoGroup);
    
    // Chat docked area
    QGroupBox* chatGroup = new QGroupBox("实时沟通");
    QVBoxLayout* chatLayout = new QVBoxLayout(chatGroup);
    
    QTextEdit* chatDisplay = new QTextEdit();
    chatDisplay->setMaximumHeight(100);
    chatDisplay->setReadOnly(true);
    chatDisplay->append("系统: 聊天功能准备就绪");
    chatLayout->addWidget(chatDisplay);
    
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLineEdit* chatInput = new QLineEdit();
    chatInput->setPlaceholderText("输入消息...");
    QPushButton* sendBtn = new QPushButton("发送");
    sendBtn->setProperty("class", "primary");
    
    inputLayout->addWidget(chatInput);
    inputLayout->addWidget(sendBtn);
    chatLayout->addLayout(inputLayout);
    
    layout->addWidget(chatGroup);
    
    return widget;
}

QWidget* ExpertMainWindow::createDiagnosticsView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("诊断工具");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Diagnostic tools
    QHBoxLayout* toolsLayout = new QHBoxLayout();
    
    QGroupBox* remoteGroup = new QGroupBox("远程诊断");
    QVBoxLayout* remoteLayout = new QVBoxLayout(remoteGroup);
    
    QPushButton* sysInfoBtn = new QPushButton("获取系统信息");
    QPushButton* logAnalysisBtn = new QPushButton("日志分析");
    QPushButton* performanceBtn = new QPushButton("性能检测");
    
    remoteLayout->addWidget(sysInfoBtn);
    remoteLayout->addWidget(logAnalysisBtn);
    remoteLayout->addWidget(performanceBtn);
    
    toolsLayout->addWidget(remoteGroup);
    
    QGroupBox* simulatorGroup = new QGroupBox("故障模拟");
    QVBoxLayout* simulatorLayout = new QVBoxLayout(simulatorGroup);
    
    QPushButton* tempSimBtn = new QPushButton("温度异常模拟");
    QPushButton* pressureSimBtn = new QPushButton("压力故障模拟");
    QPushButton* networkSimBtn = new QPushButton("网络中断模拟");
    
    simulatorLayout->addWidget(tempSimBtn);
    simulatorLayout->addWidget(pressureSimBtn);
    simulatorLayout->addWidget(networkSimBtn);
    
    toolsLayout->addWidget(simulatorGroup);
    
    layout->addLayout(toolsLayout);
    
    // Results area
    QGroupBox* resultsGroup = new QGroupBox("诊断结果");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    QTextEdit* resultsDisplay = new QTextEdit();
    resultsDisplay->setReadOnly(true);
    resultsDisplay->append("等待执行诊断命令...");
    resultsLayout->addWidget(resultsDisplay);
    
    layout->addWidget(resultsGroup);
    
    return widget;
}

QWidget* ExpertMainWindow::createChatView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("沟通中心");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    QLabel* placeholder = new QLabel("多方聊天和消息历史\n(暂未实现)");
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setMinimumHeight(300);
    layout->addWidget(placeholder);
    
    return widget;
}

QWidget* ExpertMainWindow::createKnowledgeBaseView()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    QLabel* header = new QLabel("技术资料");
    header->setProperty("class", "title");
    layout->addWidget(header);
    
    // Search bar
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* searchInput = new QLineEdit();
    searchInput->setPlaceholderText("搜索技术文档、故障案例...");
    QPushButton* searchBtn = new QPushButton("搜索");
    searchBtn->setProperty("class", "primary");
    
    searchLayout->addWidget(searchInput);
    searchLayout->addWidget(searchBtn);
    layout->addLayout(searchLayout);
    
    // Categories
    QGroupBox* categoriesGroup = new QGroupBox("资料分类");
    QVBoxLayout* categoriesLayout = new QVBoxLayout(categoriesGroup);
    
    QLabel* categories = new QLabel(
        "📖 设备手册\n"
        "🔧 维修指南\n" 
        "📊 故障案例库\n"
        "🎯 最佳实践\n"
        "📋 标准流程\n"
        "🆘 应急预案"
    );
    categories->setProperty("class", "subtitle");
    categoriesLayout->addWidget(categories);
    
    layout->addWidget(categoriesGroup);
    
    return widget;
}

void ExpertMainWindow::onNavigationChanged()
{
    int index = navigationList_->currentRow();
    contentStack_->setCurrentIndex(index);
    
    // Update status based on current view
    QStringList viewNames = {"专家面板", "待处理工单", "远程协助", "诊断工具", "沟通中心", "技术资料"};
    if (index >= 0 && index < viewNames.size()) {
        statusLabel_->setText(QString("当前视图: %1").arg(viewNames[index]));
    }
}

void ExpertMainWindow::onLogout()
{
    // TODO: Implement logout logic
    QApplication::quit();
}

void ExpertMainWindow::onJoinTicket()
{
    // Switch to tickets view
    navigationList_->setCurrentRow(1);
    statusLabel_->setText("准备接受工单");
}

void ExpertMainWindow::onRefreshTickets()
{
    updateTicketsList();
    statusLabel_->setText("工单列表已刷新");
}

void ExpertMainWindow::updateTicketsList()
{
    // Clear existing rows
    ticketsTable_->setRowCount(0);
    
    // Sample data - in real implementation, this would come from server
    struct TicketData {
        QString id;
        QString title;
        QString priority;
        QString category;
        QString time;
    };
    
    QList<TicketData> tickets = {
        {"T001", "设备温度传感器异常", "高", "机械", "2024-01-15 14:30"},
        {"T002", "压力控制系统故障", "中", "电气", "2024-01-15 13:45"},
        {"T003", "自动化程序错误", "高", "软件", "2024-01-15 12:20"},
        {"T004", "冷却系统维护请求", "低", "机械", "2024-01-15 11:10"},
        {"T005", "网络连接不稳定", "中", "其他", "2024-01-15 10:05"}
    };
    
    for (int i = 0; i < tickets.size(); ++i) {
        const auto& ticket = tickets[i];
        ticketsTable_->insertRow(i);
        ticketsTable_->setItem(i, 0, new QTableWidgetItem(ticket.id));
        ticketsTable_->setItem(i, 1, new QTableWidgetItem(ticket.title));
        ticketsTable_->setItem(i, 2, new QTableWidgetItem(ticket.priority));
        ticketsTable_->setItem(i, 3, new QTableWidgetItem(ticket.category));
        ticketsTable_->setItem(i, 4, new QTableWidgetItem(ticket.time));
        
        QPushButton* actionBtn = new QPushButton("接受");
        actionBtn->setProperty("class", "primary");
        ticketsTable_->setCellWidget(i, 5, actionBtn);
    }
    
    // Resize columns to content
    ticketsTable_->resizeColumnsToContents();
}