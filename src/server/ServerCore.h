// src/server/ServerCore.h
#pragma once

#include <QObject>
#include <QPointer>

#include "common/Network/Protocol.h"

namespace bookclub::server {

class ConnectionManager;
class RequestRouter;
class RequestHandlerBase;
class DatabaseManager;

class ServerCore : public QObject {
    Q_OBJECT
public:
    explicit ServerCore(QObject* parent = nullptr);
    ~ServerCore() override;

    // --- Lifecycle ---
    bool initialize(const QString& dbPath, quint16 port = 8080);
    void shutdown();
    bool isRunning() const;

    // --- Getters ---
    ConnectionManager* connectionManager() const;
    RequestRouter* requestRouter() const;
    quint16 port() const;

    // --- Registration ---
    void registerHandler(common::Command command, RequestHandlerBase* handler);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void errorOccurred(const QString& message);

private slots:
    void onClientConnected(const QString& clientId);
    void onClientDisconnected(const QString& clientId);
    void onUnhandledCommand(common::Command command, const QString& clientId);

private:
    bool setupDatabase(const QString& dbPath);
    void registerDefaultHandlers();

    QPointer<DatabaseManager> m_db;
    QPointer<ConnectionManager> m_connectionManager;
    QPointer<RequestRouter> m_requestRouter;

    quint16 m_port = 0;
    bool m_running = false;
};

} // namespace bookclub::server
