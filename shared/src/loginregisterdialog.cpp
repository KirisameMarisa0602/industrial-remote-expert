#include "loginregisterdialog.h"
#include <QCryptographicHash>

LoginRegisterDialog::LoginRegisterDialog(QWidget *parent)
    : QDialog(parent)
    , tabWidget_(nullptr)
    , loginTab_(nullptr)
    , registerTab_(nullptr)
    , isRegister_(false)
{
    setupUI();
    setWindowTitle("登录 / 注册");
    setFixedSize(400, 500);
    setModal(true);
    
    // Apply modern styling
    setStyleSheet(R"(
        QDialog {
            background-color: #2b2b2b;
            color: #ffffff;
        }
        QTabWidget::pane {
            border: 1px solid #555555;
            background-color: #2b2b2b;
        }
        QTabBar::tab {
            background-color: #404040;
            color: #ffffff;
            padding: 8px 16px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #0078d4;
        }
        QTabBar::tab:hover {
            background-color: #555555;
        }
        QLineEdit {
            padding: 8px;
            border: 1px solid #555555;
            border-radius: 4px;
            background-color: #404040;
            color: #ffffff;
        }
        QLineEdit:focus {
            border: 2px solid #0078d4;
        }
        QPushButton {
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            background-color: #0078d4;
            color: #ffffff;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #106ebe;
        }
        QPushButton:pressed {
            background-color: #005a9e;
        }
        QPushButton:disabled {
            background-color: #555555;
            color: #888888;
        }
        QRadioButton {
            color: #ffffff;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 16px;
            height: 16px;
        }
        QRadioButton::indicator:unchecked {
            border: 2px solid #555555;
            border-radius: 8px;
            background-color: #404040;
        }
        QRadioButton::indicator:checked {
            border: 2px solid #0078d4;
            border-radius: 8px;
            background-color: #0078d4;
        }
        QLabel {
            color: #ffffff;
        }
        QProgressBar {
            border: 1px solid #555555;
            border-radius: 4px;
            text-align: center;
            background-color: #404040;
        }
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 3px;
        }
    )");
}

void LoginRegisterDialog::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    
    // Role status banner (shown at top when no role selected)
    roleStatusLabel_ = new QLabel("未选择身份");
    roleStatusLabel_->setAlignment(Qt::AlignCenter);
    roleStatusLabel_->setStyleSheet(R"(
        QLabel {
            background-color: #666666;
            color: #ffffff;
            padding: 8px;
            border-radius: 4px;
            font-weight: bold;
        }
    )");
    layout->addWidget(roleStatusLabel_);
    
    // Tab widget for login/register
    tabWidget_ = new QTabWidget;
    layout->addWidget(tabWidget_);
    
    // Login tab
    setupLoginTab();
    
    // Register tab  
    setupRegisterTab();
    
    // Role selection (shared between tabs)
    auto* roleGroupBox = new QGroupBox("身份选择");
    roleGroupBox->setStyleSheet("QGroupBox { color: #ffffff; font-weight: bold; }");
    auto* roleLayout = new QVBoxLayout(roleGroupBox);
    
    roleGroup_ = new QButtonGroup(this);
    roleNone_ = new QRadioButton("未选择身份");
    roleFactory_ = new QRadioButton("工厂客户端");
    roleExpert_ = new QRadioButton("技术专家客户端");
    
    roleNone_->setChecked(true); // Default to no selection
    
    roleGroup_->addButton(roleNone_, static_cast<int>(UserRole::None));
    roleGroup_->addButton(roleFactory_, static_cast<int>(UserRole::Factory));
    roleGroup_->addButton(roleExpert_, static_cast<int>(UserRole::Expert));
    
    roleLayout->addWidget(roleNone_);
    roleLayout->addWidget(roleFactory_);
    roleLayout->addWidget(roleExpert_);
    
    layout->addWidget(roleGroupBox);
    
    // Button layout
    auto* buttonLayout = new QHBoxLayout;
    cancelButton_ = new QPushButton("取消");
    loginButton_ = new QPushButton("登录");
    registerButton_ = new QPushButton("注册");
    
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(loginButton_);
    buttonLayout->addWidget(registerButton_);
    
    layout->addLayout(buttonLayout);
    
    // Connect signals
    connect(tabWidget_, &QTabWidget::currentChanged, this, &LoginRegisterDialog::onTabChanged);
    connect(roleGroup_, QOverload<int>::of(&QButtonGroup::idClicked), this, &LoginRegisterDialog::onRoleSelectionChanged);
    connect(loginButton_, &QPushButton::clicked, this, &LoginRegisterDialog::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginRegisterDialog::onRegisterClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    
    // Initial state
    onTabChanged(0);
    onRoleSelectionChanged();
}

void LoginRegisterDialog::setupLoginTab()
{
    loginTab_ = new QWidget;
    tabWidget_->addTab(loginTab_, "登录");
    
    auto* layout = new QFormLayout(loginTab_);
    
    loginUsername_ = new QLineEdit;
    loginUsername_->setPlaceholderText("请输入用户名");
    layout->addRow("用户名:", loginUsername_);
    
    loginPassword_ = new QLineEdit;
    loginPassword_->setEchoMode(QLineEdit::Password);
    loginPassword_->setPlaceholderText("请输入密码");
    layout->addRow("密码:", loginPassword_);
}

void LoginRegisterDialog::setupRegisterTab()
{
    registerTab_ = new QWidget;
    tabWidget_->addTab(registerTab_, "注册");
    
    auto* layout = new QFormLayout(registerTab_);
    
    registerUsername_ = new QLineEdit;
    registerUsername_->setPlaceholderText("请输入用户名");
    layout->addRow("用户名:", registerUsername_);
    
    registerPassword_ = new QLineEdit;
    registerPassword_->setEchoMode(QLineEdit::Password);
    registerPassword_->setPlaceholderText("请输入密码");
    layout->addRow("密码:", registerPassword_);
    
    confirmPassword_ = new QLineEdit;
    confirmPassword_->setEchoMode(QLineEdit::Password);
    confirmPassword_->setPlaceholderText("请确认密码");
    layout->addRow("确认密码:", confirmPassword_);
    
    // Password strength indicator
    passwordStrength_ = new QProgressBar;
    passwordStrength_->setRange(0, 100);
    passwordStrength_->setValue(0);
    strengthLabel_ = new QLabel("密码强度: 无");
    layout->addRow("强度:", passwordStrength_);
    layout->addRow("", strengthLabel_);
    
    phoneEdit_ = new QLineEdit;
    phoneEdit_->setPlaceholderText("选填");
    layout->addRow("手机号:", phoneEdit_);
    
    emailEdit_ = new QLineEdit;
    emailEdit_->setPlaceholderText("选填");
    layout->addRow("邮箱:", emailEdit_);
    
    // Connect password strength checking
    connect(registerPassword_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updatePasswordStrength);
}

void LoginRegisterDialog::onRoleSelectionChanged()
{
    UserRole role = getSelectedRole();
    
    if (role == UserRole::None) {
        roleStatusLabel_->setText("未选择身份");
        roleStatusLabel_->setStyleSheet(R"(
            QLabel {
                background-color: #666666;
                color: #ffffff;
                padding: 8px;
                border-radius: 4px;
                font-weight: bold;
            }
        )");
    } else {
        roleStatusLabel_->setText(role == UserRole::Factory ? "已选择: 工厂客户端" : "已选择: 技术专家客户端");
        roleStatusLabel_->setStyleSheet(R"(
            QLabel {
                background-color: #0078d4;
                color: #ffffff;
                padding: 8px;
                border-radius: 4px;
                font-weight: bold;
            }
        )");
    }
    
    updateButtonStates();
}

void LoginRegisterDialog::onLoginClicked()
{
    if (!validateInputs()) return;
    
    isRegister_ = false;
    emit loginRequested(getUsername(), getPassword(), getSelectedRole());
    accept();
}

void LoginRegisterDialog::onRegisterClicked()
{
    if (!validateInputs()) return;
    
    // Additional validation for registration
    if (registerPassword_->text() != confirmPassword_->text()) {
        QMessageBox::warning(this, "注册错误", "密码和确认密码不匹配！");
        return;
    }
    
    if (registerPassword_->text().length() < 6) {
        QMessageBox::warning(this, "注册错误", "密码长度至少6位！");
        return;
    }
    
    isRegister_ = true;
    emit registerRequested(getUsername(), getPassword(), getSelectedRole());
    accept();
}

void LoginRegisterDialog::onTabChanged(int index)
{
    bool isLoginTab = (index == 0);
    loginButton_->setVisible(isLoginTab);
    registerButton_->setVisible(!isLoginTab);
    updateButtonStates();
}

void LoginRegisterDialog::updatePasswordStrength()
{
    QString password = registerPassword_->text();
    int strength = 0;
    QString strengthText = "无";
    
    if (password.length() >= 6) strength += 25;
    if (password.contains(QRegExp("[a-z]"))) strength += 25;
    if (password.contains(QRegExp("[A-Z]"))) strength += 25;
    if (password.contains(QRegExp("[0-9]"))) strength += 25;
    
    if (strength >= 75) {
        strengthText = "强";
        passwordStrength_->setStyleSheet("QProgressBar::chunk { background-color: #28a745; }");
    } else if (strength >= 50) {
        strengthText = "中";
        passwordStrength_->setStyleSheet("QProgressBar::chunk { background-color: #ffc107; }");
    } else if (strength >= 25) {
        strengthText = "弱";
        passwordStrength_->setStyleSheet("QProgressBar::chunk { background-color: #fd7e14; }");
    } else {
        strengthText = "无";
        passwordStrength_->setStyleSheet("QProgressBar::chunk { background-color: #dc3545; }");
    }
    
    passwordStrength_->setValue(strength);
    strengthLabel_->setText(QString("密码强度: %1").arg(strengthText));
}

void LoginRegisterDialog::updateButtonStates()
{
    bool hasRole = (getSelectedRole() != UserRole::None);
    bool isLoginTab = (tabWidget_->currentIndex() == 0);
    
    if (isLoginTab) {
        bool hasLoginData = !loginUsername_->text().trimmed().isEmpty() && 
                           !loginPassword_->text().isEmpty();
        loginButton_->setEnabled(hasRole && hasLoginData);
    } else {
        bool hasRegisterData = !registerUsername_->text().trimmed().isEmpty() && 
                             !registerPassword_->text().isEmpty();
        registerButton_->setEnabled(hasRole && hasRegisterData);
    }
}

bool LoginRegisterDialog::validateInputs()
{
    if (getSelectedRole() == UserRole::None) {
        QMessageBox::warning(this, "输入错误", "请选择身份类型！");
        return false;
    }
    
    if (getUsername().trimmed().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入用户名！");
        return false;
    }
    
    if (getPassword().isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入密码！");
        return false;
    }
    
    return true;
}

QString LoginRegisterDialog::getUsername() const
{
    bool isLoginTab = (tabWidget_->currentIndex() == 0);
    return isLoginTab ? loginUsername_->text().trimmed() : registerUsername_->text().trimmed();
}

QString LoginRegisterDialog::getPassword() const
{
    bool isLoginTab = (tabWidget_->currentIndex() == 0);
    return isLoginTab ? loginPassword_->text() : registerPassword_->text();
}

UserRole LoginRegisterDialog::getSelectedRole() const
{
    return static_cast<UserRole>(roleGroup_->checkedId());
}

bool LoginRegisterDialog::isRegisterMode() const
{
    return isRegister_;
}