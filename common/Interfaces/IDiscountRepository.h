// common/Interfaces/IDiscountRepository.h
//
// Repository for time-boxed Discounts on books.
#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

namespace bookclub::common {

class Discount;

class IDiscountRepository {
public:
    virtual ~IDiscountRepository() = default;

    virtual Discount* findById(const QString& id) const = 0;
    virtual QVector<Discount*> findByBook(const QString& bookId) const = 0;
    virtual QVector<Discount*> findActiveByBook(const QString& bookId) const = 0;
    virtual bool save(Discount* discount) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool deactivateExpired() = 0;        // housekeeping
};

IDiscountRepository* createDiscountRepository();

} // namespace bookclub::common
