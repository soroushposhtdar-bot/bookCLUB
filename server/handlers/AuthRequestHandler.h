#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class AuthRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit AuthRequestHandler(QObject* parent = nullptr);
    ~AuthRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
