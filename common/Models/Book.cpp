#include "common/Models/Book.h"
#include <QtMath>

namespace bookclub::common {

Book::Book(QObject* parent) : QObject(parent) {}

Book::Book(const QString& id, QObject* parent) : QObject(parent), m_id(id) {}

// ---- Getters ----
const QString& Book::id() const { return m_id; }
const QString& Book::title() const { return m_title; }
const QString& Book::authorName() const { return m_authorName; }
const QString& Book::publisherId() const { return m_publisherId; }
const QStringList& Book::genreIds() const { return m_genreIds; }
const QString& Book::description() const { return m_description; }
const QString& Book::coverImagePath() const { return m_coverImagePath; }
const QString& Book::pdfFilePath() const { return m_pdfFilePath; }
const QDateTime& Book::createdAt() const { return m_createdAt; }
const QDateTime& Book::updatedAt() const { return m_updatedAt; }
double Book::price() const { return m_price; }
double Book::basePrice() const { return m_basePrice; }
double Book::discountValue() const { return m_discountValue; }
double Book::averageRating() const { return m_averageRating; }
int Book::ratingCount() const { return m_ratingCount; }
int Book::totalSales() const { return m_totalSales; }
int Book::stockCount() const { return m_stockCount; }
bool Book::isFree() const { return m_free; }
bool Book::isActive() const { return m_active; }
bool Book::isTimedDiscountActive() const { return m_timedDiscountActive; }
BookVisibility Book::visibility() const { return m_visibility; }
BookAvailability Book::availability() const { return m_availability; }

// ---- Setters ----
void Book::setId(const QString& id) { m_id = id; }
void Book::setTitle(const QString& title) {
    if (m_title != title) {
        m_title = title;
        emit bookChanged();
    }
}
void Book::setAuthorName(const QString& authorName) {
    if (m_authorName != authorName) {
        m_authorName = authorName;
        emit bookChanged();
    }
}
void Book::setPublisherId(const QString& publisherId) {
    if (m_publisherId != publisherId) {
        m_publisherId = publisherId;
        emit bookChanged();
    }
}
void Book::setGenreIds(const QStringList& genreIds) {
    if (m_genreIds != genreIds) {
        m_genreIds = genreIds;
        emit bookChanged();
    }
}
void Book::setDescription(const QString& description) {
    if (m_description != description) {
        m_description = description;
        emit bookChanged();
    }
}
void Book::setCoverImagePath(const QString& path) {
    if (m_coverImagePath != path) {
        m_coverImagePath = path;
        emit bookChanged();
    }
}
void Book::setPdfFilePath(const QString& path) {
    if (m_pdfFilePath != path) {
        m_pdfFilePath = path;
        emit bookChanged();
    }
}
void Book::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
void Book::setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

void Book::setPrice(double price) {
    if (qFuzzyCompare(m_price, price)) return;
    m_price = price;
    emit priceChanged(price);
    emit bookChanged();
}

void Book::setBasePrice(double price) {
    if (qFuzzyCompare(m_basePrice, price)) return;
    m_basePrice = price;
    recalculateSellingPrice();
}

void Book::setDiscountValue(double discount) {
    if (qFuzzyCompare(m_discountValue, discount)) return;
    m_discountValue = discount;
    recalculateSellingPrice();
}

void Book::setAverageRating(double rating) {
    if (qFuzzyCompare(m_averageRating, rating)) return;
    m_averageRating = rating;
    emit bookChanged();
}

void Book::setRatingCount(int count) {
    if (m_ratingCount == count) return;
    m_ratingCount = count;
    emit bookChanged();
}

void Book::setTotalSales(int sales) {
    if (m_totalSales == sales) return;
    m_totalSales = sales;
    emit bookChanged();
}

void Book::setStockCount(int count) {
    if (m_stockCount == count) return;
    m_stockCount = count;
    emit bookChanged();
}

void Book::setVisibility(BookVisibility visibility) {
    if (m_visibility == visibility) return;
    m_visibility = visibility;
    emit visibilityChanged(visibility);
    emit bookChanged();
}

void Book::setAvailability(BookAvailability availability) {
    if (m_availability == availability) return;
    m_availability = availability;
    emit availabilityChanged(availability);
    emit bookChanged();
}

// ---- Business logic ----
void Book::activate() {
    if (!m_active) {
        m_active = true;
        emit bookChanged();
    }
}

void Book::deactivate() {
    if (m_active) {
        m_active = false;
        emit bookChanged();
    }
}

void Book::archive() {
    m_active = false;
    m_visibility = BookVisibility::Archived;
    emit bookChanged();
}

void Book::applyPercentageDiscount(double percent) {
    if (percent < 0.0 || percent > 100.0) return;
    m_discountValue = m_basePrice * (percent / 100.0);
    m_timedDiscountActive = true;
    recalculateSellingPrice();
}

void Book::applyFixedDiscount(double amount) {
    if (amount < 0.0 || amount > m_basePrice) return;
    m_discountValue = amount;
    m_timedDiscountActive = true;
    recalculateSellingPrice();
}

void Book::clearDiscount() {
    m_discountValue = 0.0;
    m_timedDiscountActive = false;
    recalculateSellingPrice();
}

void Book::recalculateSellingPrice() {
    double newPrice = m_basePrice - m_discountValue;
    if (newPrice < 0.0) newPrice = 0.0;
    m_price = newPrice;
    m_free = qFuzzyIsNull(m_price);
    emit priceChanged(m_price);
    emit bookChanged();
}

} // namespace bookclub::common
