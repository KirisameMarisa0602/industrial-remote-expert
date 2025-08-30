#pragma once

#include <QtWidgets>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>

/**
 * 统一的登录/注册界面组件
 * 包含用户身份选择(工厂/专家)和"未选择身份"状态验证
 */
class AuthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AuthWidget(QWidget *parent = nullptr);

    // 获取表单数据
    QString getUsername() const;
    QString getPassword() const;
    QString getSelectedRole() const;
    
    // 验证表单
    bool validateForm();
    
    // 清除表单
    void clearForm();

signals:
    void loginRequested(const QString& username, const QString& password, const QString& role);
    void registerRequested(const QString& username, const QString& password, const QString& role);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onRoleChanged(const QString& role);

private:
    void setupUI();
    void updateButtonState();

    QLineEdit *usernameEdit_;
    QLineEdit *passwordEdit_;
    QComboBox *roleComboBox_;
    QPushButton *loginButton_;
    QPushButton *registerButton_;
    QLabel *roleStatusLabel_;  // 显示"未选择身份"状态
    
    bool isRoleSelected_;
};