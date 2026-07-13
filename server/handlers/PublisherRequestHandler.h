#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class PublisherRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit PublisherRequestHandler(QObject* parent = nullptr);
    ~PublisherRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
