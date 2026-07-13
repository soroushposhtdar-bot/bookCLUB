// =============================================================================
//  PublisherViewModel.cpp
// =============================================================================
#include "viewmodels/publisher/PublisherViewModel.h"
#include "services/PublisherService.h"

namespace bookclub::client {

PublisherViewModel::PublisherViewModel(QObject* parent) : UserViewModelBase(parent) {}

void PublisherViewModel::setPublisherService(PublisherService* s) {
    if (m_service == s) return;
    if (m_service) disconnect(m_service, nullptr, this, nullptr);
    m_service = s;
    if (m_service) {
        connect(m_service, &PublisherService::booksChanged, this, &PublisherViewModel::booksChanged);
        connect(m_service, &PublisherService::promotionsChanged, this, &PublisherViewModel::promotionsChanged);
        connect(m_service, &PublisherService::notificationsChanged, this, &PublisherViewModel::notificationsChanged);
        connect(m_service, &PublisherService::profileChanged, this, &PublisherViewModel::profileChanged);
    }
    emit publisherServiceChanged();
    emit booksChanged();
    emit promotionsChanged();
    emit notificationsChanged();
    emit profileChanged();
}

QList<QObject*> PublisherViewModel::books() const { return m_service ? m_service->publisherBooks() : QList<QObject*>{}; }
int PublisherViewModel::bookCount() const { return m_service ? m_service->publisherBooks().size() : 0; }
QVariantList PublisherViewModel::topBooks() const { return m_service ? m_service->topBooks() : QVariantList{}; }
QVariantList PublisherViewModel::topViewedBooks() const { return m_service ? m_service->topViewedBooksVariant(5) : QVariantList{}; }
QVariantList PublisherViewModel::leastSellingBooks() const { return m_service ? m_service->leastSellingBooks(5) : QVariantList{}; }
QVariantList PublisherViewModel::promotions() const { return m_service ? m_service->promotions() : QVariantList{}; }
QVariantList PublisherViewModel::publisherNotifications() const { return m_service ? m_service->publisherNotifications() : QVariantList{}; }
QVariantList PublisherViewModel::revenueSeries() const { return m_service ? m_service->revenueSeries(14) : QVariantList{}; }
QVariantList PublisherViewModel::genreBreakdown() const { return m_service ? m_service->genreBreakdown() : QVariantList{}; }
QVariantList PublisherViewModel::geographicBreakdown() const { return m_service ? m_service->geographicBreakdown() : QVariantList{}; }
QVariantList PublisherViewModel::activityFeed() const { return m_service ? m_service->activityFeed(8) : QVariantList{}; }
QVariantList PublisherViewModel::monthlyRevenue() const { return m_service ? m_service->monthlyRevenue(12) : QVariantList{}; }
QVariantList PublisherViewModel::recentOrders() const { return m_service ? m_service->recentOrders(10) : QVariantList{}; }
QVariantList PublisherViewModel::topBuyers() const { return m_service ? m_service->topBuyers(5) : QVariantList{}; }
QVariantMap PublisherViewModel::publisherProfile() const { return m_service ? m_service->publisherProfile() : QVariantMap{}; }
QString PublisherViewModel::revenueTrend() const { return m_service ? m_service->revenueTrend() : QStringLiteral("+0.0%"); }
QString PublisherViewModel::unitsSoldTrend() const { return m_service ? m_service->unitsSoldTrend() : QStringLiteral("+0.0%"); }
int PublisherViewModel::repeatBuyerRate() const { return m_service ? m_service->repeatBuyerRate() : 0; }
QString PublisherViewModel::totalRevenue() const { return m_service ? m_service->totalRevenue() : QStringLiteral("$0"); }
int PublisherViewModel::totalUnitsSold() const { return m_service ? m_service->totalUnitsSold() : 0; }
int PublisherViewModel::activeTitles() const { return m_service ? m_service->activeTitleCount() : 0; }
int PublisherViewModel::totalBooks() const { return m_service ? m_service->totalBooks() : 0; }
QString PublisherViewModel::averageRating() const { return m_service ? m_service->averageRating() : QStringLiteral("0.00"); }

void PublisherViewModel::refresh() { beginAsync(400); }
void PublisherViewModel::onAsyncReady() {
    emit booksChanged(); emit promotionsChanged(); emit notificationsChanged();
    emit profileChanged(); finishAsync();
}

QString PublisherViewModel::addBook(const QString& title, const QString& author, const QString& genre,
                                  const QString& description, double price, double discountPercent,
                                  const QString& coverColor, const QString& coverAccent,
                                  const QString& coverImage,
                                  const QString& pdfFilePath) {
    if (!m_service) return {}; return m_service->addBook(title, author, genre, description, price, discountPercent, coverColor, coverAccent, coverImage, pdfFilePath);
}
bool PublisherViewModel::updateBook(const QString& bookId, const QString& title, const QString& author,
                                     const QString& genre, const QString& description, double price,
                                     double discountPercent, const QString& coverColor,
                                     const QString& coverAccent,
                                     const QString& coverImage,
                                     const QString& pdfFilePath) {
    if (!m_service) return false; return m_service->updateBook(bookId, title, author, genre, description, price, discountPercent, coverColor, coverAccent, coverImage, pdfFilePath);
}
bool PublisherViewModel::removeBook(const QString& bookId) { if (!m_service) return false; return m_service->removeBook(bookId); }
bool PublisherViewModel::setBookStatus(const QString& bookId, const QString& status) { if (!m_service) return false; return m_service->setBookStatus(bookId, status); }
bool PublisherViewModel::addPromotion(const QString& code, const QString& description, int discountPercent, int cap, const QString& startDate, const QString& endDate) {
    if (!m_service) return false; return m_service->addPromotion(code, description, discountPercent, cap, startDate, endDate);
}
bool PublisherViewModel::removePromotion(const QString& code) { if (!m_service) return false; return m_service->removePromotion(code); }
QVariantMap PublisherViewModel::bookDetail(const QString& bookId) const { return m_service ? m_service->bookDetail(bookId) : QVariantMap{}; }
bool PublisherViewModel::updatePublisherProfile(const QString& publisherName, const QString& biography, const QString& website, const QString& email, const QString& taxId) {
    if (!m_service) return false; return m_service->updatePublisherProfile(publisherName, biography, website, email, taxId);
}
void PublisherViewModel::markAllNotificationsRead() { if (m_service) m_service->markAllNotificationsRead(); }
void PublisherViewModel::clearReadNotifications() { if (m_service) m_service->clearReadNotifications(); }
void PublisherViewModel::markNotificationRead(const QString& id, bool read) { if (m_service) m_service->markNotificationRead(id, read); }
QVariantList PublisherViewModel::ratingDistribution(const QString& bookId) const { return m_service ? m_service->ratingDistribution(bookId) : QVariantList{}; }

} // namespace bookclub::client
