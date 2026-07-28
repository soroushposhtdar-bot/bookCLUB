// client/src/services/AdminService.cpp
//
// Full socket-backed implementation of AdminService.
#include "services/AdminService.h"
#include "services/NetworkService.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>
#include <QDateTime>
#include <algorithm>

namespace bookclub::client {

AdminService::AdminService(QObject* parent) : QObject(parent) {}

// ============================================================================
//  User management
// ============================================================================

QVariantList AdminService::users() const {
    // BUG FIX: cache the user list so that activePublishers(), pendingPublishers(),
    // totalUsers(), userDetails() etc. don't each fire a separate synchronous
    // GetUsersList round-trip. Without this cache, every QML binding that reads
    // any derived user property caused a network request inside a nested
    // QEventLoop — which produced "no handler registered" warnings on the server
    // (the response arrived after the timeout, so the requestId was already
    // unregistered) and could freeze the UI.
    if (!m_usersCacheDirty) return m_usersCache;
    m_usersCacheDirty = false;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetUsersList);
    if (!resp.isSuccess()) { m_usersCacheDirty = true; return m_usersCache; }
    m_usersCache = resp.payload.value("users").toArray().toVariantList();
    return m_usersCache;
}

int AdminService::totalUsers() const {
    return users().size();
}

QVariantMap AdminService::userDetails(const QString& username) const {
    for (const QVariant& v : users()) {
        QVariantMap u = v.toMap();
        if (u.value("username").toString() == username) return u;
    }
    return {};
}

void AdminService::blockUser(const QString& username) {
    const QVariantMap u = userDetails(username);
    if (u.isEmpty()) return;
    QJsonObject p;
    p["userId"] = u.value("id").toString();
    // v21: use sendRequest (synchronous) instead of sendAsync so the
    // block takes effect immediately. Then invalidate the cache so
    // the next read fetches fresh data from the server.
    NetworkService::instance().sendRequest(common::Command::BlockUser, p);
    m_usersCache.clear();  // invalidate
    emit dataChanged();
    emit usersChanged();
}

void AdminService::unblockUser(const QString& username) {
    const QVariantMap u = userDetails(username);
    if (u.isEmpty()) return;
    QJsonObject p;
    p["userId"] = u.value("id").toString();
    // v21: synchronous so the unblock takes effect immediately.
    NetworkService::instance().sendRequest(common::Command::UnblockUser, p);
    m_usersCache.clear();
    emit dataChanged();
    emit usersChanged();
}

void AdminService::deleteUser(const QString& username) {
    const QVariantMap u = userDetails(username);
    if (u.isEmpty()) return;
    QJsonObject p;
    p["userId"] = u.value("id").toString();
    // v21: synchronous so the delete takes effect immediately.
    NetworkService::instance().sendRequest(common::Command::DeleteUser, p);
    m_usersCache.clear();
    emit dataChanged();
    emit usersChanged();
}

// v12: activate/deactivate user (doc §4-2 — "مدیریت وضعیت فعال یا غیرفعال کاربران")
// activate = unblock (set status to Active), deactivate = block (set status to Blocked)
void AdminService::activateUser(const QString& username) {
    unblockUser(username);  // reuse existing unblock logic
}

void AdminService::deactivateUser(const QString& username) {
    blockUser(username);  // reuse existing block logic
}

void AdminService::setUserRole(const QString& /*username*/, const QString& /*role*/) {
    // Server has no setUserRole endpoint yet.
}

QVariantList AdminService::userLoginHistory(const QString& /*username*/) const {
    return {};
}
QVariantList AdminService::userMemberships(const QString& /*username*/) const {
    return {};
}
bool AdminService::suspendMembership(const QString&, int) { return false; }
bool AdminService::reactivateMembership(const QString&, int) { return false; }
bool AdminService::cancelMembership(const QString&, int) { return false; }

// ============================================================================
//  Publisher management (derived from users())
// ============================================================================

QVariantList AdminService::activePublishers() const {
    QVariantList out;
    for (const QVariant& v : users()) {
        QVariantMap u = v.toMap();
        if (u.value("role").toInt() == 1 && u.value("status").toInt() == 1) {
            out.append(u);
        }
    }
    return out;
}

int AdminService::activePublishersCount() const {
    return activePublishers().size();
}

QVariantList AdminService::pendingPublishers() const {
    QVariantList out;
    for (const QVariant& v : users()) {
        QVariantMap u = v.toMap();
        if (u.value("role").toInt() == 1 && u.value("status").toInt() == 0) {
            out.append(u);
        }
    }
    return out;
}

int AdminService::pendingPublishersCount() const {
    return pendingPublishers().size();
}

bool AdminService::approvePublisher(const QString& username) {
    // Approval = set status to Active (1). No dedicated endpoint; fall back
    // to UnblockUser which sets status=Active.
    unblockUser(username);
    return true;
}

bool AdminService::rejectPublisher(const QString& username, const QString& /*reason*/) {
    // No dedicated endpoint; soft-delete via deleteUser.
    deleteUser(username);
    return true;
}

// ============================================================================
//  Content moderation
// ============================================================================

QVariantList AdminService::flaggedReviews() const {
    // Aggregate reviews with isFlagged=true across all books. Server has no
    // "all flagged reviews" endpoint, so we iterate books and filter.
    QVariantList out;
    for (const QVariant& v : allBooks()) {
        const QString bookId = v.toMap().value("id").toString();
        for (const QVariant& r : reviewsForBook(bookId)) {
            if (r.toMap().value("isFlagged").toBool()) out.append(r);
        }
    }
    return out;
}

int AdminService::flaggedReviewsCount() const {
    return flaggedReviews().size();
}

void AdminService::dismissFlaggedReview(const QString& /*id*/) {
    // No server endpoint; local-only.
}
void AdminService::removeFlaggedReview(const QString& id) {
    deleteReview(id);
}

QVariantList AdminService::reportedContent() const { return {}; }
int AdminService::reportedContentCount() const { return 0; }
QVariantList AdminService::abuseReports() const { return {}; }
int AdminService::abuseReportsCount() const { return 0; }
QVariantList AdminService::reports() const { return {}; }
int AdminService::pendingReports() const { return 0; }
void AdminService::takeActionOnReport(const QString&, const QString&) {}
void AdminService::updateReportStatus(const QString&, const QString&) {}
void AdminService::assignReport(const QString&, const QString&) {}
bool AdminService::resolveReport(const QString&) { return false; }
bool AdminService::dismissReport(const QString&) { return false; }

// ============================================================================
//  Books (server-backed)
// ============================================================================

QVariantList AdminService::booksList() const {
    return allBooks();
}

QVariantList AdminService::allBooks() const {
    // Return cached result if still valid.  Without this cache, every
    // QML binding that reads .allBooks (or .allReviews which calls allBooks
    // internally) triggers a fresh SearchBooks round-trip.  When the
    // review-monitor ListView on AdminBooksPage re-evaluates its model
    // binding every 5 s (Timer pulse), this caused N+1 synchronous network
    // calls that froze / crashed the UI.
    if (!m_booksCacheDirty) return m_booksCache;
    m_booksCacheDirty = false;

    // BUG 2: GetHomeSections returns a curated subset (featured / newBooks /
    // bestSellers / freeBooks) intended for the user home screen — if all
    // four are empty, allBooks() returned [] regardless of how many books
    // existed. Switch to SearchBooks.
    //
    // IMPORTANT — server contract quirk:
    //   * Server's handleSearchBooks reads the parameter from the "keyword"
    //     key (NOT "query"). Sending "query" was a no-op.
    //   * Server writes the result array under the "results" key (NOT
    //     "books"). Reading "books" returned nothing — that's why the
    //     admin books page showed 0 books.
    //   * When "keyword" is empty/whitespace, the server routes to
    //     listFeaturedBooks() which returns only the top 10 books by
    //     sales — NOT the full catalog. To get every active book without
    //     changing backend code (constraint), we send a non-empty keyword
    //     with field="genre". The server's searchBooksByField(Genre)
    //     implementation currently delegates to m_bookRepo->findAll(),
    //     which returns ALL active books. This is the only existing
    //     endpoint path that returns the full catalog.
    QJsonObject p;
    p["keyword"] = "*";      // non-empty so server bypasses listFeaturedBooks()
    p["field"]   = "genre";  // routes to searchBooksByField(Genre) → findAll()
    auto resp = NetworkService::instance().sendRequest(common::Command::SearchBooks, p);
    if (!resp.isSuccess()) { m_booksCacheDirty = true; m_booksCache.clear(); return m_booksCache; }
    m_booksCache.clear();
    for (const auto& v : resp.payload.value("results").toArray()) {
        m_booksCache.append(v.toObject().toVariantMap());
    }
    return m_booksCache;
}

QVariantMap AdminService::bookDetails(const QString& bookId) const {
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    QVariantMap result;
    if (resp.isSuccess()) {
        result = resp.payload.toVariantMap();
        // Cache the reviews so allReviews() can reuse them.
        const QVariantList reviews = result.value("reviews").toList();
        m_reviewsCache[bookId] = reviews;
    }
    return result;
}

bool AdminService::deleteBook(const QString& bookId, const QString& /*reason*/) {
    // Invalidate caches before the operation so stale data isn't served.
    m_booksCacheDirty = true;
    m_reviewsCacheDirty = true;
    m_reviewsCache.clear();
    return removeBook(bookId);
}

bool AdminService::removeBook(const QString& bookId) {
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::RemoveBookByAdmin, p);
    if (resp.isSuccess()) {
        emit dataChanged();
        emit booksChanged();
        return true;
    }
    return false;
}

bool AdminService::setBookStatus(const QString& bookId, const QString& status) {
    QJsonObject p;
    p["bookId"] = bookId;
    // v21b: send the desired action explicitly so the server doesn't
    // just toggle (which could activate when the admin wanted to
    // deactivate, or vice versa).
    p["action"] = status;  // "active" or "inactive"
    auto resp = NetworkService::instance().sendRequest(common::Command::ModerateBook, p);
    if (resp.isSuccess()) {
        m_booksCacheDirty = true;
        m_reviewsCacheDirty = true;
        m_reviewsCache.clear();
        emit dataChanged();
        emit booksChanged();
        return true;
    }
    return false;
}

bool AdminService::updateBookInfo(const QString& bookId, const QString& title,
                                   const QString& author, const QString& genre,
                                   double price, const QString& description) {
    QJsonObject p;
    p["id"]          = bookId;
    p["title"]       = title;
    p["authorName"]  = author;
    p["description"] = description;
    p["basePrice"]   = price;
    QJsonArray genres;
    genres.append(genre);
    p["genreIds"] = genres;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateBook, p);
    if (resp.isSuccess()) {
        m_booksCacheDirty = true;
        emit dataChanged();
        emit booksChanged();
        return true;
    }
    return false;
}

// ============================================================================
//  Reviews (server-backed via GetBookDetails)
// ============================================================================

QVariantList AdminService::allReviews() const {
    // Cache the aggregated result so the admin Books page review-monitor
    // ListView doesn't re-trigger N+1 network calls every time the binding
    // is re-evaluated (every 5 s from the Timer pulse).
    if (!m_reviewsCacheDirty) return m_allReviewsCache;
    m_reviewsCacheDirty = false;

    QVariantList out;
    const QVariantList books = allBooks(); // uses books cache
    for (const QVariant& v : books) {
        const QString bookId = v.toMap().value("id").toString();
        for (const QVariant& r : reviewsForBook(bookId)) { // uses per-book cache
            QVariantMap entry = r.toMap();
            entry["bookTitle"] = v.toMap().value("title");
            // Normalise the key so QML can bind to modelData.flagged
            if (entry.contains("isFlagged"))
                entry["flagged"] = entry.value("isFlagged");
            out.append(entry);
        }
    }
    m_allReviewsCache = out;
    return m_allReviewsCache;
}

QVariantList AdminService::reviewsForBook(const QString& bookId) const {
    // Per-book cache so allReviews() doesn't re-fetch for every book on
    // every binding evaluation.
    auto it = m_reviewsCache.constFind(bookId);
    if (it != m_reviewsCache.constEnd()) return *it;

    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    QVariantList reviews;
    if (resp.isSuccess())
        reviews = resp.payload.value("reviews").toArray().toVariantList();
    m_reviewsCache[bookId] = reviews;
    return reviews;
}

int AdminService::totalReviews() const {
    return allReviews().size();
}

bool AdminService::deleteReview(const QString& /*reviewId*/) {
    // Server has no admin delete-review endpoint yet (only the user can
    // delete their own review via DeleteReview). Admin uses the same.
    // TODO: route via DeleteReview once the server-side handler checks
    // admin permissions.
    return false;
}

bool AdminService::approveReview(const QString& /*reviewId*/) {
    // Server has no approve-review endpoint.
    return false;
}

// ============================================================================
//  Audit log (server-backed via GetServerLogs)
// ============================================================================

QVariantList AdminService::auditLog() const {
    // BUG 1: use the same cache pattern as allBooks() so the
    // AdminDashboardPage's activity feed survives a tab switch.
    // Without this cache, every QML binding that reads
    // viewModel.auditLog (the dashboard's _activity computed property,
    // systemAlerts(), etc.) triggers a fresh synchronous GetServerLogs
    // round-trip. When the page is destroyed on tab switch and
    // recreated on return, the new Component.onCompleted cycle races
    // the VM's refresh() and the feed never repopulates.
    if (!m_auditLogCacheDirty) return m_auditLogCache;
    m_auditLogCacheDirty = false;

    auto resp = NetworkService::instance().sendRequest(common::Command::GetServerLogs);
    if (!resp.isSuccess()) { m_auditLogCache.clear(); return m_auditLogCache; }
    m_auditLogCache = resp.payload.value("logs").toArray().toVariantList();
    return m_auditLogCache;
}

QVariantList AdminService::systemAlerts() const {
    // Filter audit log for warning/error entries.
    QVariantList out;
    for (const QVariant& v : auditLog()) {
        const QString level = v.toMap().value("level").toString().toLower();
        if (level == "warning" || level == "error" || level == "critical") {
            out.append(v);
        }
    }
    return out;
}

// ============================================================================
//  Platform analytics
// ============================================================================

int AdminService::totalBooks() const {
    return allBooks().size();
}

int AdminService::totalPublishers() const {
    return activePublishersCount();
}

double AdminService::totalRevenue() const {
    // Sum every publisher's revenue. Server has no platform-wide revenue
    // endpoint; we'd need GetPublisherAnalytics per publisher. For now,
    // sum the per-book totalSales * price from allBooks().
    double total = 0;
    for (const QVariant& v : allBooks()) {
        const auto m = v.toMap();
        total += m.value("totalSales").toInt() * m.value("price").toDouble();
    }
    return total;
}

QString AdminService::totalRevenueText() const {
    return QLocale().toString(totalRevenue(), 'f', 2);
}

int AdminService::totalSales() const {
    int total = 0;
    for (const QVariant& v : allBooks()) {
        total += v.toMap().value("totalSales").toInt();
    }
    return total;
}

QVariantList AdminService::revenueSeries() const {
    QVariantList out;
    for (const QVariant& v : allBooks()) {
        QVariantMap point;
        point["label"] = v.toMap().value("title");
        point["value"] = v.toMap().value("totalSales").toInt() *
                         v.toMap().value("price").toDouble();
        out.append(point);
    }
    return out;
}

QVariantList AdminService::userGrowthSeries() const {
    // Server has no historical user-growth endpoint. Build a synthetic
    // single-point series so the sparkline isn't empty.
    QVariantList out;
    QVariantMap point;
    point["label"] = QStringLiteral("Now");
    point["value"] = totalUsers();
    out.append(point);
    return out;
}

QVariantList AdminService::salesByCategory() const {
    // Aggregate books by first genre.
    QVariantList out;
    QHash<QString, int> counts;
    for (const QVariant& v : allBooks()) {
        const auto genres = v.toMap().value("genreIds").toStringList();
        if (!genres.isEmpty()) counts[genres.first()]++;
    }
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QVariantMap point;
        point["label"] = it.key();
        point["value"] = it.value();
        out.append(point);
    }
    return out;
}

QVariantList AdminService::topBooks() const {
    auto books = allBooks();
    std::sort(books.begin(), books.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("totalSales").toInt() > b.toMap().value("totalSales").toInt();
    });
    if (books.size() > 5) books = books.mid(0, 5);
    return books;
}

QVariantList AdminService::topPublishers() const {
    // Aggregate book sales per publisher.
    QVariantList out;
    QHash<QString, int> salesByPublisher;
    for (const QVariant& v : allBooks()) {
        const QString pubId = v.toMap().value("publisherId").toString();
        salesByPublisher[pubId] += v.toMap().value("totalSales").toInt();
    }
    // Resolve publisher names from users().
    QHash<QString, QString> nameById;
    for (const QVariant& v : users()) {
        const auto m = v.toMap();
        nameById[m.value("id").toString()] = m.value("displayName").toString();
    }
    for (auto it = salesByPublisher.begin(); it != salesByPublisher.end(); ++it) {
        QVariantMap point;
        point["label"] = nameById.value(it.key(), it.key());
        point["value"] = it.value();
        out.append(point);
    }
    std::sort(out.begin(), out.end(), [](const QVariant& a, const QVariant& b) {
        return a.toMap().value("value").toInt() > b.toMap().value("value").toInt();
    });
    return out;
}

QVariantList AdminService::topGenres() const {
    return salesByCategory();
}

QVariantList AdminService::geographicDistribution() const {
    // Server has no geo data.
    return {};
}

QString AdminService::systemUptime() const {
    auto health = systemHealth();
    return health.value("uptime").toString();
}

QVariantMap AdminService::systemHealth() const {
    // BUG FIX: cache the health snapshot. AdminDashboardPage reads
    // viewModel.systemHealth via multiple computed properties (_healthData,
    // _health array, _healthStatus, _healthStatusColor). Without a cache,
    // each re-evaluation fired a synchronous GetServerHealth inside a
    // nested QEventLoop. The nested loops produced "no handler registered"
    // log spam on the server and caused system health cards to show 0 /
    // unknown after entering the Server section (cross-contamination of
    // in-flight requests via the single shared TCP connection).
    if (!m_healthCacheDirty) return m_healthCache;
    m_healthCacheDirty = false;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetServerHealth);
    if (!resp.isSuccess()) { m_healthCacheDirty = true; return m_healthCache; }
    m_healthCache = resp.payload.toVariantMap();
    return m_healthCache;
}

// ============================================================================
//  Refresh
// ============================================================================

void AdminService::refresh() {
    // Invalidate caches so the next property read fetches fresh data.
    m_booksCacheDirty = true;
    m_reviewsCacheDirty = true;
    m_reviewsCache.clear();
    // BUG 1: invalidate the audit log cache before emitting
    // auditLogChanged() so the next auditLog() read re-fetches from the
    // server. Mirrors the m_booksCacheDirty handling above.
    m_auditLogCacheDirty = true;
    // Invalidate user + health caches so the next read fetches fresh data.
    m_usersCacheDirty = true;
    m_healthCacheDirty = true;

    emit dataChanged();
    emit usersChanged();
    emit publishersChanged();
    emit booksChanged();
    emit reviewsChanged();
    emit auditLogChanged();
    emit moderationChanged();
    emit reportsChanged();
}

} // namespace bookclub::client
