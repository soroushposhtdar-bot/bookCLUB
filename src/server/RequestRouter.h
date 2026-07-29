// src/server/RequestRouter.h
#pragma once

#include <QObject>
#include <QMap>
#include <functional>

#include "common/Network/Protocol.h"
#include "common/Network/Message.h"

namespace bookclub::server {

class ClientConnection;
class RequestHandlerBase;

class RequestRouter : public QObject {
    Q_OBJECT
public:
    explicit RequestRouter(QObject* parent = nullptr);
    ~RequestRouter() override;

    // --- Registration ---
    void registerHandler(common::Command command, RequestHandlerBase* handler);
    void registerDefaultHandler(RequestHandlerBase* handler);

    // --- Routing ---
    void handleRequest(const common::Message& message, ClientConnection* client);

signals:
    void unhandledCommand(common::Command command, const QString& clientId);

private:
    QMap<common::Command, RequestHandlerBase*> m_handlers;
    RequestHandlerBase* m_defaultHandler = nullptr;
};

} // namespace bookclub::server
