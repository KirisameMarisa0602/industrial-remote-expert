#include <QtWidgets>
#include "loginwindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Industrial Remote Expert");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("Industrial Remote Expert");
    
    LoginWindow w;
    w.show();
    
    return app.exec();
}
