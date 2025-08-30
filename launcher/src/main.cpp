#include <QApplication>
#include <QFile>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Industrial Remote Expert Launcher");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Industrial Remote Expert");
    
    // 加载深色主题
    QFile themeFile(":/styles/theme-dark.qss");
    if (themeFile.open(QFile::ReadOnly)) {
        QString theme = QLatin1String(themeFile.readAll());
        app.setStyleSheet(theme);
    }
    
    MainWindow window;
    window.show();
    
    return app.exec();
}