#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

struct LoginResult {
    QString username;
    QString role;           // "factory" or "expert"
    QString sessionToken;
    bool success = false;
};

class LoginRegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginRegisterDialog(QWidget *parent = nullptr);
    
    LoginResult getResult() const { return result_; }
    
    // Set default server connection details
    void setServerDetails(const QString& host, int port);

signals:
    void loginAttempted(const QString& username, const QString& password, const QString& role, const QString& host, int port);
    void registerAttempted(const QString& username, const QString& password, const QString& role, const QString& host, int port);

public slots:
    void onLoginSuccess(const QString& username, const QString& role, const QString& sessionToken);
    void onLoginFailed(const QString& error);
    void onRegisterSuccess(const QString& message);
    void onRegisterFailed(const QString& error);

private slots:
    void onIdentityChanged(const QString& identity);
    void onLoginClicked();
    void onRegisterClicked();
    void updateButtonStates();

private:
    void setupUI();
    void setupConnections();
    void applyDarkTheme();
    bool validateLoginForm();
    bool validateRegisterForm();

    // UI Components
    QTabWidget* tabWidget_;
    
    // Login tab
    QWidget* loginTab_;
    QLineEdit* loginUsername_;
    QLineEdit* loginPassword_;
    QComboBox* loginIdentity_;
    QPushButton* loginButton_;
    QLabel* loginStatusLabel_;
    
    // Register tab  
    QWidget* registerTab_;
    QLineEdit* registerUsername_;
    QLineEdit* registerPassword_;
    QLineEdit* registerConfirmPassword_;
    QComboBox* registerIdentity_;
    QPushButton* registerButton_;
    QLabel* registerStatusLabel_;
    
    // Common server settings
    QGroupBox* serverGroup_;
    QLineEdit* serverHost_;
    QLineEdit* serverPort_;
    QCheckBox* showServerSettings_;
    
    // Identity status
    QLabel* identityStatusLabel_;
    
    LoginResult result_;
};