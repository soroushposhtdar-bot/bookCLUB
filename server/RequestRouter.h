#pragma once

#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QString>

namespace bookclub::server {

class RequestRouter : public QObject {
    Q_OBJECT
public:
    explicit RequestRouter(QObject* parent = nullptr);
    ~RequestRouter() override = default;

    void route(const QJsonObject& request);
    void registerHandler(const QString& actionName, QObject* handler);

signals:
    void requestRouted(const QString& actionName);
    void requestRejected(const QString& reason);
    void requestProcessed(const QJsonObject& response);

private:
    QHash<QString, QObject*> m_handlers;
};

} // namespace bookclub::server
