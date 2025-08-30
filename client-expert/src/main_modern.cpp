#include <QApplication>
#include <QFile>
#include "modernmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Expert Client - Modern");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Industrial Remote Expert");
    
    ModernMainWindow window;
    window.show();
    
    return app.exec();
}