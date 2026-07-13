#pragma once

#include <QObject>
#include <QHash>
#include <QVector>

namespace bookclub::server {

class ClientConnection;

class ConnectionManager : public QObject {
    Q_OBJECT
public:
    explicit ConnectionManager(QObject* parent = nullptr);
    ~ConnectionManager() override = default;

    void registerConnection(ClientConnection* connection);
    void unregisterConnection(const QString& connectionId);
    ClientConnection* connection(const QString& connectionId) const;
    QVector<ClientConnection*> allConnections() const;
    QVector<ClientConnection*> authenticatedConnections() const;
    int onlineCount() const;

signals:
    void connectionCountChanged(int count);
    void clientConnected(const QString& connectionId);
    void clientDisconnected(const QString& connectionId);

private:
    QHash<QString, ClientConnection*> m_connections;
};

} // namespace bookclub::server
