#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUrl>

#include "common/AppEnums.h"

namespace bookclub::common {

class Book : public QObject {
    Q_OBJECT
public:
    explicit Book(QObject* parent = nullptr);
    Book(const QString& id, QObject* parent = nullptr);
    ~Book() override = default;

    const QString& id() const;
    const QString& title() const;
    const QString& authorName() const;
    const QString& publisherId() const;
    const QStringList& genreIds() const;
    const QString& description() const;
    const QString& coverImagePath() const;
    const QString& pdfFilePath() const;
    const QDateTime& createdAt() const;
    const QDateTime& updatedAt() const;
    double price() const;
    double basePrice() const;
    double discountValue() const;
    double averageRating() const;
    int ratingCount() const;
    int totalSales() const;
    int stockCount() const;
    bool isFree() const;
    bool isActive() const;
    bool isTimedDiscountActive() const;
    BookVisibility visibility() const;
    BookAvailability availability() const;

    void setId(const QString& id);
    void setTitle(const QString& title);
    void setAuthorName(const QString& authorName);
    void setPublisherId(const QString& publisherId);
    void setGenreIds(const QStringList& genreIds);
    void setDescription(const QString& description);
    void setCoverImagePath(const QString& path);
    void setPdfFilePath(const QString& path);
    void setCreatedAt(const QDateTime& createdAt);
    void setUpdatedAt(const QDateTime& updatedAt);
    void setPrice(double price);
    void setBasePrice(double price);
    void setDiscountValue(double discount);
    void setAverageRating(double rating);
    void setRatingCount(int count);
    void setTotalSales(int sales);
    void setStockCount(int count);
    void setVisibility(BookVisibility visibility);
    void setAvailability(BookAvailability availability);

    void activate();
    void deactivate();
    void archive();
    void applyPercentageDiscount(double percent);
    void applyFixedDiscount(double amount);
    void clearDiscount();
    void recalculateSellingPrice();

signals:
    void bookChanged();
    void priceChanged(double price);
    void visibilityChanged(bookclub::common::BookVisibility visibility);
    void availabilityChanged(bookclub::common::BookAvailability availability);

private:
    QString m_id;
    QString m_title;
    QString m_authorName;
    QString m_publisherId;
    QStringList m_genreIds;
    QString m_description;
    QString m_coverImagePath;
    QString m_pdfFilePath;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    double m_basePrice = 0.0;
    double m_price = 0.0;
    double m_discountValue = 0.0;
    double m_averageRating = 0.0;
    int m_ratingCount = 0;
    int m_totalSales = 0;
    int m_stockCount = 0;
    bool m_free = false;
    bool m_active = true;
    bool m_timedDiscountActive = false;
    BookVisibility m_visibility = BookVisibility::Visible;
    BookAvailability m_availability = BookAvailability::Available;
};

} // namespace bookclub::common
