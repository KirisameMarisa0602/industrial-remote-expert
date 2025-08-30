#include "logindialog.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    applyTheme();
    setFixedSize(400, 350);
    setWindowTitle("Industrial Remote Expert - 登录");
    setModal(true);
}

void LoginDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Title
    auto titleLabel = new QLabel("工业远程专家系统");
    titleLabel->setProperty("class", "title");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Tab widget for login/register
    tabWidget_ = new QTabWidget();
    mainLayout->addWidget(tabWidget_);
    
    // Login tab
    setupLoginTab();
    
    // Register tab  
    setupRegisterTab();
    
    mainLayout->addStretch();
}

void LoginDialog::setupLoginTab()
{
    loginTab_ = new QWidget();
    auto layout = new QVBoxLayout(loginTab_);
    
    auto formLayout = new QFormLayout();
    
    // Username
    loginUsername_ = new QLineEdit();
    loginUsername_->setPlaceholderText("请输入用户名");
    formLayout->addRow("用户名:", loginUsername_);
    
    // Password
    loginPassword_ = new QLineEdit();
    loginPassword_->setEchoMode(QLineEdit::Password);
    loginPassword_->setPlaceholderText("请输入密码");
    formLayout->addRow("密码:", loginPassword_);
    
    // Identity selection
    loginIdentity_ = new QComboBox();
    loginIdentity_->setProperty("class", "identity-selector");
    loginIdentity_->addItem("", ""); // Empty option
    loginIdentity_->addItem("工厂", "factory");
    loginIdentity_->addItem("专家", "expert");
    formLayout->addRow("身份:", loginIdentity_);
    
    layout->addLayout(formLayout);
    
    // Identity warning label
    loginIdentityWarning_ = new QLabel("未选择身份");
    loginIdentityWarning_->setProperty("class", "identity-warning");
    loginIdentityWarning_->setAlignment(Qt::AlignCenter);
    layout->addWidget(loginIdentityWarning_);
    
    // Login button
    loginSubmit_ = new QPushButton("登录");
    loginSubmit_->setProperty("class", "primary");
    loginSubmit_->setEnabled(false);
    layout->addWidget(loginSubmit_);
    
    // Connect signals
    connect(loginIdentity_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onIdentityChanged);
    connect(loginUsername_, &QLineEdit::textChanged, this, &LoginDialog::updateSubmitButtons);
    connect(loginPassword_, &QLineEdit::textChanged, this, &LoginDialog::updateSubmitButtons);
    connect(loginSubmit_, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    
    tabWidget_->addTab(loginTab_, "登录");
}

void LoginDialog::setupRegisterTab()
{
    registerTab_ = new QWidget();
    auto layout = new QVBoxLayout(registerTab_);
    
    auto formLayout = new QFormLayout();
    
    // Username
    registerUsername_ = new QLineEdit();
    registerUsername_->setPlaceholderText("请输入用户名");
    formLayout->addRow("用户名:", registerUsername_);
    
    // Password
    registerPassword_ = new QLineEdit();
    registerPassword_->setEchoMode(QLineEdit::Password);
    registerPassword_->setPlaceholderText("请输入密码");
    formLayout->addRow("密码:", registerPassword_);
    
    // Confirm password
    registerPasswordConfirm_ = new QLineEdit();
    registerPasswordConfirm_->setEchoMode(QLineEdit::Password);
    registerPasswordConfirm_->setPlaceholderText("请再次输入密码");
    formLayout->addRow("确认密码:", registerPasswordConfirm_);
    
    // Identity selection
    registerIdentity_ = new QComboBox();
    registerIdentity_->setProperty("class", "identity-selector");
    registerIdentity_->addItem("", ""); // Empty option
    registerIdentity_->addItem("工厂", "factory");
    registerIdentity_->addItem("专家", "expert");
    formLayout->addRow("身份:", registerIdentity_);
    
    layout->addLayout(formLayout);
    
    // Identity warning label
    registerIdentityWarning_ = new QLabel("未选择身份");
    registerIdentityWarning_->setProperty("class", "identity-warning");
    registerIdentityWarning_->setAlignment(Qt::AlignCenter);
    layout->addWidget(registerIdentityWarning_);
    
    // Register button
    registerSubmit_ = new QPushButton("注册");
    registerSubmit_->setProperty("class", "primary");
    registerSubmit_->setEnabled(false);
    layout->addWidget(registerSubmit_);
    
    // Connect signals
    connect(registerIdentity_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onIdentityChanged);
    connect(registerUsername_, &QLineEdit::textChanged, this, &LoginDialog::updateSubmitButtons);
    connect(registerPassword_, &QLineEdit::textChanged, this, &LoginDialog::updateSubmitButtons);
    connect(registerPasswordConfirm_, &QLineEdit::textChanged, this, &LoginDialog::updateSubmitButtons);
    connect(registerSubmit_, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    
    tabWidget_->addTab(registerTab_, "注册");
}

void LoginDialog::onIdentityChanged()
{
    updateSubmitButtons();
    
    // Update warning labels
    QComboBox* identity = qobject_cast<QComboBox*>(sender());
    if (identity == loginIdentity_) {
        loginIdentityWarning_->setVisible(loginIdentity_->currentData().toString().isEmpty());
    } else if (identity == registerIdentity_) {
        registerIdentityWarning_->setVisible(registerIdentity_->currentData().toString().isEmpty());
    }
}

void LoginDialog::updateSubmitButtons()
{
    // Login tab
    bool loginValid = !loginUsername_->text().trimmed().isEmpty() &&
                     !loginPassword_->text().isEmpty() &&
                     !loginIdentity_->currentData().toString().isEmpty();
    loginSubmit_->setEnabled(loginValid);
    
    // Register tab
    bool registerValid = !registerUsername_->text().trimmed().isEmpty() &&
                        !registerPassword_->text().isEmpty() &&
                        !registerPasswordConfirm_->text().isEmpty() &&
                        registerPassword_->text() == registerPasswordConfirm_->text() &&
                        !registerIdentity_->currentData().toString().isEmpty();
    registerSubmit_->setEnabled(registerValid);
}

void LoginDialog::onLoginClicked()
{
    if (loginIdentity_->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "错误", "请选择身份");
        return;
    }
    
    result_.isRegister = false;
    result_.username = loginUsername_->text().trimmed();
    result_.password = loginPassword_->text();
    result_.role = loginIdentity_->currentData().toString();
    
    accept();
}

void LoginDialog::onRegisterClicked()
{
    if (registerIdentity_->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "错误", "请选择身份");
        return;
    }
    
    if (registerPassword_->text() != registerPasswordConfirm_->text()) {
        QMessageBox::warning(this, "错误", "密码不一致");
        return;
    }
    
    result_.isRegister = true;
    result_.username = registerUsername_->text().trimmed();
    result_.password = registerPassword_->text();
    result_.role = registerIdentity_->currentData().toString();
    
    accept();
}

void LoginDialog::applyTheme()
{
    // Load dark theme
    QFile themeFile(":/styles/theme-dark.qss");
    if (themeFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&themeFile);
        QString styleSheet = stream.readAll();
        setStyleSheet(styleSheet);
    }
}