// src/server/ClientConnection.h
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QPointer>

#include "common/Network/PacketParser.h"
#include "common/Network/Message.h"

namespace bookclub::server {

class ConnectionManager;
class RequestRouter;

class ClientConnection : public QObject {
    Q_OBJECT
public:
    explicit ClientConnection(QTcpSocket* socket,
                              ConnectionManager* manager,
                              RequestRouter* router,
                              QObject* parent = nullptr);
    ~ClientConnection() override;

    // --- Basic Info ---
    qintptr socketDescriptor() const;
    QString clientId() const;
    QString userId() const;
    bool isAuthenticated() const;

    // --- Authentication ---
    void setUserId(const QString& userId);
    void setAuthenticated(bool authenticated);

    // --- Send Message ---
    void sendMessage(const common::Message& message);

    // --- Connection State ---
    bool isConnected() const;

signals:
    void disconnected(const QString& clientId);
    void messageReceived(const common::Message& message);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    void processPendingPackets();
    void sendPacket(const QByteArray& packet);

    qintptr m_socketDescriptor;
    QTcpSocket* m_socket;
    common::PacketParser m_packetParser;

    QString m_clientId;
    QString m_userId;
    bool m_authenticated = false;

    QPointer<ConnectionManager> m_manager;
    QPointer<RequestRouter> m_router;
};

} // namespace bookclub::server
