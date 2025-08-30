#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "video/FrameGrabberFilter.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<FrameGrabberFilter>("App.Video", 1, 0, "FrameGrabberFilter");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}