#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class Order;

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual bool save(Order* order) = 0;
    virtual bool update(Order* order) = 0;
    virtual Order* findById(const QString& id) const = 0;
    virtual QVector<Order*> findByUser(const QString& userId) const = 0;
    virtual QVector<Order*> findByPublisher(const QString& publisherId) const = 0;
    virtual QVector<Order*> findAll() const = 0;
    virtual int totalSalesCount() const = 0;
};

} // namespace bookclub::common
