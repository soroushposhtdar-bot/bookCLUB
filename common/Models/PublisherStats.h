#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

namespace bookclub::common {

class BookStatItem : public QObject {
    Q_OBJECT
public:
    explicit BookStatItem(QObject* parent = nullptr);
    ~BookStatItem() override = default;

    const QString& bookId() const;
    const QString& title() const;
    int salesCount() const;
    double revenue() const;
    double averageRating() const;

    void setBookId(const QString& bookId);
    void setTitle(const QString& title);
    void setSalesCount(int salesCount);
    void setRevenue(double revenue);
    void setAverageRating(double averageRating);

private:
    QString m_bookId;
    QString m_title;
    int m_salesCount = 0;
    double m_revenue = 0.0;
    double m_averageRating = 0.0;
};

class PublisherStats : public QObject {
    Q_OBJECT
public:
    explicit PublisherStats(QObject* parent = nullptr);
    ~PublisherStats() override;

    const QString& publisherId() const;
    const QVector<BookStatItem*>& bookStats() const;
    int totalBooks() const;
    int totalSales() const;
    double totalRevenue() const;
    const QDateTime& updatedAt() const;

    void setPublisherId(const QString& publisherId);
    void setBookStats(const QVector<BookStatItem*>& bookStats);
    void setTotalBooks(int totalBooks);
    void setTotalSales(int totalSales);
    void setTotalRevenue(double totalRevenue);
    void setUpdatedAt(const QDateTime& updatedAt);
    void recalculate();

signals:
    void statsChanged();

private:
    QString m_publisherId;
    QVector<BookStatItem*> m_bookStats;
    int m_totalBooks = 0;
    int m_totalSales = 0;
    double m_totalRevenue = 0.0;
    QDateTime m_updatedAt;
};

} // namespace bookclub::common
