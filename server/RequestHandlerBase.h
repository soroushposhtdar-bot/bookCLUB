#pragma once

#include <QObject>
#include <QJsonObject>

namespace bookclub::server {

class RequestHandlerBase : public QObject {
    Q_OBJECT
public:
    explicit RequestHandlerBase(QObject* parent = nullptr);
    ~RequestHandlerBase() override = default;

    virtual QString actionName() const = 0;
    virtual QJsonObject handle(const QJsonObject& request) = 0;
};

} // namespace bookclub::server
