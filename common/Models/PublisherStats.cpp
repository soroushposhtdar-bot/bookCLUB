#include "common/Models/PublisherStats.h"
#include <numeric>

namespace bookclub::common {

// ---- BookStatItem ----
BookStatItem::BookStatItem(QObject* parent) : QObject(parent) {}

const QString& BookStatItem::bookId() const { return m_bookId; }
const QString& BookStatItem::title() const { return m_title; }
int BookStatItem::salesCount() const { return m_salesCount; }
double BookStatItem::revenue() const { return m_revenue; }
double BookStatItem::averageRating() const { return m_averageRating; }

void BookStatItem::setBookId(const QString& bookId) { m_bookId = bookId; }
void BookStatItem::setTitle(const QString& title) { m_title = title; }
void BookStatItem::setSalesCount(int salesCount) { m_salesCount = salesCount; }
void BookStatItem::setRevenue(double revenue) { m_revenue = revenue; }
void BookStatItem::setAverageRating(double averageRating) { m_averageRating = averageRating; }

// ---- PublisherStats ----
PublisherStats::PublisherStats(QObject* parent) : QObject(parent) {}

PublisherStats::~PublisherStats() {
    qDeleteAll(m_bookStats);
}

const QString& PublisherStats::publisherId() const { return m_publisherId; }
const QVector<BookStatItem*>& PublisherStats::bookStats() const { return m_bookStats; }
int PublisherStats::totalBooks() const { return m_totalBooks; }
int PublisherStats::totalSales() const { return m_totalSales; }
double PublisherStats::totalRevenue() const { return m_totalRevenue; }
const QDateTime& PublisherStats::updatedAt() const { return m_updatedAt; }

void PublisherStats::setPublisherId(const QString& publisherId) { m_publisherId = publisherId; }

void PublisherStats::setBookStats(const QVector<BookStatItem*>& bookStats) {
    qDeleteAll(m_bookStats);
    m_bookStats = bookStats;
    recalculate();
    emit statsChanged();
}

void PublisherStats::setTotalBooks(int totalBooks) { m_totalBooks = totalBooks; }
void PublisherStats::setTotalSales(int totalSales) { m_totalSales = totalSales; }
void PublisherStats::setTotalRevenue(double totalRevenue) { m_totalRevenue = totalRevenue; }
void PublisherStats::setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

void PublisherStats::recalculate() {
    int totalSales = 0;
    double totalRevenue = 0.0;
    for (const BookStatItem* item : m_bookStats) {
        totalSales += item->salesCount();
        totalRevenue += item->revenue();
    }
    m_totalSales = totalSales;
    m_totalRevenue = totalRevenue;
    m_totalBooks = m_bookStats.size();
    m_updatedAt = QDateTime::currentDateTime();
    emit statsChanged();
}

} // namespace bookclub::common
