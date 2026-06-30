#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class BookRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit BookRequestHandler(QObject* parent = nullptr);
    ~BookRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
