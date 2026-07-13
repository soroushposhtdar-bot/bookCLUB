// =============================================================================
//  PublisherService.h
// =============================================================================
//  Service layer for the Publisher role. Provides publisher-scoped catalog
//  management, sales analytics, promotion management, and notifications.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>

#include "services/MockDataStore.h"

namespace bookclub::client {

class PublisherService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PublisherService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    // Catalog management
    Q_INVOKABLE QList<QObject*> publisherBooks() const;
    Q_INVOKABLE QString addBook(const QString& title, const QString& author,
                             const QString& genre, const QString& description,
                             double price, double discountPercent,
                             const QString& coverColor, const QString& coverAccent,
                             const QString& coverImage = QString(),
                             const QString& pdfFilePath = QString());
    Q_INVOKABLE bool updateBook(const QString& bookId, const QString& title,
                                const QString& author, const QString& genre,
                                const QString& description, double price,
                                double discountPercent,
                                const QString& coverColor, const QString& coverAccent,
                                const QString& coverImage = QString(),
                                const QString& pdfFilePath = QString());
    Q_INVOKABLE bool removeBook(const QString& bookId);
    Q_INVOKABLE bool setBookStatus(const QString& bookId, const QString& status);

    // Sales analytics
    Q_INVOKABLE QString totalRevenue() const;
    Q_INVOKABLE int totalUnitsSold() const;
    Q_INVOKABLE int activeTitleCount() const;
    Q_INVOKABLE QString averageRating() const;
    Q_INVOKABLE QList<QObject*> topSellingBooks(int count) const;
    Q_INVOKABLE QList<QObject*> topViewedBooks(int count) const;
    Q_INVOKABLE QVariantList topBooks() const;
    Q_INVOKABLE QVariantList topViewedBooksVariant(int count) const;
    Q_INVOKABLE QVariantList leastSellingBooks(int count) const;   // spec §3-3: top 5 least-selling
    Q_INVOKABLE QVariantList revenueSeries(int days = 14) const;
    Q_INVOKABLE QVariantList genreBreakdown() const;
    Q_INVOKABLE QVariantList geographicBreakdown() const;          // replaces hardcoded QML array
    Q_INVOKABLE QVariantList activityFeed(int count = 8) const;    // replaces hardcoded QML array
    Q_INVOKABLE int repeatBuyerRate() const;                       // replaces hardcoded "62%"
    Q_INVOKABLE QString unitsSoldTrend() const;                    // replaces hardcoded "+8.1%"
    Q_INVOKABLE QVariantList ratingDistribution(const QString& bookId) const;  // spec §3-3: per-book 1-5 star histogram

    // Extended analytics (Phase 6)
    //   Monthly revenue for the last 12 months — used by the Sales page bar chart.
    //   Each entry: { label ("Jan"), value (revenue in USD), month (1-12), year (e.g. 2026) }.
    Q_INVOKABLE QVariantList monthlyRevenue(int months = 12) const;
    //   Recent orders across the publisher's catalog — synthesizes a plausible
    //   order stream from the book sales data so the dashboard has a live feed.
    //   Each entry: { orderId, bookTitle, customer, quantity, total, time, status }.
    Q_INVOKABLE QVariantList recentOrders(int count = 10) const;
    //   Top buyers — customers who purchased the most from this publisher.
    //   Each entry: { username, displayName, books, totalSpent, lastOrder }.
    Q_INVOKABLE QVariantList topBuyers(int count = 5) const;
    //   Detailed book view — used by the catalog row click → detail drawer.
    //   Returns all book fields plus a `reviews` QVariantList from the store.
    Q_INVOKABLE QVariantMap bookDetail(const QString& bookId) const;
    //   Sales trend — returns the % change in revenue between the last 7 days
    //   and the previous 7 days, used by the dashboard "vs last week" badge.
    Q_INVOKABLE QString revenueTrend() const;
    //   Total books in the catalog (including removed).
    Q_INVOKABLE int totalBooks() const;

    // Publisher profile (spec §3-1)
    //   The publisher's account info + catalog stats in one map. Used by the
    //   new PublisherProfilePage. Editable fields are persisted to m_profile
    //   (in-memory; a real backend would PUT to /publishers/me).
    Q_INVOKABLE QVariantMap publisherProfile() const;
    Q_INVOKABLE bool updatePublisherProfile(const QString& publisherName,
                                             const QString& biography,
                                             const QString& website,
                                             const QString& email,
                                             const QString& taxId);

    // Promotions
    Q_INVOKABLE QVariantList promotions() const;
    Q_INVOKABLE bool addPromotion(const QString& code, const QString& description,
                                  int discountPercent, int cap,
                                  const QString& startDate, const QString& endDate);
    Q_INVOKABLE bool removePromotion(const QString& code);

    // Publisher notifications
    Q_INVOKABLE QVariantList publisherNotifications() const;
    Q_INVOKABLE void markAllNotificationsRead();
    Q_INVOKABLE void clearReadNotifications();
    Q_INVOKABLE void markNotificationRead(const QString& id, bool read);   // per-item mark-as-read

signals:
    void booksChanged();
    void promotionsChanged();
    void notificationsChanged();
    void profileChanged();

private:
    MockDataStore* m_store = nullptr;

    struct Promotion {
        QString code, description, scope, status;
        QString startDate;     // ISO yyyy-MM-dd (may be empty for immediate start)
        QString endDate;       // ISO yyyy-MM-dd (may be empty for no expiry)
        int discountPercent, uses, cap;
    };
    QList<Promotion> m_promotions;

    QVariantList m_notifications;

    // Publisher profile cache (spec §3-1). Seeded once on construction,
    // updated in-place when updatePublisherProfile() is called.
    QVariantMap m_profile;

    void _seedPromotions();
    void _seedNotifications();
    void _seedProfile();
};

} // namespace bookclub::client
