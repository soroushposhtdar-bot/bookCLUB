#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class NotificationRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit NotificationRequestHandler(QObject* parent = nullptr);
    ~NotificationRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
