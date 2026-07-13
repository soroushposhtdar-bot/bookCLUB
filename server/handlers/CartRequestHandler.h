#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class CartRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit CartRequestHandler(QObject* parent = nullptr);
    ~CartRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
