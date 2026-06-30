#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class AdminRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit AdminRequestHandler(QObject* parent = nullptr);
    ~AdminRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
