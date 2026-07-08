// common/interfaces/IOrderRepository.cpp
#include "common/interfaces/IOrderRepository.h"
#include "common/models/Order.h"
#include "common/utils/IdGenerator.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>

namespace bookclub::common {

class OrderRepositoryImpl : public IOrderRepository {
public:
    OrderRepositoryImpl() = default;
    ~OrderRepositoryImpl() override = default;

    bool save(Order* order) override {
        if (!order) return false;
        if (order->id().isEmpty()) {
            order->setId(IdGenerator::generateUuid());
        }

        // Serialize items to JSON
        QJsonArray itemsArray;
        for (const auto* item : order->items()) {
            QJsonObject itemObj;
            itemObj["bookId"] = item->bookId();
            itemObj["title"] = item->title();
            itemObj["unitPrice"] = item->unitPrice();
            itemObj["quantity"] = item->quantity();
            itemsArray.append(itemObj);
        }
        QJsonDocument itemsDoc(itemsArray);

        QString sql = R"(
            INSERT INTO Orders (
                id, userId, items, subtotal, discountTotal, finalTotal,
                paid, completed, createdAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getQuery(sql, {
            order->id(),
            order->userId(),
            QString(itemsDoc.toJson(QJsonDocument::Compact)),
            order->subtotal(),
            order->discountTotal(),
            order->finalTotal(),
            order->isPaid() ? 1 : 0,
            order->isCompleted() ? 1 : 0,
            QDateTime::currentDateTime()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    bool update(Order* order) override {
        if (!order || order->id().isEmpty()) return false;

        // Serialize items to JSON
        QJsonArray itemsArray;
        for (const auto* item : order->items()) {
            QJsonObject itemObj;
            itemObj["bookId"] = item->bookId();
            itemObj["title"] = item->title();
            itemObj["unitPrice"] = item->unitPrice();
            itemObj["quantity"] = item->quantity();
            itemsArray.append(itemObj);
        }
        QJsonDocument itemsDoc(itemsArray);

        QString sql = R"(
            UPDATE Orders SET
                items = ?,
                subtotal = ?,
                discountTotal = ?,
                finalTotal = ?,
                paid = ?,
                completed = ?
            WHERE id = ?
        )";

        QSqlQuery query = getQuery(sql, {
            QString(itemsDoc.toJson(QJsonDocument::Compact)),
            order->subtotal(),
            order->discountTotal(),
            order->finalTotal(),
            order->isPaid() ? 1 : 0,
            order->isCompleted() ? 1 : 0,
            order->id()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    Order* findById(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT * FROM Orders WHERE id = ?", {id});
        if (query.next()) {
            return createOrderFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Order*> findByUser(const QString& userId) const override {
        QVector<Order*> orders;
        QSqlQuery query = getQuery(
            "SELECT * FROM Orders WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            orders.append(createOrderFromCurrentRecord(query));
        }
        return orders;
    }

    QVector<Order*> findByPublisher(const QString& publisherId) const override {
        QVector<Order*> orders;
        // Join with Books to filter by publisher
        QString sql = R"(
            SELECT DISTINCT o.* FROM Orders o
            JOIN OrderItems oi ON o.id = oi.orderId
            JOIN Books b ON oi.bookId = b.id
            WHERE b.publisherId = ?
            ORDER BY o.createdAt DESC
        )";
        QSqlQuery query = getQuery(sql, {publisherId});
        while (query.next()) {
            orders.append(createOrderFromCurrentRecord(query));
        }
        return orders;
    }

    QVector<Order*> findAll() const override {
        QVector<Order*> orders;
        QSqlQuery query = getQuery("SELECT * FROM Orders ORDER BY createdAt DESC");
        while (query.next()) {
            orders.append(createOrderFromCurrentRecord(query));
        }
        return orders;
    }

    int totalSalesCount() const override {
        QSqlQuery query = getQuery("SELECT COUNT(*) FROM Orders WHERE paid = 1 AND completed = 1");
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

private:
    // ====== Helper methods ======
    QSqlQuery getQuery(const QString& sql, const QVariantList& params = {}) const {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);
        query.prepare(sql);
        for (const auto& param : params) {
            query.addBindValue(param);
        }
        query.exec();
        return query;
    }

    QSqlDatabase getDatabase() const {
        static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if (!db.isOpen()) {
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        return db;
    }

    Order* createOrderFromCurrentRecord(const QSqlQuery& query) const {
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

        // Parse items from JSON
        QString itemsJson = rec.value("items").toString();
        if (!itemsJson.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(itemsJson.toUtf8());
            if (doc.isArray()) {
                for (const auto& val : doc.array()) {
                    QJsonObject obj = val.toObject();
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

// ========== Factory function ==========
IOrderRepository* createOrderRepository() {
    static OrderRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
