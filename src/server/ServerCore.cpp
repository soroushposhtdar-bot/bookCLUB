// src/server/ServerCore.cpp
#include "src/server/ServerCore.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/DatabaseManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QCoreApplication>
#include <QDir>

namespace bookclub::server {

ServerCore::ServerCore(QObject* parent)
    : QObject(parent)
{
    m_db = &DatabaseManager::instance();
    m_requestRouter = new RequestRouter(this);
    m_connectionManager = new ConnectionManager(m_requestRouter, this);

    // Connect signals
    connect(m_connectionManager, &ConnectionManager::clientConnected,
            this, &ServerCore::onClientConnected);
    connect(m_connectionManager, &ConnectionManager::clientDisconnected,
            this, &ServerCore::onClientDisconnected);
    connect(m_requestRouter, &RequestRouter::unhandledCommand,
            this, &ServerCore::onUnhandledCommand);
}

ServerCore::~ServerCore()
{
    shutdown();
}

bool ServerCore::initialize(const QString& dbPath, quint16 port)
{
    LOG_INFO("Initializing server...");

    // 1. Setup database
    if (!setupDatabase(dbPath)) {
        LOG_ERROR("Failed to setup database");
        return false;
    }

    // 2. Register default handlers
    registerDefaultHandlers();

    // 3. Start server
    if (!m_connectionManager->startServer(port)) {
        LOG_ERROR("Failed to start server on port " + QString::number(port));
        return false;
    }

    m_port = port;
    m_running = true;
    LOG_INFO("Server initialized successfully on port " + QString::number(port));
    emit serverStarted(port);
    return true;
}

void ServerCore::shutdown()
{
    if (!m_running) return;

    LOG_INFO("Shutting down server...");
    m_connectionManager->stopServer();
    m_running = false;
    emit serverStopped();
    LOG_INFO("Server shutdown complete");
}

bool ServerCore::isRunning() const
{
    return m_running && m_connectionManager && m_connectionManager->isRunning();
}

ConnectionManager* ServerCore::connectionManager() const
{
    return m_connectionManager;
}

RequestRouter* ServerCore::requestRouter() const
{
    return m_requestRouter;
}

quint16 ServerCore::port() const
{
    return m_port;
}

void ServerCore::registerHandler(common::Command command, RequestHandlerBase* handler)
{
    if (m_requestRouter) {
        m_requestRouter->registerHandler(command, handler);
    }
}

bool ServerCore::setupDatabase(const QString& dbPath)
{
    // Ensure directory exists
    QString dbDir = QFileInfo(dbPath).absolutePath();
    if (!common::FileSystemUtils::ensureDirectoryExists(dbDir)) {
        LOG_ERROR("Failed to create database directory: " + dbDir);
        return false;
    }

    if (!m_db->initialize(dbPath)) {
        LOG_ERROR("Failed to initialize database: " + m_db->lastError());
        return false;
    }

    LOG_INFO("Database initialized: " + dbPath);
    return true;
}

void ServerCore::registerDefaultHandlers()
{
    // TODO: Register default handlers here
    // For example:
    // registerHandler(common::Command::Login, new AuthHandler(this));
    // registerHandler(common::Command::Register, new AuthHandler(this));
    // registerHandler(common::Command::GetHomeSections, new BookHandler(this));
    // ...
    LOG_INFO("Default handlers registered (TODOs)");
}

void ServerCore::onClientConnected(const QString& clientId)
{
    LOG_INFO("Client connected: " + clientId);
}

void ServerCore::onClientDisconnected(const QString& clientId)
{
    LOG_INFO("Client disconnected: " + clientId);
}

void ServerCore::onUnhandledCommand(common::Command command, const QString& clientId)
{
    LOG_WARNING("Unhandled command " + common::commandToString(command) +
                " from client " + clientId);
}

} // namespace bookclub::server
