#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class LibraryRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit LibraryRequestHandler(QObject* parent = nullptr);
    ~LibraryRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
