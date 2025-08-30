#include <QtWidgets>
#include <QApplication>
#include <QResource>
#include <QFile>
#include "mainwindow.h"
#include "loginregisterdialog.h"
#include "clientconn.h"

class LoginManager : public QObject
{
    Q_OBJECT
public:
    LoginManager(QObject* parent = nullptr) : QObject(parent), conn_(nullptr), mainWindow_(nullptr) {}

    int exec() {
        // Apply dark theme
        QResource::registerResource(":/themes");
        QFile themeFile(":/themes/theme-dark.qss");
        if (themeFile.open(QIODevice::ReadOnly)) {
            QString themeStyle = QString::fromUtf8(themeFile.readAll());
            qApp->setStyleSheet(themeStyle);
        }

        // Show login dialog
        LoginRegisterDialog loginDialog;
        connect(&loginDialog, &LoginRegisterDialog::loginAttempted, this, &LoginManager::onLoginAttempted);
        connect(&loginDialog, &LoginRegisterDialog::registerAttempted, this, &LoginManager::onRegisterAttempted);
        
        if (loginDialog.exec() == QDialog::Accepted) {
            LoginResult result = loginDialog.getResult();
            if (result.success) {
                // Check if role matches client
                QString expectedRole = QApplication::applicationName().contains("expert") ? "expert" : "factory";
                if (result.role != expectedRole) {
                    QMessageBox::critical(nullptr, "Role Mismatch", 
                        QString("This client is for %1 users only. You logged in as %2.").arg(expectedRole, result.role));
                    return 1;
                }
                
                // Create main window with authenticated session
                mainWindow_ = new MainWindow(nullptr);
                mainWindow_->setWindowTitle(QString("Industrial Remote Expert - %1 User: %2")
                    .arg(result.role.toUpper(), result.username));
                mainWindow_->resize(720, 480);
                mainWindow_->show();
                mainWindow_->startCamera();
                return qApp->exec();
            }
        }
        return 0;
    }

private slots:
    void onLoginAttempted(const QString& username, const QString& password, const QString& role, const QString& host, int port) {
        if (!conn_) {
            conn_ = new ClientConn(this);
            connect(conn_, &ClientConn::sigPkt, this, &LoginManager::onServerResponse);
            connect(conn_, &ClientConn::sigConnected, this, &LoginManager::onConnected);
        }
        
        pendingUsername_ = username;
        pendingPassword_ = password;
        pendingRole_ = role;
        isRegisterAttempt_ = false;
        
        conn_->connectTo(host, port);
    }
    
    void onRegisterAttempted(const QString& username, const QString& password, const QString& role, const QString& host, int port) {
        if (!conn_) {
            conn_ = new ClientConn(this);
            connect(conn_, &ClientConn::sigPkt, this, &LoginManager::onServerResponse);
            connect(conn_, &ClientConn::sigConnected, this, &LoginManager::onConnected);
        }
        
        pendingUsername_ = username;
        pendingPassword_ = password;
        pendingRole_ = role;
        isRegisterAttempt_ = true;
        
        conn_->connectTo(host, port);
    }
    
    void onConnected() {
        QJsonObject data{{"username", pendingUsername_}, {"password", pendingPassword_}, {"role", pendingRole_}};
        if (isRegisterAttempt_) {
            conn_->send(MSG_REGISTER, data);
        } else {
            conn_->send(MSG_LOGIN, data);
        }
    }
    
    void onServerResponse(Packet p) {
        if (p.type == MSG_SERVER_EVENT) {
            int code = p.json.value("code").toInt();
            QString message = p.json.value("message").toString();
            
            // Find the dialog by iterating through top-level widgets
            LoginRegisterDialog* dialog = nullptr;
            foreach (QWidget* widget, QApplication::topLevelWidgets()) {
                dialog = qobject_cast<LoginRegisterDialog*>(widget);
                if (dialog) break;
            }
            
            if (isRegisterAttempt_) {
                if (code == 0) {
                    if (dialog) dialog->onRegisterSuccess(message);
                } else {
                    if (dialog) dialog->onRegisterFailed(message);
                }
            } else {
                if (code == 0) {
                    QString token = p.json.value("token").toString();
                    QString username = p.json.value("username").toString();
                    QString role = p.json.value("role").toString();
                    if (dialog) dialog->onLoginSuccess(username, role, token);
                } else {
                    if (dialog) dialog->onLoginFailed(message);
                }
            }
        }
    }

private:
    ClientConn* conn_;
    MainWindow* mainWindow_;
    QString pendingUsername_;
    QString pendingPassword_;
    QString pendingRole_;
    bool isRegisterAttempt_;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("client-expert");
    
    LoginManager manager;
    return manager.exec();
}

#include "main.moc"
