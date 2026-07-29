// common/Interfaces/IOrderRepository.cpp
//
// SQLite-backed IOrderRepository.
// Uses the new normalised Orders + OrderItems tables.
#include "common/Interfaces/IOrderRepository.h"
#include "common/Models/Order.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
Order* orderFromCurrentRecord(QSqlQuery& q)
{
    auto* order = new Order;
    order->setId(q.value("id").toString());
    order->setUserId(q.value("userId").toString());
    order->setSubtotal(q.value("subtotal").toDouble());
    order->setDiscountTotal(q.value("discountTotal").toDouble());
    order->setFinalTotal(q.value("finalTotal").toDouble());
    order->setPaid(q.value("paid").toInt() == 1);
    order->setCompleted(q.value("completed").toInt() == 1);
    order->setCreatedAt(q.value("createdAt").toDateTime());

    // Load items
    auto items = DbConnection::run(
        "SELECT * FROM OrderItems WHERE orderId = ?",
        {order->id()}
    );
    while (items.next()) {
        auto* item = new OrderItem;
        item->setBookId(items.value("bookId").toString());
        item->setTitle(items.value("bookTitle").toString());
        item->setUnitPrice(items.value("unitPrice").toDouble());
        item->setQuantity(items.value("quantity").toInt());
        order->addItem(item);
    }
    return order;
}
} // namespace

// ============== Implementation ==============
class OrderRepositoryImpl : public IOrderRepository {
public:
    bool save(Order* order) override
    {
        if (!order) return false;
        if (order->id().isEmpty()) order->setId(IdGenerator::generateUuid());
        if (!order->createdAt().isValid()) order->setCreatedAt(QDateTime::currentDateTime());

        const QString sql = QStringLiteral(
            "INSERT INTO Orders (id, userId, subtotal, discountTotal, finalTotal,"
            "  paid, completed, createdAt) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        const bool ok = DbConnection::execOk(sql, {
            order->id(), order->userId(),
            order->subtotal(), order->discountTotal(), order->finalTotal(),
            order->isPaid() ? 1 : 0,
            order->isCompleted() ? 1 : 0,
            order->createdAt()
        });

        if (!ok) {
            LOG_ERROR("Failed to save order: " + order->id()
                      + " | " + DbConnection::lastErrorText());
            return false;
        }

        // Persist items
        for (const OrderItem* item : order->items()) {
            const double lineTotal = item->unitPrice() * item->quantity();
            DbConnection::execOk(
                "INSERT INTO OrderItems (id, orderId, bookId, bookTitle, unitPrice,"
                "  discountAmount, quantity, lineTotal) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                {
                    IdGenerator::generateUuid(),
                    order->id(),
                    item->bookId(),
                    item->title(),
                    item->unitPrice(),
                    0.0,
                    item->quantity(),
                    lineTotal
                }
            );
        }
        return true;
    }

    bool update(Order* order) override
    {
        if (!order || order->id().isEmpty()) return false;
        return DbConnection::execOk(
            "UPDATE Orders SET subtotal=?, discountTotal=?, finalTotal=?, paid=?, completed=? "
            "WHERE id=?",
            {
                order->subtotal(), order->discountTotal(), order->finalTotal(),
                order->isPaid() ? 1 : 0,
                order->isCompleted() ? 1 : 0,
                order->id()
            }
        );
    }

    Order* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Orders WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return orderFromCurrentRecord(q);
    }

    QVector<Order*> findByUser(const QString& userId) const override
    {
        QVector<Order*> orders;
        auto q = DbConnection::run(
            "SELECT * FROM Orders WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (q.next()) orders.append(orderFromCurrentRecord(q));
        return orders;
    }

    QVector<Order*> findByPublisher(const QString& publisherId) const override
    {
        // Find orders that contain at least one book by this publisher.
        QVector<Order*> orders;
        const QString sql = QStringLiteral(
            "SELECT DISTINCT o.* FROM Orders o "
            "JOIN OrderItems oi ON oi.orderId = o.id "
            "JOIN Books b ON b.id = oi.bookId "
            "WHERE b.publisherId = ? "
            "ORDER BY o.createdAt DESC"
        );
        auto q = DbConnection::run(sql, {publisherId});
        while (q.next()) orders.append(orderFromCurrentRecord(q));
        return orders;
    }

    QVector<Order*> findAll() const override
    {
        QVector<Order*> orders;
        auto q = DbConnection::run("SELECT * FROM Orders ORDER BY createdAt DESC");
        while (q.next()) orders.append(orderFromCurrentRecord(q));
        return orders;
    }

    int totalSalesCount() const override
    {
        auto q = DbConnection::run(
            "SELECT COALESCE(SUM(quantity), 0) FROM OrderItems"
        );
        return q.next() ? q.value(0).toInt() : 0;
    }
};

// ============== Factory ==============
IOrderRepository* createOrderRepository() {
    static OrderRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
