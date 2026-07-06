// client/main.cpp
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
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

    // --- Setup Logger ---
    FileSystemUtils::ensureDirectoryExists("logs");
    Logger::instance().setLogFile("logs/client.log");
    LOG_INFO("=== BookClub Client Started (Headless Mode) ===");

    // --- Connect to Server ---
    auto& network = ClientNetworkManager::instance();
    qDebug() << "Connecting to server at 127.0.0.1:8080...";
    LOG_INFO("Connecting to server at 127.0.0.1:8080");

    if (!network.connectToServer("127.0.0.1", 8080)) {
        LOG_ERROR("Failed to connect to server. Exiting.");
        qDebug() << "Failed to connect to server. Exiting.";
        return 1;
    }

    qDebug() << "Connected to server successfully!";
    LOG_INFO("Connected to server successfully");

    // --- Create Auth Controller ---
    AuthController auth;

    // --- Set Login Credentials (Sample) ---
    // Replace with your own test credentials
    QString testUsername = "testuser";
    QString testPassword = "123456";
    auth.setLoginCredentials(testUsername, testPassword);

    qDebug() << "Attempting to login with username:" << testUsername;
    LOG_INFO("Attempting to login with username: " + testUsername);

    // --- Connect Signals ---
    QObject::connect(&auth, &AuthController::loginSucceeded, [&]() {
        auto& session = SessionManager::instance();
        qDebug() << "✅ Login successful!";
        qDebug() << "   User ID   :" << session.userId();
        qDebug() << "   Username  :" << session.username();
        qDebug() << "   Display   :" << session.displayName();
        qDebug() << "   Role      :" << session.roleName();

        LOG_INFO("Login successful for user: " + session.username() +
                 " (ID: " + session.userId() + ", Role: " + session.roleName() + ")");

        // Quit after 1 second to allow logs to be written
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    });

    QObject::connect(&auth, &AuthController::loginFailed, [&](const QString& error) {
        qDebug() << "❌ Login failed:" << error;
        LOG_ERROR("Login failed: " + error);

        // Quit after 1 second
        QTimer::singleShot(1000, &app, &QCoreApplication::quit);
    });

    QObject::connect(&auth, &AuthController::busyChanged, [&](bool busy) {
        qDebug() << "Busy state changed:" << (busy ? "Processing..." : "Idle");
    });

    // --- Perform Login ---
    auth.login();

    qDebug() << "Waiting for server response...";
    LOG_INFO("Waiting for server response...");

    // --- Run Event Loop ---
    int result = app.exec();

    LOG_INFO("Client application finished. Exit code: " + QString::number(result));
    qDebug() << "Client application finished with exit code:" << result;

    return result;
}
