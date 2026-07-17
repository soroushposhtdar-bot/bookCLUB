// common/Interfaces/IOrderRepository.cpp
//
// Concrete `IOrderRepository` backed by the shared SQLite database.
// Fixes from the previous version:
//   - Dropped the JOIN on a non-existent `OrderItems` table in
//     `findByPublisher`. Order items are stored as JSON inside `Orders.items`,
//     so we filter publisher orders by parsing that JSON in C++ instead.
//   - Added missing `<QJsonObject>` include so `QJsonObject` is complete.
//   - Use shared "bookclub_shared" connection.

#include "common/Interfaces/IOrderRepository.h"
#include "common/Models/Order.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
QSqlDatabase sharedDatabase()
{
    auto db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }
    db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
    db.setDatabaseName("bookclub.db");
    db.open();
    return db;
}

QSqlQuery runQuery(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery query(sharedDatabase());
    query.prepare(sql);
    for (const auto& p : params) {
        query.addBindValue(p);
    }
    query.exec();
    return query;
}

bool execOk(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery q = runQuery(sql, params);
    if (q.lastError().type() != QSqlError::NoError) {
        LOG_ERROR("OrderRepository SQL error: " + q.lastError().text());
        LOG_ERROR("SQL: " + sql);
        return false;
    }
    return true;
}

QJsonArray itemsToJson(const QVector<OrderItem*>& items)
{
    QJsonArray arr;
    for (const OrderItem* item : items) {
        QJsonObject obj;
        obj["bookId"] = item->bookId();
        obj["title"] = item->title();
        obj["unitPrice"] = item->unitPrice();
        obj["quantity"] = item->quantity();
        arr.append(obj);
    }
    return arr;
}
} // namespace

// ========== Implementation ==========
class OrderRepositoryImpl : public IOrderRepository {
public:
    OrderRepositoryImpl() = default;
    ~OrderRepositoryImpl() override = default;

    bool save(Order* order) override
    {
        if (!order) return false;
        if (order->id().isEmpty()) {
            order->setId(IdGenerator::generateUuid());
        }
        if (!order->createdAt().isValid()) {
            order->setCreatedAt(QDateTime::currentDateTime());
        }

        const QString sql = R"(
            INSERT INTO Orders (
                id, userId, items, subtotal, discountTotal, finalTotal,
                paid, completed, createdAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        const QString itemsJson = QString::fromUtf8(
            QJsonDocument(itemsToJson(order->items())).toJson(QJsonDocument::Compact)
        );

        return execOk(sql, {
            order->id(),
            order->userId(),
            itemsJson,
            order->subtotal(),
            order->discountTotal(),
            order->finalTotal(),
            order->isPaid() ? 1 : 0,
            order->isCompleted() ? 1 : 0,
            order->createdAt()
        });
    }

    bool update(Order* order) override
    {
        if (!order || order->id().isEmpty()) return false;

        const QString sql = R"(
            UPDATE Orders SET
                items = ?,
                subtotal = ?,
                discountTotal = ?,
                finalTotal = ?,
                paid = ?,
                completed = ?
            WHERE id = ?
        )";

        const QString itemsJson = QString::fromUtf8(
            QJsonDocument(itemsToJson(order->items())).toJson(QJsonDocument::Compact)
        );

        return execOk(sql, {
            itemsJson,
            order->subtotal(),
            order->discountTotal(),
            order->finalTotal(),
            order->isPaid() ? 1 : 0,
            order->isCompleted() ? 1 : 0,
            order->id()
        });
    }

    Order* findById(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Orders WHERE id = ?", {id});
        if (query.next()) {
            return createOrderFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Order*> findByUser(const QString& userId) const override
    {
        QVector<Order*> orders;
        QSqlQuery query = runQuery(
            "SELECT * FROM Orders WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            orders.append(createOrderFromCurrentRecord(query));
        }
        return orders;
    }

    QVector<Order*> findByPublisher(const QString& publisherId) const override
    {
        // No `OrderItems` table exists; orders store their items as JSON.
        // We load all orders and filter by publisher using the books we know
        // belong to this publisher.
        QSet<QString> publisherBookIds;
        {
            QSqlQuery bookQuery = runQuery(
                "SELECT id FROM Books WHERE publisherId = ?",
                {publisherId}
            );
            while (bookQuery.next()) {
                publisherBookIds.insert(bookQuery.value(0).toString());
            }
        }

        QVector<Order*> orders;
        QSqlQuery query = runQuery("SELECT * FROM Orders ORDER BY createdAt DESC");
        while (query.next()) {
            const QString itemsJson = query.record().value("items").toString();
            const QJsonArray items = QJsonDocument::fromJson(itemsJson.toUtf8()).array();
            bool match = false;
            for (const auto& v : items) {
                const QString bookId = v.toObject().value("bookId").toString();
                if (publisherBookIds.contains(bookId)) {
                    match = true;
                    break;
                }
            }
            if (match) {
                orders.append(createOrderFromCurrentRecord(query));
            }
        }
        return orders;
    }

    QVector<Order*> findAll() const override
    {
        QVector<Order*> orders;
        QSqlQuery query = runQuery("SELECT * FROM Orders ORDER BY createdAt DESC");
        while (query.next()) {
            orders.append(createOrderFromCurrentRecord(query));
        }
        return orders;
    }

    int totalSalesCount() const override
    {
        QSqlQuery query = runQuery("SELECT COUNT(*) FROM Orders WHERE paid = 1 AND completed = 1");
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

private:
    Order* createOrderFromCurrentRecord(QSqlQuery& query) const
    {
        QSqlRecord rec = query.record();
        auto* order = new Order;
        order->setId(rec.value("id").toString());
        order->setUserId(rec.value("userId").toString());
        order->setSubtotal(rec.value("subtotal").toDouble());
        order->setDiscountTotal(rec.value("discountTotal").toDouble());
        order->setFinalTotal(rec.value("finalTotal").toDouble());
        order->setPaid(rec.value("paid").toInt() == 1);
        order->setCompleted(rec.value("completed").toInt() == 1);
        order->setCreatedAt(rec.value("createdAt").toDateTime());

        const QString itemsJson = rec.value("items").toString();
        if (!itemsJson.isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(itemsJson.toUtf8());
            if (doc.isArray()) {
                for (const auto& val : doc.array()) {
                    const QJsonObject obj = val.toObject();
                    auto* item = new OrderItem;
                    item->setBookId(obj["bookId"].toString());
                    item->setTitle(obj["title"].toString());
                    item->setUnitPrice(obj["unitPrice"].toDouble());
                    item->setQuantity(obj["quantity"].toInt());
                    order->addItem(item);
                }
            }
        }
        return order;
    }
};

// ========== Factory ==========
IOrderRepository* createOrderRepository() {
    static OrderRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
