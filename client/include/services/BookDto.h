#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QQmlEngine>
#include <QJsonObject>

namespace bookclub::client {

// Lightweight book DTO — populated from server JSON.
class BookDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id            READ id            CONSTANT)
    Q_PROPERTY(QString title         READ title         CONSTANT)
    Q_PROPERTY(QString authorName    READ authorName    CONSTANT)
    Q_PROPERTY(QString publisherName READ publisherName CONSTANT)
    Q_PROPERTY(QString publisherId   READ publisherId   CONSTANT)
    Q_PROPERTY(QStringList genreIds  READ genreIds      CONSTANT)
    Q_PROPERTY(QString description   READ description   CONSTANT)
    Q_PROPERTY(QString coverColor    READ coverColor    CONSTANT)
    Q_PROPERTY(QString coverAccent   READ coverAccent   CONSTANT)
    Q_PROPERTY(QString coverImage    READ coverImage    CONSTANT)
    Q_PROPERTY(QString pdfFilePath   READ pdfFilePath   CONSTANT)
    Q_PROPERTY(double   basePrice    READ basePrice     CONSTANT)
    Q_PROPERTY(double   price        READ price         CONSTANT)
    Q_PROPERTY(double   discountValue READ discountValue CONSTANT)
    Q_PROPERTY(double   averageRating READ averageRating CONSTANT)
    Q_PROPERTY(int      ratingCount  READ ratingCount   CONSTANT)
    Q_PROPERTY(int      totalSales   READ totalSales    CONSTANT)
    Q_PROPERTY(bool     isFree       READ isFree        CONSTANT)
    Q_PROPERTY(bool     hasDiscount  READ hasDiscount   CONSTANT)
    Q_PROPERTY(int      discountPercent READ discountPercent CONSTANT)
    Q_PROPERTY(QString priceText     READ priceText     CONSTANT)
    Q_PROPERTY(QString basePriceText READ basePriceText CONSTANT)
    Q_PROPERTY(bool     inWishlist   READ inWishlist    NOTIFY inWishlistChanged)
    Q_PROPERTY(bool     purchased    READ purchased     NOTIFY purchasedChanged)
    Q_PROPERTY(QStringList genres    READ genreIds      CONSTANT)
    Q_PROPERTY(QString createdAtText READ createdAtText CONSTANT)
    Q_PROPERTY(QDateTime createdAt   READ createdAt     CONSTANT)
    Q_PROPERTY(QString ageText       READ ageText       CONSTANT)
    Q_PROPERTY(QString status        READ status        CONSTANT)

public:
    explicit BookDto(QObject* parent = nullptr) : QObject(parent) {}

    // Populate from a JSON object (server response)
    void fromJson(const QJsonObject& j);

    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QString authorName() const { return m_authorName; }
    QString publisherName() const { return m_publisherName; }
    QString publisherId() const { return m_publisherId; }
    QStringList genreIds() const { return m_genreIds; }
    QString description() const { return m_description; }
    QString coverColor() const { return m_coverColor; }
    QString coverAccent() const { return m_coverAccent; }
    QString coverImage() const { return m_coverImage; }
    QString pdfFilePath() const { return m_pdfFilePath; }
    double basePrice() const { return m_basePrice; }
    double price() const { return m_price; }
    double discountValue() const { return m_discountValue; }
    double averageRating() const { return m_averageRating; }
    int ratingCount() const { return m_ratingCount; }
    int totalSales() const { return m_totalSales; }
    bool isFree() const { return m_isFree; }
    bool hasDiscount() const { return m_discountValue > 0; }
    int discountPercent() const;
    QString priceText() const;
    QString basePriceText() const;
    bool inWishlist() const { return m_inWishlist; }
    bool purchased() const { return m_purchased; }
    QString createdAtText() const { return m_createdAtText; }
    QDateTime createdAt() const { return m_createdAt; }
    QString ageText() const { return m_createdAtText; }
    QString status() const { return m_active ? QStringLiteral("active") : QStringLiteral("inactive"); }

    void setInWishlist(bool v) { if (m_inWishlist != v) { m_inWishlist = v; emit inWishlistChanged(); } }
    void setPurchased(bool v) { if (m_purchased != v) { m_purchased = v; emit purchasedChanged(); } }

signals:
    void inWishlistChanged();
    void purchasedChanged();

private:
    QString m_id, m_title, m_authorName, m_publisherName, m_publisherId;
    QStringList m_genreIds;
    QString m_description, m_coverColor, m_coverAccent, m_coverImage, m_pdfFilePath;
    double m_basePrice = 0, m_price = 0, m_discountValue = 0, m_averageRating = 0;
    int m_ratingCount = 0, m_totalSales = 0;
    bool m_isFree = false, m_inWishlist = false, m_purchased = false, m_active = true;
    QString m_createdAtText;
    QDateTime m_createdAt;
};

} // namespace bookclub::client
