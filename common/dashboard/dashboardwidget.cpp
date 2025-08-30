#include "dashboardwidget.h"

// DashboardCard Implementation
DashboardCard::DashboardCard(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName("dashboard-card");
    setProperty("class", "dashboard-card");
    setupUI();
    setTitle(title);
}

void DashboardCard::setupUI()
{
    setFrameStyle(QFrame::StyledPanel);
    setMinimumSize(200, 120);
    setMaximumSize(300, 180);
    
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 12, 16, 12);
    mainLayout->setSpacing(8);
    
    // 头部布局 (图标 + 标题)
    auto *headerLayout = new QHBoxLayout();
    
    iconLabel_ = new QLabel();
    iconLabel_->setFixedSize(24, 24);
    iconLabel_->setScaledContents(true);
    headerLayout->addWidget(iconLabel_);
    
    titleLabel_ = new QLabel();
    titleLabel_->setProperty("class", "card-title");
    titleLabel_->setStyleSheet("font-weight: bold; color: #3498db;");
    headerLayout->addWidget(titleLabel_);
    headerLayout->addStretch();
    
    mainLayout->addLayout(headerLayout);
    
    // 数值显示
    valueLabel_ = new QLabel("--");
    valueLabel_->setProperty("class", "card-value");
    valueLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: #27ae60;");
    valueLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(valueLabel_);
    
    // 内容区域
    contentLayout_ = new QVBoxLayout();
    mainLayout->addLayout(contentLayout_);
    
    mainLayout->addStretch();
    
    // 设置样式
    setStyleSheet(R"(
        QFrame#dashboard-card {
            background-color: #34495e;
            border: 1px solid #7f8c8d;
            border-radius: 8px;
        }
        QFrame#dashboard-card:hover {
            border-color: #3498db;
        }
    )");
}

void DashboardCard::setTitle(const QString &title)
{
    titleLabel_->setText(title);
    setProperty("cardTitle", title); // Store title for easy access
}

void DashboardCard::setValue(const QString &value)
{
    valueLabel_->setText(value);
}

void DashboardCard::setValueColor(const QColor &color)
{
    valueLabel_->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;").arg(color.name()));
}

void DashboardCard::setIcon(const QPixmap &icon)
{
    iconLabel_->setPixmap(icon);
}

// DashboardWidget Implementation
DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    createDefaultCards();
}

void DashboardWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    // 标题
    auto *titleLabel = new QLabel("系统状态监控");
    titleLabel->setProperty("class", "title");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #3498db; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // 卡片网格布局
    cardLayout_ = new QGridLayout();
    cardLayout_->setSpacing(15);
    mainLayout->addLayout(cardLayout_);
    
    mainLayout->addStretch();
}

void DashboardWidget::createDefaultCards()
{
    // 连接状态卡片
    auto *connectionCard = new DashboardCard("连接状态");
    connectionCard->setValue("未连接");
    connectionCard->setValueColor(QColor("#e74c3c"));
    addCard(connectionCard, 0, 0);
    
    // 参与者数量卡片
    auto *participantCard = new DashboardCard("在线参与者");
    participantCard->setValue("0");
    addCard(participantCard, 0, 1);
    
    // 活动房间卡片
    auto *roomCard = new DashboardCard("当前房间");
    roomCard->setValue("无");
    addCard(roomCard, 1, 0);
    
    // 消息统计卡片
    auto *messageCard = new DashboardCard("消息数量");
    messageCard->setValue("0");
    addCard(messageCard, 1, 1);
}

void DashboardWidget::addCard(DashboardCard *card, int row, int col)
{
    cardLayout_->addWidget(card, row, col);
    QString title = card->property("cardTitle").toString();
    cards_[title] = card;
    
    // 添加点击事件 - 使用installEventFilter
    card->installEventFilter(this);
}

DashboardCard* DashboardWidget::getCard(const QString &title)
{
    return cards_.value(title, nullptr);
}

void DashboardWidget::updateConnectionStatus(bool connected)
{
    auto *card = getCard("连接状态");
    if (card) {
        if (connected) {
            card->setValue("已连接");
            card->setValueColor(QColor("#27ae60"));
        } else {
            card->setValue("未连接");
            card->setValueColor(QColor("#e74c3c"));
        }
    }
}

void DashboardWidget::updateParticipantCount(int count)
{
    auto *card = getCard("在线参与者");
    if (card) {
        card->setValue(QString::number(count));
        card->setValueColor(count > 0 ? QColor("#27ae60") : QColor("#95a5a6"));
    }
}

void DashboardWidget::updateActiveRoom(const QString &roomId)
{
    auto *card = getCard("当前房间");
    if (card) {
        card->setValue(roomId.isEmpty() ? "无" : roomId);
        card->setValueColor(roomId.isEmpty() ? QColor("#95a5a6") : QColor("#3498db"));
    }
}

bool DashboardWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *card = qobject_cast<DashboardCard*>(obj);
        if (card) {
            // 查找卡片标题
            for (auto it = cards_.begin(); it != cards_.end(); ++it) {
                if (it.value() == card) {
                    emit cardClicked(it.key());
                    break;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}