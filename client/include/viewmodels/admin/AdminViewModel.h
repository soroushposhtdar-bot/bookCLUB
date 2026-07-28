// =============================================================================
//  AdminViewModel.h
// =============================================================================
#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include "viewmodels/user/UserViewModelBase.h"

// Include full service headers so MOC sees complete types for Q_PROPERTY pointers.
#include "services/AdminService.h"

namespace bookclub::client {

class AdminViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(AdminService* adminService READ adminService WRITE setAdminService NOTIFY adminServiceChanged)
    Q_PROPERTY(QVariantList users READ users NOTIFY usersChanged)
    Q_PROPERTY(int totalUsers READ totalUsers NOTIFY usersChanged)
    Q_PROPERTY(QVariantList pendingPublishers READ pendingPublishers NOTIFY publishersChanged)
    Q_PROPERTY(QVariantList activePublishers READ activePublishers NOTIFY publishersChanged)
    Q_PROPERTY(int activePublishersCount READ activePublishersCount NOTIFY publishersChanged)
    Q_PROPERTY(int pendingReports READ pendingReports NOTIFY reportsChanged)
    Q_PROPERTY(QString systemUptime READ systemUptime NOTIFY adminServiceChanged)
    Q_PROPERTY(QVariantList auditLog READ auditLog NOTIFY auditLogChanged)
    Q_PROPERTY(QVariantList userGrowthSeries READ userGrowthSeries NOTIFY usersChanged)
    Q_PROPERTY(QVariantList topGenres READ topGenres NOTIFY usersChanged)
    Q_PROPERTY(QVariantList geographicDistribution READ geographicDistribution NOTIFY usersChanged)
    Q_PROPERTY(QVariantList flaggedReviews READ flaggedReviews NOTIFY moderationChanged)
    Q_PROPERTY(QVariantList reportedContent READ reportedContent NOTIFY moderationChanged)
    Q_PROPERTY(QVariantList reports READ reports NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList allBooks READ allBooks NOTIFY booksChanged)
    Q_PROPERTY(int totalBooks READ totalBooks NOTIFY booksChanged)
    Q_PROPERTY(QVariantList allReviews READ allReviews NOTIFY reviewsChanged)
    Q_PROPERTY(int totalReviews READ totalReviews NOTIFY reviewsChanged)
    Q_PROPERTY(int flaggedReviewsCount READ flaggedReviewsCount NOTIFY reviewsChanged)
    Q_PROPERTY(QVariantMap systemHealth READ systemHealth NOTIFY usersChanged)
    Q_PROPERTY(bool loading READ isBusy NOTIFY isBusyChanged)

public:
    explicit AdminViewModel(QObject* parent = nullptr);
    AdminService* adminService() const { return m_service; }
    void setAdminService(AdminService* s);

    QVariantList users() const;
    int totalUsers() const;
    Q_INVOKABLE QVariantMap userDetails(const QString& username) const;
    Q_INVOKABLE QVariantList userLoginHistory(const QString& username) const;
    Q_INVOKABLE QVariantList userMemberships(const QString& username) const;
    QVariantList pendingPublishers() const;
    QVariantList activePublishers() const;
    int activePublishersCount() const;
    int pendingReports() const;
    QString systemUptime() const;
    QVariantList auditLog() const;
    QVariantMap systemHealth() const;
    QVariantList userGrowthSeries() const;
    QVariantList topGenres() const;
    QVariantList geographicDistribution() const;
    QVariantList flaggedReviews() const;
    QVariantList reportedContent() const;
    QVariantList reports() const;
    QVariantList allBooks() const;
    int totalBooks() const;
    QVariantList allReviews() const;
    int totalReviews() const;
    int flaggedReviewsCount() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void blockUser(const QString& username);
    Q_INVOKABLE void unblockUser(const QString& username);
    Q_INVOKABLE void deleteUser(const QString& username);
    Q_INVOKABLE void activateUser(const QString& username);     // v12: doc §4-2
    Q_INVOKABLE void deactivateUser(const QString& username);   // v12: doc §4-2
    Q_INVOKABLE void setUserRole(const QString& username, const QString& role);
    Q_INVOKABLE bool suspendMembership(const QString& username, int membershipIndex);
    Q_INVOKABLE bool reactivateMembership(const QString& username, int membershipIndex);
    Q_INVOKABLE bool cancelMembership(const QString& username, int membershipIndex);
    Q_INVOKABLE void approvePublisher(const QString& username);
    Q_INVOKABLE void rejectPublisher(const QString& username);
    Q_INVOKABLE void dismissFlaggedReview(const QString& id);
    Q_INVOKABLE void removeFlaggedReview(const QString& id);
    Q_INVOKABLE void dismissReport(const QString& id);
    Q_INVOKABLE void takeActionOnReport(const QString& id, const QString& action);
    Q_INVOKABLE void updateReportStatus(const QString& reportId, const QString& status);
    Q_INVOKABLE void assignReport(const QString& reportId, const QString& assignee);
    Q_INVOKABLE bool deleteBook(const QString& bookId, const QString& reason);
    Q_INVOKABLE bool setBookStatus(const QString& bookId, const QString& status);
    // BUG FIX (admin books real-time): added approveBook/rejectBook so the
    // AdminBookDetailDrawer's Approve/Reject buttons actually work. The
    // drawer was calling viewModel.approveBook(bookId) / rejectBook(bookId)
    // but those methods didn't exist on the VM — so the buttons were silent
    // no-ops (the `typeof === "function"` check failed and nothing happened).
    Q_INVOKABLE bool approveBook(const QString& bookId);
    Q_INVOKABLE bool rejectBook(const QString& bookId);
    Q_INVOKABLE bool updateBookInfo(const QString& bookId, const QString& title,
                                    const QString& author, const QString& genre,
                                    double price, const QString& description);
    Q_INVOKABLE QVariantMap bookDetails(const QString& bookId) const;
    Q_INVOKABLE QVariantList reviewsForBook(const QString& bookId) const;
    Q_INVOKABLE bool deleteReview(const QString& reviewId);
    Q_INVOKABLE bool approveReview(const QString& reviewId);

signals:
    void adminServiceChanged();
    void usersChanged();
    void publishersChanged();
    void moderationChanged();
    void reportsChanged();
    void auditLogChanged();
    void booksChanged();
    void reviewsChanged();

protected:
    void onAsyncReady() override;

private:
    AdminService* m_service = nullptr;
};

} // namespace bookclub::client
