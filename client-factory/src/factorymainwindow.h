#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>

class ClientConn; // Forward declaration

class FactoryMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FactoryMainWindow(const QString& username, const QString& role, QWidget *parent = nullptr);

private slots:
    void onNavigationChanged();
    void onLogout();
    void onCreateTicket();
    void onRefreshDashboard();

private:
    void setupUI();
    void setupNavigation();
    void setupViews();
    void setupToolbar();
    void setupStatusBar();
    
    // View creation methods
    QWidget* createDashboardView();
    QWidget* createTicketsView();
    QWidget* createMeetingView();
    QWidget* createTelemetryView();
    QWidget* createChatView();
    QWidget* createKnowledgeBaseView();
    
    void updateDashboardData();
    
    QString username_;
    QString role_;
    
    // UI Components
    QSplitter* mainSplitter_;
    QListWidget* navigationList_;
    QStackedWidget* contentStack_;
    QToolBar* toolbar_;
    QStatusBar* statusBar_;
    
    // Views
    QWidget* dashboardView_;
    QWidget* ticketsView_;
    QWidget* meetingView_;
    QWidget* telemetryView_;
    QWidget* chatView_;
    QWidget* knowledgeBaseView_;
    
    // Dashboard components
    QLabel* statusLabel_;
    QTextEdit* logView_;
    QTimer* refreshTimer_;
    
    // Charts for dashboard
    QtCharts::QChartView* tempChart_;
    QtCharts::QChartView* pressureChart_;
    QtCharts::QLineSeries* tempSeries_;
    QtCharts::QLineSeries* pressureSeries_;
    
    // Network connection (future)
    ClientConn* connection_;
};