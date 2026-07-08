#include "common/Models/Review.h"

namespace bookclub::common {

Review::Review(QObject* parent) : QObject(parent) {}

Review::Review(const QString& id, QObject* parent) : QObject(parent), m_id(id) {}

const QString& Review::id() const { return m_id; }
const QString& Review::bookId() const { return m_bookId; }
const QString& Review::userId() const { return m_userId; }
const QString& Review::userDisplayName() const { return m_userDisplayName; }
const QString& Review::text() const { return m_text; }
const QDateTime& Review::createdAt() const { return m_createdAt; }
const QDateTime& Review::updatedAt() const { return m_updatedAt; }
int Review::stars() const { return m_stars; }
bool Review::isEdited() const { return m_edited; }

void Review::setId(const QString& id) { m_id = id; }
void Review::setBookId(const QString& bookId) { m_bookId = bookId; }
void Review::setUserId(const QString& userId) { m_userId = userId; }
void Review::setUserDisplayName(const QString& name) { m_userDisplayName = name; }

void Review::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        m_edited = true;
        m_updatedAt = QDateTime::currentDateTime();
        emit reviewChanged();
    }
}

void Review::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
void Review::setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

void Review::setStars(int stars) {
    if (stars < 1) stars = 1;
    if (stars > 5) stars = 5;
    if (m_stars != stars) {
        m_stars = stars;
        emit reviewChanged();
    }
}

void Review::setEdited(bool edited) { m_edited = edited; }

} // namespace bookclub::common
