// client/include/services/PublisherService.h
//
// Publisher-role service. Fully backed by the server via NetworkService.
//
// All analytics methods (totalRevenue, totalUnitsSold, topBooks,
// recentOrders, revenueByBook, etc.) call GetPublisherAnalytics once and
// cache the response so the dashboard's many KPI widgets don't each fire
// a separate request.
//
// v4 polish (this revision):
//   • Local promotions cache — addPromotion / updatePromotion / removePromotion
//     now operate on a local cache so the Promotions page list actually
//     populates. (Server has no list-discounts endpoint.)
//   • Local profile cache — updatePublisherProfile now persists locally
//     AND fires UpdateProfile on the server so the display name updates.
//   • Series generators (salesSeries / revenueSeries / monthlyRevenue)
//     now produce proper time-series shapes with {label, value} points
//     spread across the requested window, derived deterministically from
//     the server's per-book stats.
//   • recentOrders / topBuyers / activityFeed / genreBreakdown /
//     geographicBreakdown all return shapes that match their QML delegates.
//   • ratingDistribution(bookId) now actually filters by bookId when one
//     is provided.
//   • setBookStatus now recognizes "published" / "active" → ActivateBook
//     and "removed" / "inactive" / "draft" → DeactivateBook.
//   • publisherNotifications() now maps the raw server payload into the
//     shape the QML delegate expects ({id, type, icon, title, body, time,
//     read, tone}).
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>

namespace bookclub::client {

class PublisherService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PublisherService(QObject* parent = nullptr);
    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op (kept for QML compat)

    // ----- Catalog management -----
    Q_INVOKABLE QList<QObject*> publisherBooks() const;
    Q_INVOKABLE QString addBook(const QString& title, const QString& author,
                             const QString& genre, const QString& description,
                             double price, double discountPercent,
                             const QString& coverColor = QString(),
                             const QString& coverAccent = QString(),
                             const QString& coverImage = QString(),
                             const QString& pdfFilePath = QString());
    Q_INVOKABLE bool updateBook(const QString& bookId, const QString& title,
                                const QString& author, const QString& genre,
                                const QString& description, double price,
                                double discountPercent,
                                const QString& coverColor = QString(),
                                const QString& coverAccent = QString(),
                                const QString& coverImage = QString(),
                                const QString& pdfFilePath = QString());
    Q_INVOKABLE bool removeBook(const QString& bookId);
    Q_INVOKABLE bool setBookStatus(const QString& bookId, const QString& status);

    // ----- Analytics (all backed by GetPublisherAnalytics) -----
    Q_INVOKABLE QString totalRevenue() const;
    Q_INVOKABLE int totalUnitsSold() const;
    Q_INVOKABLE QVariantList booksStats() const;
    Q_INVOKABLE QVariantList topBooks(int count = 5) const;
    Q_INVOKABLE QVariantList leastSellingBooks(int count = 5) const;
    Q_INVOKABLE QVariantList revenueByBook() const;
    Q_INVOKABLE int totalBooks() const;
    Q_INVOKABLE int activeTitleCount() const;
    Q_INVOKABLE QString averageRating() const;
    Q_INVOKABLE QString revenueTrend() const;
    Q_INVOKABLE QString unitsSoldTrend() const;
    Q_INVOKABLE int repeatBuyerRate() const;
    Q_INVOKABLE QVariantList ratingDistribution(const QString& bookId) const;

    // Series derived from booksStats (computed client-side from the
    // server's per-book stats so we don't need a separate endpoint).
    Q_INVOKABLE QVariantList salesSeries(int days = 14) const;
    Q_INVOKABLE QVariantList revenueSeries(int days = 14) const;
    Q_INVOKABLE QVariantList monthlyRevenue(int months = 12) const;
    Q_INVOKABLE QVariantList recentOrders(int count = 10) const;
    Q_INVOKABLE QVariantList topBuyers(int count = 5) const;
    Q_INVOKABLE QVariantList activityFeed(int count = 8) const;
    Q_INVOKABLE QVariantList genreBreakdown() const;
    Q_INVOKABLE QVariantList geographicBreakdown() const;

    // ----- Book detail (uses GetBookDetails) -----
    Q_INVOKABLE QVariantMap bookDetail(const QString& bookId) const;
    Q_INVOKABLE QVariantList topViewedBooks(int count = 5) const;
    Q_INVOKABLE QVariantList topViewedBooksVariant(int count = 5) const;
    Q_INVOKABLE QVariantList reviewsList() const;

    // ----- Promotions (time-boxed discounts) -----
    Q_INVOKABLE QVariantList promotions() const;
    Q_INVOKABLE QString addPromotion(const QString& code, const QString& description,
                                      int discountPercent, int cap,
                                      const QString& startDate, const QString& endDate);
    Q_INVOKABLE bool updatePromotion(const QString& code, const QString& description,
                                      int discountPercent,
                                      const QString& startDate, const QString& endDate);
    Q_INVOKABLE bool removePromotion(const QString& code);
    Q_INVOKABLE bool deletePromotion(const QString& code) { return removePromotion(code); }

    // ----- Notifications (publisher-scoped subset of GetNotifications) -----
    Q_INVOKABLE QVariantList notifications() const;
    Q_INVOKABLE QVariantList publisherNotifications() const;
    Q_INVOKABLE void markAllNotificationsRead();
    Q_INVOKABLE void clearReadNotifications();
    Q_INVOKABLE void markNotificationRead(const QString& id, bool read);

    // ----- Profile -----
    Q_INVOKABLE QString publisherName() const;
    Q_INVOKABLE QVariantMap publisherProfile() const;
    Q_INVOKABLE bool updatePublisherProfile(const QString& publisherName, const QString& biography,
                                             const QString& website, const QString& email,
                                             const QString& taxId);

    // ----- Refresh -----
    Q_INVOKABLE void refresh();

signals:
    void dataChanged();
    // Granular signals — the ViewModel listens to these so it can emit the
    // matching per-property signals and only refresh what changed.
    void booksChanged();
    void promotionsChanged();
    void notificationsChanged();
    void profileChanged();

private:
    // Fetches GetPublisherAnalytics once and caches it. The cache is
    // cleared by refresh() and by any mutation (addBook / removeBook /
    // addPromotion / etc.). v22 (Issue 4): the cache is also invalidated
    // whenever a real-time EvtNotification / EvtBookAdded / EvtDiscountApplied
    // / EvtReviewUpdated event arrives — see the constructor in the .cpp.
    QJsonObject fetchAnalytics() const;
    QVariantList buildActivityFeed(const QJsonArray& bookStats, int count) const;

    // Helper: map a server notification type int → {icon, tone, title}.
    static void _enrichNotification(QVariantMap& n);

    // v22 (Issue 4): real-time cache invalidation helper. Called from the
    // event subscriptions in the ctor. Clears m_analyticsCache +
    // m_notificationsCache, then emits dataChanged() + the granular signals
    // so the PublisherViewModel re-emits and QML bindings refresh.
    void _invalidateCaches();

    mutable QJsonObject m_analyticsCache;
    mutable QVariantList m_booksCache;          // from GetPublisherBooks
    QVariantList m_localBooksCache;             // v8: books added offline, merged on next fetch
    mutable QVariantList m_notificationsCache;  // from GetNotifications
    mutable bool m_notificationsCacheValid;     // v6: prevents re-fetch after clearRead
    QVariantList m_promotionsCache;             // local-only (server has no list endpoint)
    QVariantMap m_profileCache;                 // local overlay on top of AuthService
};

} // namespace bookclub::client
