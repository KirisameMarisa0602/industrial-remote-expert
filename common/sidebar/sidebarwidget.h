#pragma once

#include <QtWidgets>
#include <QFrame>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>

/**
 * 侧边栏导航组件
 * 提供主要功能区域的导航
 */
class SidebarWidget : public QFrame
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    
    // 添加导航项
    void addNavigationItem(const QString &name, const QString &text, const QString &icon = QString());
    
    // 设置当前活动项
    void setActiveItem(const QString &name);
    
    // 获取当前活动项
    QString activeItem() const;

signals:
    void navigationChanged(const QString &name);

private slots:
    void onButtonClicked();

private:
    void setupUI();
    void updateButtonStates();
    
    QVBoxLayout *layout_;
    QButtonGroup *buttonGroup_;
    QHash<QString, QPushButton*> buttons_;
    QString activeItem_;
};