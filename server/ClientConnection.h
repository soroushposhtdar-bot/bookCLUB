#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QString>

namespace bookclub::server {

class ClientConnection : public QObject {
    Q_OBJECT
public:
    explicit ClientConnection(QObject* parent = nullptr);
    ~ClientConnection() override = default;

    void attachSocket(QTcpSocket* socket);
    QTcpSocket* socket() const;
    QString connectionId() const;
    QString userId() const;
    bool isAuthenticated() const;

    void setConnectionId(const QString& id);
    void setUserId(const QString& userId);
    void setAuthenticated(bool authenticated);

    void sendJson(const QJsonObject& json);
    void disconnectClient();

signals:
    void messageReceived(const QJsonObject& json);
    void disconnected();
    void authenticated(const QString& userId);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket = nullptr;
    QString m_connectionId;
    QString m_userId;
    bool m_authenticated = false;
};

} // namespace bookclub::server
