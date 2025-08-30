#include <QCoreApplication>
#include <QTextStream>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "roomhub.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Industrial Remote Expert Server");
    app.setApplicationVersion("2.0");
    
    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Industrial Remote Expert Server");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Port to listen on (default: 9000)",
                                  "port", "9000");
    parser.addOption(portOption);
    
    parser.process(app);
    
    // Parse port
    bool ok;
    quint16 port = parser.value(portOption).toUShort(&ok);
    if (!ok || port == 0) {
        QTextStream(stderr) << "Invalid port number: " << parser.value(portOption) << Qt::endl;
        return 1;
    }
    
    // Create and start room hub
    RoomHub hub;
    if (!hub.start(port)) {
        QTextStream(stderr) << "Failed to start server on port " << port << Qt::endl;
        return 1;
    }
    
    QTextStream(stdout) << "Industrial Remote Expert Server v2.0" << Qt::endl;
    QTextStream(stdout) << "Server listening on \"0.0.0.0\" : " << port << Qt::endl;
    QTextStream(stdout) << "Usage: clients connect to server_ip: " << port << Qt::endl;
    
    return app.exec();
}