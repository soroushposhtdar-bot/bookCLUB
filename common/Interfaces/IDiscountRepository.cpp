// common/Interfaces/IDiscountRepository.cpp
#include "common/Interfaces/IDiscountRepository.h"
#include "common/Models/Discount.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

namespace {
Discount* discountFromCurrentRecord(QSqlQuery& q)
{
    auto* d = new Discount;
    d->setId(q.value("id").toString());
    d->setBookId(q.value("bookId").toString());
    d->setType(static_cast<DiscountType>(q.value("type").toInt()));
    d->setValue(q.value("value").toDouble());
    d->setStartsAt(q.value("startsAt").toDateTime());
    d->setEndsAt(q.value("endsAt").toDateTime());
    d->setActive(q.value("isActive").toInt() == 1);
    return d;
}
} // namespace

class DiscountRepositoryImpl : public IDiscountRepository {
public:
    Discount* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Discounts WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return discountFromCurrentRecord(q);
    }

    QVector<Discount*> findByBook(const QString& bookId) const override
    {
        QVector<Discount*> out;
        auto q = DbConnection::run(
            "SELECT * FROM Discounts WHERE bookId = ? ORDER BY startsAt DESC",
            {bookId}
        );
        while (q.next()) out.append(discountFromCurrentRecord(q));
        return out;
    }

    QVector<Discount*> findActiveByBook(const QString& bookId) const override
    {
        QVector<Discount*> out;
        const QDateTime now = QDateTime::currentDateTime();
        auto q = DbConnection::run(
            "SELECT * FROM Discounts WHERE bookId = ? AND isActive = 1 "
            "AND startsAt <= ? AND endsAt > ?",
            {bookId, now, now}
        );
        while (q.next()) {
            out.append(discountFromCurrentRecord(q));
        }
        return out;
    }

    bool save(Discount* discount) override
    {
        if (!discount) return false;
        if (discount->id().isEmpty()) discount->setId(IdGenerator::generateUuid());

        return DbConnection::execOk(
            "INSERT INTO Discounts (id, bookId, type, value, startsAt, endsAt, isActive, createdBy, createdAt) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET type = excluded.type, value = excluded.value, "
            "  startsAt = excluded.startsAt, endsAt = excluded.endsAt, isActive = excluded.isActive",
            {discount->id(), discount->bookId(),
             static_cast<int>(discount->type()),
             discount->value(),
             discount->startsAt(), discount->endsAt(),
             discount->isActive() ? 1 : 0,
             QVariant(), // createdBy — set by handler if available
             QDateTime::currentDateTime()}
        );
    }

    bool remove(const QString& id) override
    {
        return DbConnection::execOk("DELETE FROM Discounts WHERE id = ?", {id});
    }

    bool deactivateExpired() override
    {
        // v15j: also reset Books.discountValue to 0 for expired discounts
        // so the book can receive a new discount. Previously this only set
        // Discounts.isActive = 0 but left Books.discountValue unchanged,
        // so the server's duplicate-discount check always rejected new
        // discounts even after the old one expired.
        DbConnection::execOk(
            "UPDATE Books SET discountValue = 0, updatedAt = ? "
            "WHERE id IN (SELECT bookId FROM Discounts WHERE endsAt < ? AND isActive = 1)",
            {QDateTime::currentDateTime(), QDateTime::currentDateTime()}
        );
        return DbConnection::execOk(
            "UPDATE Discounts SET isActive = 0 WHERE endsAt < ?",
            {QDateTime::currentDateTime()}
        );
    }
};

IDiscountRepository* createDiscountRepository() {
    static DiscountRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
