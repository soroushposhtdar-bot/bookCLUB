#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJsonObject>
#include <QLocale>

namespace bookclub::client {

class CartItemDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString bookId        READ bookId        CONSTANT)
    Q_PROPERTY(QString bookTitle     READ bookTitle     CONSTANT)
    Q_PROPERTY(QString title         READ bookTitle     CONSTANT)
    Q_PROPERTY(QString authorName    READ authorName    CONSTANT)
    Q_PROPERTY(QString publisherName READ publisherName CONSTANT)
    Q_PROPERTY(QString coverColor    READ coverColor    CONSTANT)
    Q_PROPERTY(QString coverAccent   READ coverAccent   CONSTANT)
    Q_PROPERTY(QString description   READ description   CONSTANT)
    Q_PROPERTY(double   unitPrice    READ unitPrice     CONSTANT)
    Q_PROPERTY(double   basePrice    READ basePrice     CONSTANT)
    Q_PROPERTY(double   discountedUnitPrice READ discountedUnitPrice CONSTANT)
    Q_PROPERTY(double   discountValue READ discountValue CONSTANT)
    Q_PROPERTY(int      quantity     READ quantity      CONSTANT)
    Q_PROPERTY(double   lineTotal    READ lineTotal     CONSTANT)
    Q_PROPERTY(QString  unitPriceText READ unitPriceText CONSTANT)
    Q_PROPERTY(QString  basePriceText READ basePriceText CONSTANT)
    Q_PROPERTY(QString  lineTotalText READ lineTotalText CONSTANT)
    Q_PROPERTY(bool     hasDiscount  READ hasDiscount   CONSTANT)
    Q_PROPERTY(int      discountPercent READ discountPercent CONSTANT)
    Q_PROPERTY(double   averageRating READ averageRating CONSTANT)
    Q_PROPERTY(int      ratingCount  READ ratingCount   CONSTANT)

public:
    explicit CartItemDto(QObject* parent = nullptr) : QObject(parent) {}

    QString bookId() const { return m_bookId; }
    QString bookTitle() const { return m_bookTitle; }
    QString authorName() const { return m_authorName; }
    QString publisherName() const { return m_publisherName; }
    QString coverColor() const { return m_coverColor; }
    QString coverAccent() const { return m_coverAccent; }
    QString description() const { return m_description; }
    double unitPrice() const { return m_unitPrice; }
    double basePrice() const { return m_basePrice; }
    double discountedUnitPrice() const { return m_discounted; }
    double discountValue() const { return m_basePrice > m_unitPrice ? m_basePrice - m_unitPrice : 0.0; }
    int quantity() const { return m_quantity; }
    double lineTotal() const { return m_unitPrice * m_quantity; }
    QString unitPriceText() const { return QLocale().toString(m_unitPrice, 'f', 2); }
    QString basePriceText() const { return QLocale().toString(m_basePrice, 'f', 2); }
    QString lineTotalText() const { return QLocale().toString(lineTotal(), 'f', 2); }
    bool hasDiscount() const { return m_basePrice > m_unitPrice + 0.001; }
    int discountPercent() const {
        if (m_basePrice <= 0.001) return 0;
        return int((m_basePrice - m_unitPrice) / m_basePrice * 100.0 + 0.5);
    }
    double averageRating() const { return m_averageRating; }
    int ratingCount() const { return m_ratingCount; }

    void fromJson(const QJsonObject& j) {
        m_bookId        = j.value("bookId").toString();
        m_bookTitle     = j.value("bookTitle").toString();
        m_authorName    = j.value("authorName").toString();
        m_publisherName = j.value("publisherName").toString();
        m_coverColor    = j.value("coverColor").toString();
        m_coverAccent   = j.value("coverAccent").toString();
        m_description   = j.value("description").toString();
        // v15d: the server sends:
        //   unitPrice           = basePrice (ORIGINAL price — misleading name)
        //   discountedUnitPrice = the actual selling price (after discount)
        //   basePrice           = the original price (from Books table)
        //   discountValue       = absolute discount amount
        //
        // For the DTO, we want:
        //   m_unitPrice  = the SELLING price (what the user pays) = discountedUnitPrice
        //   m_basePrice  = the ORIGINAL price (before discount) = basePrice
        // so that hasDiscount = (m_basePrice > m_unitPrice) works correctly
        // and discountValue = m_basePrice - m_unitPrice is non-zero when
        // there's a discount.
        m_basePrice     = j.value("basePrice").toDouble(j.value("unitPrice").toDouble());
        m_discounted    = j.value("discountedUnitPrice").toDouble(m_basePrice);
        // m_unitPrice = the selling price. Prefer discountedUnitPrice;
        // fall back to unitPrice (which the server sets to basePrice) when
        // discountedUnitPrice is 0 or missing (no discount).
        const double serverUnitPrice = j.value("unitPrice").toDouble();
        if (m_discounted > 0.001 && m_discounted < m_basePrice) {
            m_unitPrice = m_discounted;  // discounted selling price
        } else {
            m_unitPrice = serverUnitPrice > 0.001 ? serverUnitPrice : m_basePrice;
        }
        m_quantity      = j.value("quantity").toInt(1);
        m_averageRating = j.value("averageRating").toDouble();
        m_ratingCount   = j.value("ratingCount").toInt();
    }

private:
    QString m_bookId, m_bookTitle, m_authorName, m_publisherName;
    QString m_coverColor, m_coverAccent, m_description;
    double m_unitPrice = 0, m_basePrice = 0, m_discounted = 0, m_averageRating = 0;
    int m_quantity = 1, m_ratingCount = 0;
};

} // namespace bookclub::client
