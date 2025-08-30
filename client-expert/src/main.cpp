#include <QtWidgets>
#include <QFile>
#include <QTextStream>
#include "mainwindow.h"
#include "expertmainwindow.h"
#include "logindialog.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    // Load global dark theme
    QFile themeFile(":/styles/theme-dark.qss");
    if (themeFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&themeFile);
        QString styleSheet = stream.readAll();
        app.setStyleSheet(styleSheet);
    }
    
    // Show login dialog first
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        return 0; // User cancelled login
    }
    
    LoginDialog::LoginResult result = loginDialog.getResult();
    
    QMainWindow* window = nullptr;
    
    // Route based on role
    if (result.role == "expert") {
        window = new ExpertMainWindow(result.username, result.role);
    } else {
        // For this client, even factory users will use the old interface for now
        // In a real system, you might redirect to factory client
        window = new ExpertMainWindow(result.username, result.role);
    }
    
    window->resize(1200, 800);
    window->show();
    
    int exitCode = app.exec();
    delete window;
    return exitCode;
}
