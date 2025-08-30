#include <QtCore>
#include <QtNetwork>
#include <QLoggingCategory>
#include "roomhub.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("Industrial Remote Expert Server");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Industrial Remote Expert");

    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Industrial Remote Expert Server - MVP Foundation");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption portOpt(QStringList() << "p" << "port", "Listen port", "port", "9000");
    QCommandLineOption dbOpt(QStringList() << "d" << "database", "Database file path", "path");
    QCommandLineOption verboseOpt("verbose", "Enable verbose logging");
    QCommandLineOption debugOpt("debug", "Enable debug logging");
    QCommandLineOption heartbeatOpt("heartbeat", "Heartbeat interval in seconds", "seconds", "30");
    QCommandLineOption timeoutOpt("timeout", "Heartbeat timeout in seconds", "seconds", "90");
    
    parser.addOption(portOpt);
    parser.addOption(dbOpt);
    parser.addOption(verboseOpt);
    parser.addOption(debugOpt);
    parser.addOption(heartbeatOpt);
    parser.addOption(timeoutOpt);
    
    parser.process(app);

    // Configure logging
    if (parser.isSet(debugOpt)) {
        QLoggingCategory::setFilterRules("*.debug=true");
    } else if (parser.isSet(verboseOpt)) {
        QLoggingCategory::setFilterRules("protocol.debug=false\nnetwork.debug=false\n*.debug=true");
    } else {
        QLoggingCategory::setFilterRules("*.debug=false");
    }

    // Parse options
    quint16 port = parser.value(portOpt).toUShort();
    QString dbPath = parser.value(dbOpt);
    int heartbeatInterval = parser.value(heartbeatOpt).toInt();
    int heartbeatTimeout = parser.value(timeoutOpt).toInt();

    if (port == 0) {
        qCritical() << "Invalid port number:" << parser.value(portOpt);
        return 1;
    }

    // Create and configure server
    RoomHub hub;
    hub.setHeartbeatInterval(heartbeatInterval);
    hub.setHeartbeatTimeout(heartbeatTimeout);
    
    // Start server
    if (!hub.start(port, dbPath)) {
        qCritical() << "Failed to start server";
        return 1;
    }

    qInfo() << "Industrial Remote Expert Server started";
    qInfo() << "Listen port:" << port;
    qInfo() << "Database:" << hub.database()->getDatabasePath();
    qInfo() << "Heartbeat interval:" << heartbeatInterval << "seconds";
    qInfo() << "Heartbeat timeout:" << heartbeatTimeout << "seconds";
    qInfo() << "";
    qInfo() << "Usage: clients connect to server_ip:" << port;
    qInfo() << "Press Ctrl+C to stop the server";

    // Handle graceful shutdown
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&hub]() {
        qInfo() << "Shutting down server...";
        hub.stop();
    });

    return app.exec();
}
