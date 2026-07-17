// client/main.cpp
//
// Headless client entry point. Used as a smoke test for the backend.
// Usage:
//   BookClubClient [--host HOST] [--port PORT]
//                  [--user USERNAME] [--pass PASSWORD]
//                  [--register USERNAME PASSWORD DISPLAY EMAIL PHONE Q A]
//
// Defaults: host=127.0.0.1 port=8080 user=amir pass=amir1234
//
// When the UI is ready, this binary will likely be replaced by a Qt Widgets
// or QML application, but the network layer and controllers remain the same.

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QCommandLineParser>
#include <QThread>

#include "src/client/network/ClientNetworkManager.h"
#include "src/client/controllers/AuthController.h"
#include "src/client/session/SessionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

using namespace bookclub::client;
using namespace bookclub::common;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("BookClub Client");
    app.setApplicationVersion("1.0.0");

    // --- Parse command-line arguments ---
    QCommandLineParser parser;
    parser.setApplicationDescription("BookClub headless client");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption hostOpt({"H", "host"}, "Server host (default 127.0.0.1)", "host", "127.0.0.1");
    QCommandLineOption portOpt({"p", "port"}, "Server port (default 8080)", "port", "8080");
    QCommandLineOption userOpt({"u", "user"}, "Username", "username", "amir");
    QCommandLineOption passOpt({"s", "pass"}, "Password", "password", "amir1234");
    parser.addOption(hostOpt);
    parser.addOption(portOpt);
    parser.addOption(userOpt);
    parser.addOption(passOpt);
    parser.process(app);

    const QString host = parser.value(hostOpt);
    const quint16 port = static_cast<quint16>(parser.value(portOpt).toUShort());
    const QString username = parser.value(userOpt);
    const QString password = parser.value(passOpt);

    // --- Setup Logger ---
    FileSystemUtils::ensureDirectoryExists("logs");
    Logger::instance().setLogFile("logs/client.log");
    LOG_INFO("=== BookClub Client Started (Headless Mode) ===");

    // --- Connect to Server ---
    auto& network = ClientNetworkManager::instance();
    qDebug().noquote() << "Connecting to server at" << host << ":" << port << "...";
    LOG_INFO("Connecting to server at " + host + ":" + QString::number(port));

    if (!network.connectToServer(host, port)) {
        LOG_ERROR("Failed to connect to server. Exiting.");
        qDebug() << "Failed to connect to server. Exiting.";
        return 1;
    }

    qDebug() << "Connected to server successfully!";
    LOG_INFO("Connected to server successfully");

    // --- Create Auth Controller ---
    AuthController auth;
    auth.setLoginCredentials(username, password);

    qDebug().noquote() << "Attempting to login with username:" << username;
    LOG_INFO("Attempting to login with username: " + username);

    // --- Connect Signals ---
    QObject::connect(&auth, &AuthController::loginSucceeded, [&]() {
        auto& session = SessionManager::instance();
        qDebug().noquote() << "Login successful!"
                           << "  User ID  :" << session.userId()
                           << "  Username :" << session.username()
                           << "  Display  :" << session.displayName()
                           << "  Role     :" << session.roleName();

        LOG_INFO("Login successful for user: " + session.username() +
                 " (ID: " + session.userId() + ", Role: " + session.roleName() + ")");

        QTimer::singleShot(500, &app, &QCoreApplication::quit);
    });

    QObject::connect(&auth, &AuthController::loginFailed, [&](const QString& error) {
        qDebug().noquote() << "Login failed:" << error;
        LOG_ERROR("Login failed: " + error);
        QTimer::singleShot(500, &app, &QCoreApplication::quit);
    });

    QObject::connect(&auth, &AuthController::busyChanged, [&](bool busy) {
        qDebug().noquote() << "Busy:" << (busy ? "Processing..." : "Idle");
    });

    // --- Perform Login ---
    auth.login();
    qDebug() << "Waiting for server response...";
    LOG_INFO("Waiting for server response...");

    int result = app.exec();
    LOG_INFO("Client application finished. Exit code: " + QString::number(result));
    qDebug() << "Client application finished with exit code:" << result;
    return result;
}
