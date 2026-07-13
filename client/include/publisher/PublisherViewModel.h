// =============================================================================
//  PublisherViewModel.h
// =============================================================================
#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {
class PublisherService;

class PublisherViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(PublisherService* publisherService READ publisherService WRITE setPublisherService NOTIFY publisherServiceChanged)
    Q_PROPERTY(QList<QObject*> books READ books NOTIFY booksChanged)
    Q_PROPERTY(int bookCount READ bookCount NOTIFY booksChanged)
    Q_PROPERTY(QVariantList topBooks READ topBooks NOTIFY booksChanged)
    Q_PROPERTY(QVariantList topViewedBooks READ topViewedBooks NOTIFY booksChanged)
    Q_PROPERTY(QVariantList leastSellingBooks READ leastSellingBooks NOTIFY booksChanged)
    Q_PROPERTY(QVariantList promotions READ promotions NOTIFY promotionsChanged)
    Q_PROPERTY(QVariantList publisherNotifications READ publisherNotifications NOTIFY notificationsChanged)
    Q_PROPERTY(QVariantList revenueSeries READ revenueSeries NOTIFY booksChanged)
    Q_PROPERTY(QVariantList genreBreakdown READ genreBreakdown NOTIFY booksChanged)
    Q_PROPERTY(QVariantList geographicBreakdown READ geographicBreakdown NOTIFY booksChanged)
    Q_PROPERTY(QVariantList activityFeed READ activityFeed NOTIFY booksChanged)
    Q_PROPERTY(QVariantList monthlyRevenue READ monthlyRevenue NOTIFY booksChanged)
    Q_PROPERTY(QVariantList recentOrders READ recentOrders NOTIFY booksChanged)
    Q_PROPERTY(QVariantList topBuyers READ topBuyers NOTIFY booksChanged)
    Q_PROPERTY(QVariantMap publisherProfile READ publisherProfile NOTIFY profileChanged)
    Q_PROPERTY(QString revenueTrend READ revenueTrend NOTIFY booksChanged)
    Q_PROPERTY(QString unitsSoldTrend READ unitsSoldTrend NOTIFY booksChanged)
    Q_PROPERTY(int repeatBuyerRate READ repeatBuyerRate NOTIFY booksChanged)
    Q_PROPERTY(QString totalRevenue READ totalRevenue NOTIFY booksChanged)
    Q_PROPERTY(int totalUnitsSold READ totalUnitsSold NOTIFY booksChanged)
    Q_PROPERTY(int activeTitles READ activeTitles NOTIFY booksChanged)
    Q_PROPERTY(int totalBooks READ totalBooks NOTIFY booksChanged)
    Q_PROPERTY(QString averageRating READ averageRating NOTIFY booksChanged)
    Q_PROPERTY(bool loading READ isBusy NOTIFY isBusyChanged)

public:
    explicit PublisherViewModel(QObject* parent = nullptr);
    PublisherService* publisherService() const { return m_service; }
    void setPublisherService(PublisherService* s);

    QList<QObject*> books() const;
    int bookCount() const;
    QVariantList topBooks() const;
    QVariantList topViewedBooks() const;
    QVariantList leastSellingBooks() const;
    QVariantList promotions() const;
    QVariantList publisherNotifications() const;
    QVariantList revenueSeries() const;
    QVariantList genreBreakdown() const;
    QVariantList geographicBreakdown() const;
    QVariantList activityFeed() const;
    QVariantList monthlyRevenue() const;
    QVariantList recentOrders() const;
    QVariantList topBuyers() const;
    QVariantMap publisherProfile() const;
    QString revenueTrend() const;
    QString unitsSoldTrend() const;
    int repeatBuyerRate() const;
    QString totalRevenue() const;
    int totalUnitsSold() const;
    int activeTitles() const;
    int totalBooks() const;
    QString averageRating() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString addBook(const QString& title, const QString& author, const QString& genre,
                             const QString& description, double price, double discountPercent,
                             const QString& coverColor, const QString& coverAccent,
                             const QString& coverImage = QString(),
                             const QString& pdfFilePath = QString());
    Q_INVOKABLE bool updateBook(const QString& bookId, const QString& title, const QString& author,
                                const QString& genre, const QString& description, double price,
                                double discountPercent, const QString& coverColor,
                                const QString& coverAccent,
                                const QString& coverImage = QString(),
                                const QString& pdfFilePath = QString());
    Q_INVOKABLE bool removeBook(const QString& bookId);
    Q_INVOKABLE bool setBookStatus(const QString& bookId, const QString& status);
    Q_INVOKABLE bool addPromotion(const QString& code, const QString& description,
                                  int discountPercent, int cap,
                                  const QString& startDate, const QString& endDate);
    Q_INVOKABLE bool removePromotion(const QString& code);
    Q_INVOKABLE QVariantMap bookDetail(const QString& bookId) const;
    Q_INVOKABLE bool updatePublisherProfile(const QString& publisherName,
                                             const QString& biography,
                                             const QString& website,
                                             const QString& email,
                                             const QString& taxId);
    Q_INVOKABLE void markAllNotificationsRead();
    Q_INVOKABLE void clearReadNotifications();
    Q_INVOKABLE void markNotificationRead(const QString& id, bool read);
    Q_INVOKABLE QVariantList ratingDistribution(const QString& bookId) const;

signals:
    void publisherServiceChanged();
    void booksChanged();
    void promotionsChanged();
    void notificationsChanged();
    void profileChanged();

protected:
    void onAsyncReady() override;

private:
    PublisherService* m_service = nullptr;
};

} // namespace bookclub::client
