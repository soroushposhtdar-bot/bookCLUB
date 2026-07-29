#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

#include "common/AppEnums.h"

namespace bookclub::common {

class Discount : public QObject {
    Q_OBJECT
public:
    explicit Discount(QObject* parent = nullptr);
    Discount(const QString& id, QObject* parent = nullptr);
    ~Discount() override = default;

    const QString& id() const;
    const QString& bookId() const;
    DiscountType type() const;
    double value() const;
    const QDateTime& startsAt() const;
    const QDateTime& endsAt() const;
    bool isActive() const;
    bool isExpired() const;
    bool isScheduled() const;

    void setId(const QString& id);
    void setBookId(const QString& bookId);
    void setType(DiscountType type);
    void setValue(double value);
    void setStartsAt(const QDateTime& startsAt);
    void setEndsAt(const QDateTime& endsAt);
    void setActive(bool active);

signals:
    void discountChanged();
    void discountExpired();

private:
    QString m_id;
    QString m_bookId;
    DiscountType m_type = DiscountType::Percentage;
    double m_value = 0.0;
    QDateTime m_startsAt;
    QDateTime m_endsAt;
    bool m_active = false;
};

} // namespace bookclub::common
