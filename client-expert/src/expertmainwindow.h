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
#include <QTableWidget>

class ClientConn; // Forward declaration

class ExpertMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ExpertMainWindow(const QString& username, const QString& role, QWidget *parent = nullptr);

private slots:
    void onNavigationChanged();
    void onLogout();
    void onJoinTicket();
    void onRefreshTickets();

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
    QWidget* createDiagnosticsView();
    QWidget* createChatView();
    QWidget* createKnowledgeBaseView();
    
    void updateTicketsList();
    
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
    QWidget* diagnosticsView_;
    QWidget* chatView_;
    QWidget* knowledgeBaseView_;
    
    // Components
    QLabel* statusLabel_;
    QTextEdit* logView_;
    QTableWidget* ticketsTable_;
    QTimer* refreshTimer_;
    
    // Network connection (future)
    ClientConn* connection_;
};