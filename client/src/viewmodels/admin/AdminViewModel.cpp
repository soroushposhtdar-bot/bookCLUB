// =============================================================================
//  AdminViewModel.cpp
// =============================================================================
#include "viewmodels/admin/AdminViewModel.h"
#include "services/AdminService.h"

namespace bookclub::client {

AdminViewModel::AdminViewModel(QObject* parent) : UserViewModelBase(parent) {}

void AdminViewModel::setAdminService(AdminService* s) {
    if (m_service == s) return;
    if (m_service) disconnect(m_service, nullptr, this, nullptr);
    m_service = s;
    if (m_service) {
        connect(m_service, &AdminService::usersChanged, this, &AdminViewModel::usersChanged);
        connect(m_service, &AdminService::publishersChanged, this, &AdminViewModel::publishersChanged);
        connect(m_service, &AdminService::moderationChanged, this, &AdminViewModel::moderationChanged);
        connect(m_service, &AdminService::reportsChanged, this, &AdminViewModel::reportsChanged);
        connect(m_service, &AdminService::auditLogChanged, this, &AdminViewModel::auditLogChanged);
        connect(m_service, &AdminService::booksChanged, this, &AdminViewModel::booksChanged);
        connect(m_service, &AdminService::reviewsChanged, this, &AdminViewModel::reviewsChanged);
    }
    emit adminServiceChanged();
    emit usersChanged();
    emit publishersChanged();
    emit moderationChanged();
    emit reportsChanged();
    emit auditLogChanged();
    emit booksChanged();
    emit reviewsChanged();
}

QVariantList AdminViewModel::users() const { return m_service ? m_service->users() : QVariantList{}; }
int AdminViewModel::totalUsers() const { return m_service ? m_service->totalUsers() : 0; }
QVariantMap AdminViewModel::userDetails(const QString& u) const { return m_service ? m_service->userDetails(u) : QVariantMap{}; }
QVariantList AdminViewModel::userLoginHistory(const QString& u) const { return m_service ? m_service->userLoginHistory(u) : QVariantList{}; }
QVariantList AdminViewModel::userMemberships(const QString& u) const { return m_service ? m_service->userMemberships(u) : QVariantList{}; }
QVariantList AdminViewModel::pendingPublishers() const { return m_service ? m_service->pendingPublishers() : QVariantList{}; }
QVariantList AdminViewModel::activePublishers() const { return m_service ? m_service->activePublishers() : QVariantList{}; }
int AdminViewModel::activePublishersCount() const { return m_service ? m_service->activePublishersCount() : 0; }
int AdminViewModel::pendingReports() const { return m_service ? m_service->pendingReports() : 0; }
QString AdminViewModel::systemUptime() const { return m_service ? m_service->systemUptime() : QStringLiteral("99.97%"); }
QVariantList AdminViewModel::auditLog() const { return m_service ? m_service->auditLog() : QVariantList{}; }
QVariantList AdminViewModel::userGrowthSeries() const { return m_service ? m_service->userGrowthSeries() : QVariantList{}; }
QVariantList AdminViewModel::topGenres() const { return m_service ? m_service->topGenres() : QVariantList{}; }
QVariantList AdminViewModel::geographicDistribution() const { return m_service ? m_service->geographicDistribution() : QVariantList{}; }
QVariantList AdminViewModel::flaggedReviews() const { return m_service ? m_service->flaggedReviews() : QVariantList{}; }
QVariantList AdminViewModel::reportedContent() const { return m_service ? m_service->reportedContent() : QVariantList{}; }
QVariantList AdminViewModel::reports() const { return m_service ? m_service->reports() : QVariantList{}; }
QVariantList AdminViewModel::allBooks() const { return m_service ? m_service->allBooks() : QVariantList{}; }
int AdminViewModel::totalBooks() const { return m_service ? m_service->totalBooks() : 0; }
QVariantList AdminViewModel::allReviews() const { return m_service ? m_service->allReviews() : QVariantList{}; }
int AdminViewModel::totalReviews() const { return m_service ? m_service->totalReviews() : 0; }
int AdminViewModel::flaggedReviewsCount() const { return m_service ? m_service->flaggedReviewsCount() : 0; }
QVariantMap AdminViewModel::systemHealth() const { return m_service ? m_service->systemHealth() : QVariantMap{}; }

// BUG FIX (admin users real-time): the previous refresh() only called
// beginAsync(400) + onAsyncReady() (which emits signals). It NEVER called
// m_service->refresh() — so the AdminService's caches (m_booksCache,
// m_reviewsCache) were never invalidated by the 30s timer. The users()
// method fetches fresh on every call (no cache), but the books/reviews
// caches stayed stale forever after the first load.
//
// Now refresh() calls m_service->refresh() FIRST (which invalidates all
// service caches + emits dataChanged/usersChanged/etc.), THEN calls
// beginAsync(400) so the VM's isBusy flag toggles for the UI. The
// service's emitted signals are forwarded by the connect() calls in
// setAdminService(), so the QML bindings re-evaluate with fresh data.
void AdminViewModel::refresh() {
    if (m_service) m_service->refresh();
    beginAsync(400);
}
void AdminViewModel::onAsyncReady() {
    emit usersChanged(); emit publishersChanged(); emit moderationChanged();
    emit reportsChanged(); emit auditLogChanged(); emit booksChanged(); emit reviewsChanged();
    finishAsync();
}

void AdminViewModel::blockUser(const QString& u) { if (m_service) m_service->blockUser(u); }
void AdminViewModel::unblockUser(const QString& u) { if (m_service) m_service->unblockUser(u); }
void AdminViewModel::deleteUser(const QString& u) { if (m_service) m_service->deleteUser(u); }

void AdminViewModel::activateUser(const QString& u) { if (m_service) m_service->activateUser(u); }
void AdminViewModel::deactivateUser(const QString& u) { if (m_service) m_service->deactivateUser(u); }
void AdminViewModel::setUserRole(const QString& u, const QString& r) { if (m_service) m_service->setUserRole(u, r); }
bool AdminViewModel::suspendMembership(const QString& u, int i) { return m_service ? m_service->suspendMembership(u, i) : false; }
bool AdminViewModel::reactivateMembership(const QString& u, int i) { return m_service ? m_service->reactivateMembership(u, i) : false; }
bool AdminViewModel::cancelMembership(const QString& u, int i) { return m_service ? m_service->cancelMembership(u, i) : false; }
void AdminViewModel::approvePublisher(const QString& u) { if (m_service) m_service->approvePublisher(u); }
void AdminViewModel::rejectPublisher(const QString& u) { if (m_service) m_service->rejectPublisher(u); }
void AdminViewModel::dismissFlaggedReview(const QString& id) { if (m_service) m_service->dismissFlaggedReview(id); }
void AdminViewModel::removeFlaggedReview(const QString& id) { if (m_service) m_service->removeFlaggedReview(id); }
void AdminViewModel::dismissReport(const QString& id) { if (m_service) m_service->dismissReport(id); }
void AdminViewModel::takeActionOnReport(const QString& id, const QString& action) { if (m_service) m_service->takeActionOnReport(id, action); }
void AdminViewModel::updateReportStatus(const QString& id, const QString& status) { if (m_service) m_service->updateReportStatus(id, status); }
void AdminViewModel::assignReport(const QString& id, const QString& assignee) { if (m_service) m_service->assignReport(id, assignee); }
bool AdminViewModel::deleteBook(const QString& bookId, const QString& reason) { return m_service ? m_service->deleteBook(bookId, reason) : false; }
bool AdminViewModel::setBookStatus(const QString& bookId, const QString& status) { return m_service ? m_service->setBookStatus(bookId, status) : false; }
// BUG FIX (admin books real-time): approveBook/rejectBook delegate to
// setBookStatus with the appropriate status string. The server's
// ModerateBook command toggles isActive, so approveBook = "active" and
// rejectBook = "removed". This makes the drawer's Approve/Reject buttons
// work and emit booksChanged() so the QML table updates in real-time.
bool AdminViewModel::approveBook(const QString& bookId) { return m_service ? m_service->setBookStatus(bookId, "active") : false; }
bool AdminViewModel::rejectBook(const QString& bookId) { return m_service ? m_service->setBookStatus(bookId, "removed") : false; }
bool AdminViewModel::updateBookInfo(const QString& bookId, const QString& title, const QString& author, const QString& genre, double price, const QString& description) {
    return m_service ? m_service->updateBookInfo(bookId, title, author, genre, price, description) : false;
}
QVariantMap AdminViewModel::bookDetails(const QString& bookId) const { return m_service ? m_service->bookDetails(bookId) : QVariantMap{}; }
QVariantList AdminViewModel::reviewsForBook(const QString& bookId) const { return m_service ? m_service->reviewsForBook(bookId) : QVariantList{}; }
bool AdminViewModel::deleteReview(const QString& id) { return m_service ? m_service->deleteReview(id) : false; }
bool AdminViewModel::approveReview(const QString& id) { return m_service ? m_service->approveReview(id) : false; }

} // namespace bookclub::client
