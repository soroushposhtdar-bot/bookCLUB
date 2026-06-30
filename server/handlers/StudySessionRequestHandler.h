#pragma once

#include "server/RequestHandlerBase.h"

namespace bookclub::server {

class StudySessionRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit StudySessionRequestHandler(QObject* parent = nullptr);
    ~StudySessionRequestHandler() override = default;

    QString actionName() const override;
    QJsonObject handle(const QJsonObject& request) override;
};

} // namespace bookclub::server
