#pragma once

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QAbstractSocket>

#include "common/AppEnums.h"

namespace bookclub::client {

class ClientNetworkManager : public QObject {
    Q_OBJECT
public:
    explicit ClientNetworkManager(QObject* parent = nullptr);
    ~ClientNetworkManager() override = default;

    bool connectToServer(const QHostAddress& address, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendRequest(const QJsonObject& request);
    void sendEvent(const QJsonObject& event);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString& message);
    void requestSent(const QJsonObject& request);
    void messageReceived(const QJsonObject& message);
    void notificationReceived(const QJsonObject& notification);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket = nullptr;
    QByteArray m_buffer;
};

} // namespace bookclub::client
