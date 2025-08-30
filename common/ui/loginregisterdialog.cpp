#include "loginregisterdialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QGridLayout>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QValidator>
#include <QIntValidator>

LoginRegisterDialog::LoginRegisterDialog(QWidget *parent)
    : QDialog(parent)
    , tabWidget_(nullptr)
    , loginTab_(nullptr)
    , loginUsername_(nullptr)
    , loginPassword_(nullptr)
    , loginIdentity_(nullptr)
    , loginButton_(nullptr)
    , loginStatusLabel_(nullptr)
    , registerTab_(nullptr)
    , registerUsername_(nullptr)
    , registerPassword_(nullptr)
    , registerConfirmPassword_(nullptr)
    , registerIdentity_(nullptr)
    , registerButton_(nullptr)
    , registerStatusLabel_(nullptr)
    , serverGroup_(nullptr)
    , serverHost_(nullptr)
    , serverPort_(nullptr)
    , showServerSettings_(nullptr)
    , identityStatusLabel_(nullptr)
{
    setupUI();
    setupConnections();
    applyDarkTheme();
    updateButtonStates();
    
    setModal(true);
    setFixedSize(400, 500);
    setWindowTitle("Industrial Remote Expert - 登录/注册");
}

void LoginRegisterDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Identity status label (top-centered, gray when no identity selected)
    identityStatusLabel_ = new QLabel("未选择身份", this);
    identityStatusLabel_->setAlignment(Qt::AlignCenter);
    identityStatusLabel_->setStyleSheet("color: gray; font-weight: bold; padding: 10px;");
    mainLayout->addWidget(identityStatusLabel_);
    
    // Tab widget for Login/Register
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);
    
    // Login Tab
    loginTab_ = new QWidget();
    QFormLayout* loginForm = new QFormLayout(loginTab_);
    
    loginUsername_ = new QLineEdit();
    loginUsername_->setPlaceholderText("输入用户名");
    loginForm->addRow("用户名:", loginUsername_);
    
    loginPassword_ = new QLineEdit();
    loginPassword_->setEchoMode(QLineEdit::Password);
    loginPassword_->setPlaceholderText("输入密码");
    loginForm->addRow("密码:", loginPassword_);
    
    loginIdentity_ = new QComboBox();
    loginIdentity_->addItem("请选择身份", "");  // Empty value for placeholder
    loginIdentity_->addItem("工厂", "factory");
    loginIdentity_->addItem("专家", "expert");
    loginForm->addRow("身份:", loginIdentity_);
    
    loginButton_ = new QPushButton("登录");
    loginButton_->setEnabled(false);
    loginForm->addRow("", loginButton_);
    
    loginStatusLabel_ = new QLabel("");
    loginStatusLabel_->setWordWrap(true);
    loginForm->addRow("", loginStatusLabel_);
    
    tabWidget_->addTab(loginTab_, "登录");
    
    // Register Tab  
    registerTab_ = new QWidget();
    QFormLayout* registerForm = new QFormLayout(registerTab_);
    
    registerUsername_ = new QLineEdit();
    registerUsername_->setPlaceholderText("输入用户名");
    registerForm->addRow("用户名:", registerUsername_);
    
    registerPassword_ = new QLineEdit();
    registerPassword_->setEchoMode(QLineEdit::Password);
    registerPassword_->setPlaceholderText("输入密码");
    registerForm->addRow("密码:", registerPassword_);
    
    registerConfirmPassword_ = new QLineEdit();
    registerConfirmPassword_->setEchoMode(QLineEdit::Password);
    registerConfirmPassword_->setPlaceholderText("确认密码");
    registerForm->addRow("确认密码:", registerConfirmPassword_);
    
    registerIdentity_ = new QComboBox();
    registerIdentity_->addItem("请选择身份", "");  // Empty value for placeholder
    registerIdentity_->addItem("工厂", "factory");
    registerIdentity_->addItem("专家", "expert");
    registerForm->addRow("身份:", registerIdentity_);
    
    registerButton_ = new QPushButton("注册");
    registerButton_->setEnabled(false);
    registerForm->addRow("", registerButton_);
    
    registerStatusLabel_ = new QLabel("");
    registerStatusLabel_->setWordWrap(true);
    registerForm->addRow("", registerStatusLabel_);
    
    tabWidget_->addTab(registerTab_, "注册");
    
    // Server settings (optional, collapsed by default)
    showServerSettings_ = new QCheckBox("显示服务器设置");
    mainLayout->addWidget(showServerSettings_);
    
    serverGroup_ = new QGroupBox("服务器设置");
    serverGroup_->setVisible(false);
    QFormLayout* serverForm = new QFormLayout(serverGroup_);
    
    serverHost_ = new QLineEdit("127.0.0.1");
    serverForm->addRow("主机:", serverHost_);
    
    serverPort_ = new QLineEdit("9000");
    serverPort_->setValidator(new QIntValidator(1, 65535, this));
    serverForm->addRow("端口:", serverPort_);
    
    mainLayout->addWidget(serverGroup_);
    
    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* cancelButton = new QPushButton("取消");
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void LoginRegisterDialog::setupConnections()
{
    // Identity combo changes
    connect(loginIdentity_, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &LoginRegisterDialog::onIdentityChanged);
    connect(registerIdentity_, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &LoginRegisterDialog::onIdentityChanged);
    
    // Button clicks
    connect(loginButton_, &QPushButton::clicked, this, &LoginRegisterDialog::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginRegisterDialog::onRegisterClicked);
    
    // Form field changes to update button states
    connect(loginUsername_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updateButtonStates);
    connect(loginPassword_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updateButtonStates);
    connect(registerUsername_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updateButtonStates);
    connect(registerPassword_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updateButtonStates);
    connect(registerConfirmPassword_, &QLineEdit::textChanged, this, &LoginRegisterDialog::updateButtonStates);
    
    // Server settings toggle
    connect(showServerSettings_, &QCheckBox::toggled, serverGroup_, &QGroupBox::setVisible);
    
    // Enter key handling
    connect(loginPassword_, &QLineEdit::returnPressed, this, &LoginRegisterDialog::onLoginClicked);
    connect(registerConfirmPassword_, &QLineEdit::returnPressed, this, &LoginRegisterDialog::onRegisterClicked);
}

void LoginRegisterDialog::applyDarkTheme()
{
    // Dark theme styling for the dialog
    QString darkStyle = 
        "QDialog {"
        "    background-color: #2b2b2b;"
        "    color: #ffffff;"
        "}"
        "QTabWidget::pane {"
        "    border: 1px solid #3c3c3c;"
        "    background-color: #2b2b2b;"
        "}"
        "QTabBar::tab {"
        "    background-color: #3c3c3c;"
        "    color: #ffffff;"
        "    padding: 8px 16px;"
        "    margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #4a90e2;"
        "}"
        "QLineEdit {"
        "    background-color: #3c3c3c;"
        "    border: 1px solid #555555;"
        "    color: #ffffff;"
        "    padding: 5px;"
        "    border-radius: 3px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #4a90e2;"
        "}"
        "QPushButton {"
        "    background-color: #4a90e2;"
        "    color: #ffffff;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3a7bc8;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #555555;"
        "    color: #888888;"
        "}"
        "QComboBox {"
        "    background-color: #3c3c3c;"
        "    border: 1px solid #555555;"
        "    color: #ffffff;"
        "    padding: 5px;"
        "    border-radius: 3px;"
        "}"
        "QComboBox:drop-down {"
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #3c3c3c;"
        "    color: #ffffff;"
        "    selection-background-color: #4a90e2;"
        "}"
        "QLabel {"
        "    color: #ffffff;"
        "}"
        "QGroupBox {"
        "    color: #ffffff;"
        "    border: 1px solid #555555;"
        "    margin-top: 10px;"
        "    padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    padding: 0 5px;"
        "}"
        "QCheckBox {"
        "    color: #ffffff;"
        "}";
    
    setStyleSheet(darkStyle);
}

void LoginRegisterDialog::onIdentityChanged(const QString& identity)
{
    // Update the top status label based on selected identity
    if (identity.isEmpty() || identity == "请选择身份") {
        identityStatusLabel_->setText("未选择身份");
        identityStatusLabel_->setStyleSheet("color: gray; font-weight: bold; padding: 10px;");
    } else {
        QString role = (identity == "工厂") ? "factory" : "expert";
        QString displayText = (identity == "工厂") ? "已选择身份: 工厂人员" : "已选择身份: 专家";
        identityStatusLabel_->setText(displayText);
        identityStatusLabel_->setStyleSheet("color: #4a90e2; font-weight: bold; padding: 10px;");
    }
    
    updateButtonStates();
}

void LoginRegisterDialog::updateButtonStates()
{
    // Login button: enabled only if username, password and identity are filled
    bool loginValid = !loginUsername_->text().trimmed().isEmpty() &&
                      !loginPassword_->text().isEmpty() &&
                      !loginIdentity_->currentData().toString().isEmpty();
    loginButton_->setEnabled(loginValid);
    
    // Register button: enabled only if all fields are filled and passwords match
    bool registerValid = !registerUsername_->text().trimmed().isEmpty() &&
                         !registerPassword_->text().isEmpty() &&
                         !registerConfirmPassword_->text().isEmpty() &&
                         !registerIdentity_->currentData().toString().isEmpty() &&
                         (registerPassword_->text() == registerConfirmPassword_->text());
    registerButton_->setEnabled(registerValid);
}

bool LoginRegisterDialog::validateLoginForm()
{
    if (loginUsername_->text().trimmed().isEmpty()) {
        loginStatusLabel_->setText("请输入用户名");
        loginStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (loginPassword_->text().isEmpty()) {
        loginStatusLabel_->setText("请输入密码");
        loginStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (loginIdentity_->currentData().toString().isEmpty()) {
        loginStatusLabel_->setText("请选择身份");
        loginStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    
    loginStatusLabel_->setText("正在登录...");
    loginStatusLabel_->setStyleSheet("color: #4a90e2;");
    return true;
}

bool LoginRegisterDialog::validateRegisterForm()
{
    if (registerUsername_->text().trimmed().isEmpty()) {
        registerStatusLabel_->setText("请输入用户名");
        registerStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (registerPassword_->text().isEmpty()) {
        registerStatusLabel_->setText("请输入密码");
        registerStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (registerPassword_->text().length() < 4) {
        registerStatusLabel_->setText("密码至少需要4个字符");
        registerStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (registerPassword_->text() != registerConfirmPassword_->text()) {
        registerStatusLabel_->setText("密码不匹配");
        registerStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    if (registerIdentity_->currentData().toString().isEmpty()) {
        registerStatusLabel_->setText("请选择身份");
        registerStatusLabel_->setStyleSheet("color: red;");
        return false;
    }
    
    registerStatusLabel_->setText("正在注册...");
    registerStatusLabel_->setStyleSheet("color: #4a90e2;");
    return true;
}

void LoginRegisterDialog::onLoginClicked()
{
    if (!validateLoginForm()) {
        return;
    }
    
    QString username = loginUsername_->text().trimmed();
    QString password = loginPassword_->text();
    QString role = loginIdentity_->currentData().toString();
    QString host = serverHost_->text().trimmed();
    int port = serverPort_->text().toInt();
    
    loginButton_->setEnabled(false);
    
    emit loginAttempted(username, password, role, host, port);
}

void LoginRegisterDialog::onRegisterClicked()
{
    if (!validateRegisterForm()) {
        return;
    }
    
    QString username = registerUsername_->text().trimmed();
    QString password = registerPassword_->text();
    QString role = registerIdentity_->currentData().toString();
    QString host = serverHost_->text().trimmed();
    int port = serverPort_->text().toInt();
    
    registerButton_->setEnabled(false);
    
    emit registerAttempted(username, password, role, host, port);
}

void LoginRegisterDialog::onLoginSuccess(const QString& username, const QString& role, const QString& sessionToken)
{
    result_.username = username;
    result_.role = role;
    result_.sessionToken = sessionToken;
    result_.success = true;
    
    loginStatusLabel_->setText("登录成功!");
    loginStatusLabel_->setStyleSheet("color: green;");
    
    accept();  // Close dialog with success
}

void LoginRegisterDialog::onLoginFailed(const QString& error)
{
    loginStatusLabel_->setText(QString("登录失败: %1").arg(error));
    loginStatusLabel_->setStyleSheet("color: red;");
    loginButton_->setEnabled(true);
}

void LoginRegisterDialog::onRegisterSuccess(const QString& message)
{
    registerStatusLabel_->setText(QString("注册成功: %1").arg(message));
    registerStatusLabel_->setStyleSheet("color: green;");
    registerButton_->setEnabled(true);
    
    // Switch to login tab and copy username
    tabWidget_->setCurrentIndex(0);
    loginUsername_->setText(registerUsername_->text());
    loginIdentity_->setCurrentIndex(registerIdentity_->currentIndex());
}

void LoginRegisterDialog::onRegisterFailed(const QString& error)
{
    registerStatusLabel_->setText(QString("注册失败: %1").arg(error));
    registerStatusLabel_->setStyleSheet("color: red;");
    registerButton_->setEnabled(true);
}

void LoginRegisterDialog::setServerDetails(const QString& host, int port)
{
    serverHost_->setText(host);
    serverPort_->setText(QString::number(port));
}