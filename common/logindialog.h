#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QTabWidget>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    struct LoginResult {
        bool isRegister = false;
        QString username;
        QString password;
        QString role; // "factory" or "expert"
    };

    explicit LoginDialog(QWidget *parent = nullptr);
    
    LoginResult getResult() const { return result_; }

private slots:
    void onIdentityChanged();
    void onLoginClicked();
    void onRegisterClicked();
    void updateSubmitButtons();

private:
    void setupUI();
    void setupLoginTab();
    void setupRegisterTab();
    void applyTheme();
    
    QTabWidget *tabWidget_;
    
    // Login tab
    QWidget *loginTab_;
    QLineEdit *loginUsername_;
    QLineEdit *loginPassword_;
    QComboBox *loginIdentity_;
    QPushButton *loginSubmit_;
    QLabel *loginIdentityWarning_;
    
    // Register tab
    QWidget *registerTab_;
    QLineEdit *registerUsername_;
    QLineEdit *registerPassword_;
    QLineEdit *registerPasswordConfirm_;
    QComboBox *registerIdentity_;
    QPushButton *registerSubmit_;
    QLabel *registerIdentityWarning_;
    
    LoginResult result_;
};