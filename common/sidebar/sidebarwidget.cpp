#include "sidebarwidget.h"

SidebarWidget::SidebarWidget(QWidget *parent)
    : QFrame(parent)
{
    setupUI();
}

void SidebarWidget::setupUI()
{
    setProperty("class", "sidebar");
    setFrameStyle(QFrame::StyledPanel);
    setMinimumWidth(200);
    setMaximumWidth(250);
    
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 20, 0, 20);
    layout_->setSpacing(2);
    
    // 应用程序标题/Logo区域
    auto *titleLabel = new QLabel("工业远程专家");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 16px;
            font-weight: bold;
            color: #3498db;
            padding: 20px 10px;
            border-bottom: 1px solid #7f8c8d;
            margin-bottom: 20px;
        }
    )");
    layout_->addWidget(titleLabel);
    
    buttonGroup_ = new QButtonGroup(this);
    buttonGroup_->setExclusive(true);
    
    layout_->addStretch();
    
    // 设置侧边栏样式
    setStyleSheet(R"(
        QFrame.sidebar {
            background-color: #2c3e50;
            border-right: 1px solid #7f8c8d;
        }
        QFrame.sidebar QPushButton {
            text-align: left;
            padding: 15px 20px;
            border: none;
            border-radius: 0;
            background-color: transparent;
            font-weight: normal;
            color: #ecf0f1;
            font-size: 12px;
        }
        QFrame.sidebar QPushButton:hover {
            background-color: #34495e;
        }
        QFrame.sidebar QPushButton:checked {
            background-color: #3498db;
            font-weight: bold;
        }
        QFrame.sidebar QPushButton:pressed {
            background-color: #2980b9;
        }
    )");
}

void SidebarWidget::addNavigationItem(const QString &name, const QString &text, const QString &icon)
{
    auto *button = new QPushButton(text);
    button->setCheckable(true);
    button->setProperty("name", name);
    
    if (!icon.isEmpty()) {
        // TODO: Set icon when available
        button->setText(QString("  %1").arg(text)); // Add some spacing for now
    }
    
    buttons_[name] = button;
    buttonGroup_->addButton(button);
    
    // Insert before the stretch
    layout_->insertWidget(layout_->count() - 1, button);
    
    connect(button, &QPushButton::clicked, this, &SidebarWidget::onButtonClicked);
}

void SidebarWidget::setActiveItem(const QString &name)
{
    if (buttons_.contains(name)) {
        activeItem_ = name;
        buttons_[name]->setChecked(true);
        updateButtonStates();
    }
}

QString SidebarWidget::activeItem() const
{
    return activeItem_;
}

void SidebarWidget::onButtonClicked()
{
    auto *button = qobject_cast<QPushButton*>(sender());
    if (button) {
        QString name = button->property("name").toString();
        if (name != activeItem_) {
            activeItem_ = name;
            updateButtonStates();
            emit navigationChanged(name);
        }
    }
}

void SidebarWidget::updateButtonStates()
{
    for (auto it = buttons_.begin(); it != buttons_.end(); ++it) {
        it.value()->setChecked(it.key() == activeItem_);
    }
}