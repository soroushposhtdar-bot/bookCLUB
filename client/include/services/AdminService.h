// client/include/services/AdminService.h
//
// Admin-role service. Fully backed by the server via NetworkService.
//
// User list, blocking, deletion, role changes are real. Books list uses
// GetBooksByIds batch endpoint with the user-list endpoint's books.
// Reviews moderation uses GetBookDetails' embedded reviews. Analytics
// (system health, total users, total publishers) come from GetServerHealth
// + GetUsersList filtering.
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>

namespace bookclub::client {

class AdminService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit AdminService(QObject* parent = nullptr);
    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op (kept for QML compat)

    // ----- User management (server-backed) -----
    Q_INVOKABLE QVariantList users() const;
    Q_INVOKABLE int totalUsers() const;
    Q_INVOKABLE QVariantMap userDetails(const QString& username) const;
    Q_INVOKABLE void blockUser(const QString& username);
    Q_INVOKABLE void unblockUser(const QString& username);
    Q_INVOKABLE void deleteUser(const QString& username);
    Q_INVOKABLE void activateUser(const QString& username);     // v12: doc §4-2
    Q_INVOKABLE void deactivateUser(const QString& username);   // v12: doc §4-2
    Q_INVOKABLE void setUserRole(const QString& username, const QString& role);

    // User-management sub-lookups (best-effort — server doesn't expose
    // login history or memberships yet).
    Q_INVOKABLE QVariantList userLoginHistory(const QString& username) const;
    Q_INVOKABLE QVariantList userMemberships(const QString& username) const;
    Q_INVOKABLE bool suspendMembership(const QString& username, int membershipIndex);
    Q_INVOKABLE bool reactivateMembership(const QString& username, int membershipIndex);
    Q_INVOKABLE bool cancelMembership(const QString& username, int membershipIndex);

    // ----- Publisher management (derived from users()) -----
    Q_INVOKABLE QVariantList activePublishers() const;
    Q_INVOKABLE int activePublishersCount() const;
    Q_INVOKABLE QVariantList pendingPublishers() const;
    Q_INVOKABLE int pendingPublishersCount() const;
    Q_INVOKABLE bool approvePublisher(const QString& username);
    Q_INVOKABLE bool rejectPublisher(const QString& username, const QString& reason = QString());

    // ----- Content moderation -----
    Q_INVOKABLE QVariantList flaggedReviews() const;
    Q_INVOKABLE int flaggedReviewsCount() const;
    Q_INVOKABLE void dismissFlaggedReview(const QString& id);
    Q_INVOKABLE void removeFlaggedReview(const QString& id);
    Q_INVOKABLE QVariantList reportedContent() const;
    Q_INVOKABLE int reportedContentCount() const;
    Q_INVOKABLE QVariantList abuseReports() const;
    Q_INVOKABLE int abuseReportsCount() const;
    Q_INVOKABLE QVariantList reports() const;
    Q_INVOKABLE int pendingReports() const;
    Q_INVOKABLE void takeActionOnReport(const QString& id, const QString& action);
    Q_INVOKABLE void updateReportStatus(const QString& id, const QString& status);
    Q_INVOKABLE void assignReport(const QString& id, const QString& assignee);
    Q_INVOKABLE bool resolveReport(const QString& id);
    Q_INVOKABLE bool dismissReport(const QString& id);

    // ----- Books (server-backed) -----
    Q_INVOKABLE QVariantList booksList() const;
    Q_INVOKABLE QVariantList allBooks() const;
    Q_INVOKABLE QVariantMap bookDetails(const QString& bookId) const;
    Q_INVOKABLE bool deleteBook(const QString& bookId, const QString& reason = QString());
    Q_INVOKABLE bool removeBook(const QString& bookId);
    Q_INVOKABLE bool setBookStatus(const QString& bookId, const QString& status);
    Q_INVOKABLE bool updateBookInfo(const QString& bookId, const QString& title,
                                     const QString& author, const QString& genre,
                                     double price, const QString& description);

    // ----- Reviews (server-backed via GetBookDetails) -----
    Q_INVOKABLE QVariantList allReviews() const;
    Q_INVOKABLE QVariantList reviewsForBook(const QString& bookId) const;
    Q_INVOKABLE int totalReviews() const;
    Q_INVOKABLE bool deleteReview(const QString& reviewId);
    Q_INVOKABLE bool approveReview(const QString& reviewId);

    // ----- Audit log (server-backed via GetServerLogs) -----
    Q_INVOKABLE QVariantList auditLog() const;
    Q_INVOKABLE QVariantList systemAlerts() const;

    // ----- Platform analytics -----
    Q_INVOKABLE int totalBooks() const;
    Q_INVOKABLE int totalPublishers() const;
    Q_INVOKABLE double totalRevenue() const;
    Q_INVOKABLE QString totalRevenueText() const;
    Q_INVOKABLE int totalSales() const;
    Q_INVOKABLE QVariantList revenueSeries() const;
    Q_INVOKABLE QVariantList userGrowthSeries() const;
    Q_INVOKABLE QVariantList salesByCategory() const;
    Q_INVOKABLE QVariantList topBooks() const;
    Q_INVOKABLE QVariantList topPublishers() const;
    Q_INVOKABLE QVariantList topGenres() const;
    Q_INVOKABLE QVariantList geographicDistribution() const;
    Q_INVOKABLE QString systemUptime() const;
    Q_INVOKABLE QVariantMap systemHealth() const;

    // ----- Refresh -----
    Q_INVOKABLE void refresh();

private:
    mutable QVariantList m_booksCache;
    mutable QHash<QString, QVariantList> m_reviewsCache;
    mutable QVariantList m_allReviewsCache;
    mutable bool m_booksCacheDirty = true;
    mutable bool m_reviewsCacheDirty = true;
    // BUG FIX (admin users real-time + crash): added a users cache so that
    // activePublishers() / pendingPublishers() / userDetails() /
    // totalUsers() don't each trigger a separate synchronous GetUsersList
    // network round-trip. Without this cache, the AdminPublishersPage's KPI
    // cards alone triggered 3+ synchronous requests on every page entry —
    // which froze the UI and sometimes crashed when the VM emitted signals
    // during the layout pass. The cache is invalidated by refresh() and by
    // blockUser/unblockUser/deleteUser after the server confirms the change.
    mutable QVariantList m_usersCache;
    mutable bool m_usersCacheDirty = true;

    // Health cache — prevents GetServerHealth being sent on every QML
    // binding re-evaluation (which happens on every signal emission).
    mutable QVariantMap m_healthCache;
    mutable bool m_healthCacheDirty = true;

    // BUG 1 (admin dashboard activity feed + server logs page): audit log
    // cache + dirty flag, mirroring the m_booksCache / m_booksCacheDirty
    // pair. Without this, auditLog() fires a fresh GetServerLogs request
    // on every read; when the AdminDashboardPage is destroyed on tab
    // switch and recreated on return, Component.onCompleted's refresh()
    // cycle races the VM and the feed never repopulates. The cache is
    // invalidated by refresh() before auditLogChanged() is emitted.
    mutable QVariantList m_auditLogCache;
    mutable bool m_auditLogCacheDirty = true;

signals:
    void dataChanged();
    void usersChanged();
    void publishersChanged();
    void moderationChanged();
    void reportsChanged();
    void auditLogChanged();
    void booksChanged();
    void reviewsChanged();
};

} // namespace bookclub::client
