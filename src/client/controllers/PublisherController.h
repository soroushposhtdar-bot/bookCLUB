// src/client/controllers/PublisherController.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class PublisherController : public QObject {
    Q_OBJECT
public:
    explicit PublisherController(QObject* parent = nullptr);
    ~PublisherController() override;

    // ---- Public Methods ----
    void loadMyBooks();
    void createBook();
    void updateBook(const QString& bookId);
    void deactivateBook(const QString& bookId);
    void activateBook(const QString& bookId);
    void applyTimedDiscount(const QString& bookId);
    void loadAnalytics();

    // ---- Setter Methods ----
    void setBookData(const QJsonObject& bookData);
    void setBookField(const QString& field, const QJsonValue& value);
    void clearBookData();
    void setDiscountData(const QJsonObject& discountData);
    void setDiscountField(const QString& field, const QJsonValue& value);
    void clearDiscountData();

    // ---- Accessors ----
    QJsonArray getMyBooks() const;
    QJsonObject getBook(const QString& bookId) const;
    int getBookCount() const;
    double getTotalRevenue() const;
    int getTotalSales() const;
    int getTotalBooksPublished() const;
    QJsonArray getBookStats() const;
    QJsonObject getBookStat(const QString& bookId) const;

signals:
    void bookListChanged();
    void analyticsChanged();
    void errorOccurred(const QString& message);

private:
    void handleGetPublisherBooksResponse(const common::Message& response);
    void handlePublishBookResponse(const common::Message& response);
    void handleUpdateBookResponse(const common::Message& response);
    void handleDeactivateBookResponse(const common::Message& response);
    void handleActivateBookResponse(const common::Message& response);
    void handleApplyTimedDiscountResponse(const common::Message& response);
    void handleGetPublisherAnalyticsResponse(const common::Message& response);

    QJsonObject m_bookData;
    QJsonObject m_discountData;
    QJsonObject m_booksData;
    QJsonObject m_analyticsData;
};

} // namespace bookclub::client
