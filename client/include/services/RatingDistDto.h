// =============================================================================
//  RatingDistDto.h
// =============================================================================
//  Tiny QObject wrapper around a single bar in the rating-distribution chart.
//  { stars: int (5..1), count: int }
// =============================================================================
#pragma once

#include <QObject>
#include <QQmlEngine>

namespace bookclub::client {

class RatingDistDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int stars READ stars CONSTANT)
    Q_PROPERTY(int count READ count CONSTANT)

public:
    explicit RatingDistDto(QObject* parent = nullptr) : QObject(parent) {}
    RatingDistDto(int s, int c, QObject* parent = nullptr) : QObject(parent), m_stars(s), m_count(c) {}

    int stars() const { return m_stars; }
    int count() const { return m_count; }

private:
    int m_stars = 5;
    int m_count = 0;
};

} // namespace bookclub::client
