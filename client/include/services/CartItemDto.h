// =============================================================================
//  CartItemDto.h
// =============================================================================
//  Thin QObject wrapper around a cart row, exposed to QML via Q_PROPERTY.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>

namespace bookclub::client {

class CartItemDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString bookId      READ bookId      CONSTANT)
    Q_PROPERTY(QString title       READ title       CONSTANT)
    Q_PROPERTY(QString authorName  READ authorName  CONSTANT)
    Q_PROPERTY(QString coverColor  READ coverColor  CONSTANT)
    Q_PROPERTY(QString coverAccent READ coverAccent CONSTANT)
    Q_PROPERTY(double  unitPrice   READ unitPrice   CONSTANT)
    Q_PROPERTY(double  basePrice   READ basePrice   CONSTANT)
    Q_PROPERTY(double  discountAmount READ discountAmount CONSTANT)
    Q_PROPERTY(int     quantity    READ quantity    CONSTANT)
    Q_PROPERTY(bool    hasDiscount READ hasDiscount CONSTANT)
    Q_PROPERTY(QString unitPriceText READ unitPriceText CONSTANT)
    Q_PROPERTY(QString basePriceText READ basePriceText CONSTANT)

public:
    explicit CartItemDto(QObject* parent = nullptr) : QObject(parent) {}

    QString bookId() const { return m_bookId; }
    QString title() const { return m_title; }
    QString authorName() const { return m_authorName; }
    QString coverColor() const { return m_coverColor; }
    QString coverAccent() const { return m_coverAccent; }
    double unitPrice() const { return m_unitPrice; }
    double basePrice() const { return m_basePrice; }
    double discountAmount() const { return m_discountAmount; }
    int quantity() const { return m_quantity; }
    bool hasDiscount() const { return m_discountAmount > 0.0; }
    QString unitPriceText() const {
        if (m_unitPrice <= 0.0) return QStringLiteral("Free");
        return QStringLiteral("$%1").arg(m_unitPrice, 0, 'f', 2);
    }
    QString basePriceText() const {
        return QStringLiteral("$%1").arg(m_basePrice, 0, 'f', 2);
    }

    QString m_bookId, m_title, m_authorName, m_coverColor, m_coverAccent;
    double m_unitPrice = 0.0;
    double m_basePrice = 0.0;
    double m_discountAmount = 0.0;
    int m_quantity = 1;
};

} // namespace bookclub::client
