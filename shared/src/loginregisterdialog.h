#pragma once

#include <QtWidgets>

enum class UserRole {
    None = 0,     // 未选择身份
    Factory = 1,  // 工厂客户端  
    Expert = 2    // 技术专家客户端
};

class LoginRegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginRegisterDialog(QWidget *parent = nullptr);
    
    QString getUsername() const;
    QString getPassword() const;
    UserRole getSelectedRole() const;
    bool isRegisterMode() const;

signals:
    void loginRequested(const QString& username, const QString& password, UserRole role);
    void registerRequested(const QString& username, const QString& password, UserRole role);

private slots:
    void onRoleSelectionChanged();
    void onLoginClicked();
    void onRegisterClicked();
    void onTabChanged(int index);
    void updatePasswordStrength();

private:
    void setupUI();
    void setupLoginTab();
    void setupRegisterTab();
    void updateButtonStates();
    bool validateInputs();
    
    // UI components
    QTabWidget* tabWidget_;
    
    // Login tab
    QWidget* loginTab_;
    QLineEdit* loginUsername_;
    QLineEdit* loginPassword_;
    
    // Register tab  
    QWidget* registerTab_;
    QLineEdit* registerUsername_;
    QLineEdit* registerPassword_;
    QLineEdit* confirmPassword_;
    QLineEdit* phoneEdit_;
    QLineEdit* emailEdit_;
    QProgressBar* passwordStrength_;
    QLabel* strengthLabel_;
    
    // Role selection (shared)
    QLabel* roleStatusLabel_;   // Shows "未选择身份" when no role selected
    QRadioButton* roleNone_;
    QRadioButton* roleFactory_;
    QRadioButton* roleExpert_;
    QButtonGroup* roleGroup_;
    
    // Action buttons
    QPushButton* loginButton_;
    QPushButton* registerButton_;
    QPushButton* cancelButton_;
    
    bool isRegister_;
};