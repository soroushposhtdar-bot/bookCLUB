// =============================================================================
//  ShelfDto.h / PurchaseDto.h / NotificationDto.h
// =============================================================================
//  QObject wrappers for shelves, purchase-history rows and notifications.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QQmlEngine>

#include "services/MockTypes.h"

namespace bookclub::client {

class ShelfDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id           READ id           CONSTANT)
    Q_PROPERTY(QString name         READ name         CONSTANT)
    Q_PROPERTY(QString description  READ description  CONSTANT)
    Q_PROPERTY(QStringList bookIds  READ bookIds      CONSTANT)
    Q_PROPERTY(int      bookCount   READ bookCount    CONSTANT)
    Q_PROPERTY(QString createdAtText READ createdAtText CONSTANT)
    Q_PROPERTY(QString color        READ color        CONSTANT)
    Q_PROPERTY(bool     favorite    READ favorite     CONSTANT)
    Q_PROPERTY(bool     isPrivate   READ isPrivate    CONSTANT)
    Q_PROPERTY(int      order       READ order        CONSTANT)

public:
    explicit ShelfDto(QObject* parent = nullptr) : QObject(parent) {}
    explicit ShelfDto(const MockShelf& s, QObject* parent = nullptr);

    QString id() const { return m_s.id; }
    QString name() const { return m_s.name; }
    QString description() const { return m_s.description; }
    QStringList bookIds() const { return m_s.bookIds; }
    int bookCount() const { return m_s.bookIds.size(); }
    QString createdAtText() const {
        return m_s.createdAt.toString(QStringLiteral("MMM d, yyyy"));
    }
    QString color() const { return m_s.color; }
    bool favorite() const { return m_s.favorite; }
    bool isPrivate() const { return m_s.isPrivate; }
    int order() const { return m_s.order; }

private:
    MockShelf m_s;
};

// -----------------------------------------------------------------------------

class PurchaseDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString orderId      READ orderId      CONSTANT)
    Q_PROPERTY(QString dateText     READ dateText     CONSTANT)
    Q_PROPERTY(QString relativeDate READ relativeDate CONSTANT)
    Q_PROPERTY(QStringList bookIds  READ bookIds      CONSTANT)
    Q_PROPERTY(QStringList bookTitles READ bookTitles CONSTANT)
    Q_PROPERTY(QString titlesSummary READ titlesSummary CONSTANT)
    Q_PROPERTY(int      itemCount   READ itemCount    CONSTANT)
    Q_PROPERTY(double   total       READ total        CONSTANT)
    Q_PROPERTY(double   discountTotal READ discountTotal CONSTANT)
    Q_PROPERTY(QString  totalText   READ totalText    CONSTANT)
    Q_PROPERTY(QString  discountText READ discountText CONSTANT)

public:
    explicit PurchaseDto(QObject* parent = nullptr) : QObject(parent) {}
    explicit PurchaseDto(const MockPurchase& p, QObject* parent = nullptr);

    QString orderId() const { return m_p.orderId; }
    QString dateText() const {
        return m_p.date.toString(QStringLiteral("MMM d, yyyy"));
    }
    QString relativeDate() const {
        int days = m_p.date.daysTo(QDateTime::currentDateTime());
        if (days < 1)  return QStringLiteral("Today");
        if (days < 2)  return QStringLiteral("Yesterday");
        if (days < 30) return QStringLiteral("%1 days ago").arg(days);
        return QStringLiteral("%1 months ago").arg(days / 30);
    }
    QStringList bookIds() const { return m_p.bookIds; }
    QStringList bookTitles() const { return m_p.bookTitles; }
    QString titlesSummary() const {
        if (m_p.bookTitles.isEmpty()) return QStringLiteral("No items");
        if (m_p.bookTitles.size() == 1) return m_p.bookTitles.first();
        return m_p.bookTitles.first() + QStringLiteral(" + %1 more").arg(m_p.bookTitles.size() - 1);
    }
    int itemCount() const { return m_p.itemCount; }
    double total() const { return m_p.total; }
    double discountTotal() const { return m_p.discountTotal; }
    QString totalText() const {
        return QStringLiteral("$%1").arg(m_p.total, 0, 'f', 2);
    }
    QString discountText() const {
        if (m_p.discountTotal <= 0.0) return QStringLiteral("No discount");
        return QStringLiteral("Saved $%1").arg(m_p.discountTotal, 0, 'f', 2);
    }

private:
    MockPurchase m_p;
};

// -----------------------------------------------------------------------------

class NotificationDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id     READ id     CONSTANT)
    Q_PROPERTY(QString type   READ type   CONSTANT)
    Q_PROPERTY(QString category READ category CONSTANT)
    Q_PROPERTY(QString title  READ title  CONSTANT)
    Q_PROPERTY(QString body   READ body   CONSTANT)
    Q_PROPERTY(QString bookId READ bookId CONSTANT)
    Q_PROPERTY(bool    read   READ read   NOTIFY readChanged)
    Q_PROPERTY(bool    archived READ archived CONSTANT)
    Q_PROPERTY(QString relativeTime READ relativeTime CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString accentColor READ accentColor CONSTANT)

public:
    explicit NotificationDto(QObject* parent = nullptr) : QObject(parent) {}
    explicit NotificationDto(const MockNotification& n, QObject* parent = nullptr);

    QString id() const { return m_n.id; }
    QString type() const { return m_n.type; }
    QString category() const { return m_n.category; }
    QString title() const { return m_n.title; }
    QString body() const { return m_n.body; }
    QString bookId() const { return m_n.bookId; }
    bool read() const { return m_n.read; }
    bool archived() const { return m_n.archived; }

    QString relativeTime() const {
        auto secs = m_n.createdAt.secsTo(QDateTime::currentDateTime());
        if (secs < 60)    return QStringLiteral("just now");
        if (secs < 3600)  return QStringLiteral("%1m ago").arg(secs / 60);
        if (secs < 86400) return QStringLiteral("%1h ago").arg(secs / 3600);
        int days = secs / 86400;
        if (days < 7)     return QStringLiteral("%1d ago").arg(days);
        if (days < 30)    return QStringLiteral("%1w ago").arg(days / 7);
        return QStringLiteral("%1mo ago").arg(days / 30);
    }

    QString iconName() const {
        if (m_n.type == QStringLiteral("NewBookInFavoriteGenre")) return QStringLiteral("new_releases");
        if (m_n.type == QStringLiteral("DiscountOnSavedBook"))    return QStringLiteral("local_offer");
        if (m_n.type == QStringLiteral("SaleRegistered"))         return QStringLiteral("shopping_bag");
        if (m_n.type == QStringLiteral("NewReview"))              return QStringLiteral("rate_review");
        if (m_n.type == QStringLiteral("NewRating"))              return QStringLiteral("star");
        if (m_n.type == QStringLiteral("SystemAlert"))            return QStringLiteral("campaign");
        if (m_n.type == QStringLiteral("PublisherUpdate"))        return QStringLiteral("campaign");
        if (m_n.type == QStringLiteral("Security"))               return QStringLiteral("shield");
        if (m_n.type == QStringLiteral("ReadingReminder"))        return QStringLiteral("schedule");
        return QStringLiteral("notifications");
    }
    QString accentColor() const {
        if (m_n.category == QStringLiteral("recommendation")) return QStringLiteral("#1A73E8");
        if (m_n.category == QStringLiteral("discount"))       return QStringLiteral("#F29900");
        if (m_n.category == QStringLiteral("purchase"))       return QStringLiteral("#1E8E3E");
        if (m_n.category == QStringLiteral("review"))         return QStringLiteral("#1A73E8");
        if (m_n.category == QStringLiteral("publisher"))      return QStringLiteral("#7C4DFF");
        if (m_n.category == QStringLiteral("system"))         return QStringLiteral("#5F6368");
        if (m_n.category == QStringLiteral("security"))       return QStringLiteral("#D93025");
        if (m_n.category == QStringLiteral("reminder"))       return QStringLiteral("#F29900");
        return QStringLiteral("#1A73E8");
    }

    void setRead(bool r) { if (m_n.read != r) { m_n.read = r; emit readChanged(); } }

signals:
    void readChanged();

private:
    MockNotification m_n;
};

} // namespace bookclub::client
