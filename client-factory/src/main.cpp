#include <QtWidgets>
#include <QFile>
#include <QTextStream>
#include "mainwindow.h"
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
    
    MainWindow w;
    w.setWindowTitle(QString("Industrial Remote Expert - %1 (%2)").arg(
        result.role == "factory" ? "工厂端" : "专家端", result.username));
    w.resize(720, 480);
    w.show();
    w.startCamera();
    return app.exec();
}
