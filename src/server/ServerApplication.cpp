// src/server/ServerApplication.cpp
#include "src/server/ServerApplication.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QSignalMapper>
#include <QFile>
#include <csignal>

namespace bookclub::server {

// --- Signal Handlers for Graceful Shutdown ---
static ServerApplication* g_appInstance = nullptr;

void signalHandler(int signal)
{
    if (g_appInstance) {
        LOG_INFO("Received signal: " + QString::number(signal) + ". Shutting down...");
        g_appInstance->stop();
    }
}

ServerApplication::ServerApplication(QObject* parent)
    : QObject(parent)
{
    g_appInstance = this;
}

ServerApplication::~ServerApplication()
{
    g_appInstance = nullptr;
    if (m_server) {
        m_server->shutdown();
        delete m_server;
        m_server = nullptr;
    }
    if (m_app) {
        m_app->quit();
        delete m_app;
        m_app = nullptr;
    }
}

int ServerApplication::run(int argc, char* argv[])
{
    // Create application
    m_app = new QCoreApplication(argc, argv);
    m_app->setApplicationName("BookClub Server");
    m_app->setApplicationVersion("1.0.0");

    // Parse command line arguments
    if (!parseArguments(m_app->arguments())) {
        return 1;
    }

    if (m_showHelp) {
        return 0;
    }

    // Setup logging
    QString logPath = "logs/server.log";
    common::FileSystemUtils::ensureDirectoryExists("logs");
    common::Logger::instance().setLogFile(logPath);
    LOG_INFO("=== BookClub Server Starting ===");

    // Setup signal handlers for graceful shutdown
    setupSignalHandlers();

    // Create and initialize server
    m_server = new ServerCore(this);
    connect(m_server, &ServerCore::serverStarted, this, &ServerApplication::onServerStarted);
    connect(m_server, &ServerCore::errorOccurred, this, &ServerApplication::onServerError);

    if (!m_server->initialize(m_dbPath, m_port)) {
        LOG_ERROR("Failed to initialize server");
        return 1;
    }

    // Shutdown timer for graceful exit (if needed)
    m_shutdownTimer = new QTimer(this);
    m_shutdownTimer->setSingleShot(true);
    connect(m_shutdownTimer, &QTimer::timeout, this, &ServerApplication::onShutdownTimer);

    LOG_INFO("Server application ready");
    emit applicationReady();

    return m_app->exec();
}

void ServerApplication::stop()
{
    LOG_INFO("Stopping server application...");
    if (m_server) {
        m_server->shutdown();
    }
    if (m_app) {
        m_app->quit();
    }
}

bool ServerApplication::parseArguments(const QStringList& arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("BookClub Server - Powerful e-book management server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dbOption(
        QStringList() << "d" << "database",
        "Path to SQLite database file (default: bookclub.db)",
        "path"
    );
    parser.addOption(dbOption);

    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "Server port (default: 8080)",
        "port"
    );
    parser.addOption(portOption);

    if (!parser.parse(arguments)) {
        qCritical() << "Command line parse error:" << parser.errorText();
        return false;
    }

    if (parser.isSet("help")) {
        parser.showHelp();
        m_showHelp = true;
        return true;
    }

    if (parser.isSet(dbOption)) {
        m_dbPath = parser.value(dbOption);
    }

    if (parser.isSet(portOption)) {
        bool ok;
        int port = parser.value(portOption).toInt(&ok);
        if (ok && port > 0 && port < 65536) {
            m_port = static_cast<quint16>(port);
        } else {
            qCritical() << "Invalid port number:" << parser.value(portOption);
            return false;
        }
    }

    LOG_INFO("Configuration loaded:");
    LOG_INFO("  Database: " + m_dbPath);
    LOG_INFO("  Port: " + QString::number(m_port));

    return true;
}

void ServerApplication::setupSignalHandlers()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
#ifndef Q_OS_WIN
    signal(SIGQUIT, signalHandler);
#endif
}

void ServerApplication::onServerStarted(quint16 port)
{
    LOG_INFO("Server started successfully on port " + QString::number(port));
}

void ServerApplication::onServerError(const QString& message)
{
    LOG_ERROR("Server error: " + message);
}

void ServerApplication::onShutdownTimer()
{
    LOG_INFO("Shutdown timer triggered. Stopping...");
    stop();
}

} // namespace bookclub::server
