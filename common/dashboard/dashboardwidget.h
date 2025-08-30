#pragma once

#include <QtWidgets>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QTableWidget>
#include <QSplitter>

/**
 * 仪表盘卡片组件
 * 显示系统状态、统计信息等
 */
class DashboardCard : public QFrame
{
    Q_OBJECT

public:
    explicit DashboardCard(const QString &title, QWidget *parent = nullptr);
    
    void setTitle(const QString &title);
    void setValue(const QString &value);
    void setValueColor(const QColor &color);
    void setIcon(const QPixmap &icon);
    
    // 内容区域，可以添加自定义控件
    QVBoxLayout* contentLayout() { return contentLayout_; }

private:
    void setupUI();
    
    QLabel *titleLabel_;
    QLabel *valueLabel_;
    QLabel *iconLabel_;
    QVBoxLayout *contentLayout_;
};

/**
 * 主仪表盘组件
 * 包含多个卡片和状态显示
 */
class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    
    // 添加/获取仪表盘卡片
    void addCard(DashboardCard *card, int row, int col);
    DashboardCard* getCard(const QString &title);
    
    // 更新状态信息
    void updateConnectionStatus(bool connected);
    void updateParticipantCount(int count);
    void updateActiveRoom(const QString &roomId);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void cardClicked(const QString &title);

private:
    void setupUI();
    void createDefaultCards();
    
    QGridLayout *cardLayout_;
    QHash<QString, DashboardCard*> cards_;
};