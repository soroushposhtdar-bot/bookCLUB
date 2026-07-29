#include "common/Models/Discount.h"

namespace bookclub::common {

Discount::Discount(QObject* parent) : QObject(parent) {}

Discount::Discount(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}

const QString& Discount::id() const { return m_id; }
const QString& Discount::bookId() const { return m_bookId; }
DiscountType Discount::type() const { return m_type; }
double Discount::value() const { return m_value; }
const QDateTime& Discount::startsAt() const { return m_startsAt; }
const QDateTime& Discount::endsAt() const { return m_endsAt; }

bool Discount::isActive() const {
    if (!m_active) return false;
    QDateTime now = QDateTime::currentDateTime();
    return (m_startsAt.isValid() && now >= m_startsAt) &&
           (m_endsAt.isValid() && now <= m_endsAt);
}

bool Discount::isExpired() const {
    if (!m_active) return true;
    QDateTime now = QDateTime::currentDateTime();
    return m_endsAt.isValid() && now > m_endsAt;
}

bool Discount::isScheduled() const {
    QDateTime now = QDateTime::currentDateTime();
    return m_active && m_startsAt.isValid() && now < m_startsAt;
}

void Discount::setId(const QString& id) { m_id = id; }
void Discount::setBookId(const QString& bookId) { m_bookId = bookId; }
void Discount::setType(DiscountType type) { m_type = type; }
void Discount::setValue(double value) { m_value = value; }
void Discount::setStartsAt(const QDateTime& startsAt) { m_startsAt = startsAt; }

void Discount::setEndsAt(const QDateTime& endsAt) {
    if (m_endsAt != endsAt) {
        m_endsAt = endsAt;
        emit discountChanged();
    }
}

void Discount::setActive(bool active) {
    if (m_active != active) {
        m_active = active;
        if (!active) {
            emit discountExpired();
        }
        emit discountChanged();
    }
}

} // namespace bookclub::common
