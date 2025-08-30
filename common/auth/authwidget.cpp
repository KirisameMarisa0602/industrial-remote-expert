#include "authwidget.h"

AuthWidget::AuthWidget(QWidget *parent)
    : QWidget(parent)
    , isRoleSelected_(false)
{
    setupUI();
    updateButtonState();
}

void AuthWidget::setupUI()
{
    setFixedSize(350, 280);
    setWindowTitle("用户登录/注册");
    
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题
    auto *titleLabel = new QLabel("工业现场远程专家支持系统");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #2c3e50; }");
    mainLayout->addWidget(titleLabel);
    
    // 表单布局
    auto *formLayout = new QGridLayout();
    formLayout->setSpacing(10);
    
    // 用户名
    formLayout->addWidget(new QLabel("用户名:"), 0, 0);
    usernameEdit_ = new QLineEdit();
    usernameEdit_->setPlaceholderText("请输入用户名");
    formLayout->addWidget(usernameEdit_, 0, 1);
    
    // 密码
    formLayout->addWidget(new QLabel("密码:"), 1, 0);
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("请输入密码");
    formLayout->addWidget(passwordEdit_, 1, 1);
    
    // 身份选择
    formLayout->addWidget(new QLabel("身份:"), 2, 0);
    roleComboBox_ = new QComboBox();
    roleComboBox_->addItem("请选择身份...", "");
    roleComboBox_->addItem("工厂人员", "工厂");
    roleComboBox_->addItem("专家", "专家");
    roleComboBox_->setCurrentIndex(0);
    formLayout->addWidget(roleComboBox_, 2, 1);
    
    mainLayout->addLayout(formLayout);
    
    // 身份状态提示
    roleStatusLabel_ = new QLabel("未选择身份");
    roleStatusLabel_->setAlignment(Qt::AlignCenter);
    roleStatusLabel_->setStyleSheet("QLabel { color: #7f8c8d; font-style: italic; }");
    mainLayout->addWidget(roleStatusLabel_);
    
    // 按钮布局
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    loginButton_ = new QPushButton("登录");
    registerButton_ = new QPushButton("注册");
    
    // 按钮样式
    QString buttonStyle = R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #2980b9;
        }
        QPushButton:pressed {
            background-color: #21618c;
        }
        QPushButton:disabled {
            background-color: #bdc3c7;
            color: #7f8c8d;
        }
    )";
    
    loginButton_->setStyleSheet(buttonStyle);
    registerButton_->setStyleSheet(buttonStyle);
    
    buttonLayout->addWidget(loginButton_);
    buttonLayout->addWidget(registerButton_);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(loginButton_, &QPushButton::clicked, this, &AuthWidget::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &AuthWidget::onRegisterClicked);
    connect(roleComboBox_, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &AuthWidget::onRoleChanged);
}

void AuthWidget::onLoginClicked()
{
    if (validateForm()) {
        emit loginRequested(getUsername(), getPassword(), getSelectedRole());
    }
}

void AuthWidget::onRegisterClicked()
{
    if (validateForm()) {
        emit registerRequested(getUsername(), getPassword(), getSelectedRole());
    }
}

void AuthWidget::onRoleChanged(const QString& role)
{
    Q_UNUSED(role)
    isRoleSelected_ = (roleComboBox_->currentData().toString() != "");
    updateButtonState();
    
    if (isRoleSelected_) {
        roleStatusLabel_->setText(QString("已选择: %1").arg(roleComboBox_->currentText()));
        roleStatusLabel_->setStyleSheet("QLabel { color: #27ae60; font-style: italic; }");
    } else {
        roleStatusLabel_->setText("未选择身份");
        roleStatusLabel_->setStyleSheet("QLabel { color: #7f8c8d; font-style: italic; }");
    }
}

void AuthWidget::updateButtonState()
{
    loginButton_->setEnabled(isRoleSelected_);
    registerButton_->setEnabled(isRoleSelected_);
}

QString AuthWidget::getUsername() const
{
    return usernameEdit_->text().trimmed();
}

QString AuthWidget::getPassword() const
{
    return passwordEdit_->text();
}

QString AuthWidget::getSelectedRole() const
{
    return roleComboBox_->currentData().toString();
}

bool AuthWidget::validateForm()
{
    if (!isRoleSelected_) {
        QMessageBox::warning(this, "验证错误", "请先选择您的身份");
        return false;
    }
    
    if (getUsername().isEmpty()) {
        QMessageBox::warning(this, "验证错误", "请输入用户名");
        usernameEdit_->setFocus();
        return false;
    }
    
    if (getPassword().isEmpty()) {
        QMessageBox::warning(this, "验证错误", "请输入密码");
        passwordEdit_->setFocus();
        return false;
    }
    
    if (getPassword().length() < 4) {
        QMessageBox::warning(this, "验证错误", "密码长度至少为4位");
        passwordEdit_->setFocus();
        return false;
    }
    
    return true;
}

void AuthWidget::clearForm()
{
    usernameEdit_->clear();
    passwordEdit_->clear();
    roleComboBox_->setCurrentIndex(0);
    isRoleSelected_ = false;
    updateButtonState();
}