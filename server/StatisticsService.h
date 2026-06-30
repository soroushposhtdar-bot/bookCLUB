#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QPair>

namespace bookclub::common {
class PublisherStats;
}

namespace bookclub::server {

class StatisticsService : public QObject {
    Q_OBJECT
public:
    explicit StatisticsService(QObject* parent = nullptr);
    ~StatisticsService() override = default;

    bookclub::common::PublisherStats* buildPublisherStats(const QString& publisherId) const;
    QVector<QPair<QString, int>> dailySales(const QString& publisherId) const;
    QVector<QPair<QString, int>> monthlySales(const QString& publisherId) const;
    QVector<QPair<QString, double>> averageRatings(const QString& publisherId) const;

signals:
    void statsUpdated(const QString& publisherId);
};

} // namespace bookclub::server
