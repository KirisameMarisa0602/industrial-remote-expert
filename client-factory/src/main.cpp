#include <QtWidgets>
#include "mainwindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(720, 480);
    w.show();
    w.startCamera();
    return app.exec();
}
