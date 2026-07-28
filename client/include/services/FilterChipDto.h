// =============================================================================
//  FilterChipDto.h
// =============================================================================
//  Represents one active filter shown as a removable chip in the Search page.
//  { key, label, value }
// =============================================================================
#pragma once

#include <QObject>
#include <QQmlEngine>

namespace bookclub::client {

class FilterChipDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString key   READ key   CONSTANT)
    Q_PROPERTY(QString label READ label CONSTANT)
    Q_PROPERTY(QString value READ value CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)

public:
    explicit FilterChipDto(QObject* parent = nullptr) : QObject(parent) {}
    FilterChipDto(const QString& k, const QString& lbl, const QString& v, const QString& icon = "filter_alt", QObject* parent = nullptr)
        : QObject(parent), m_key(k), m_label(lbl), m_value(v), m_icon(icon) {}

    QString key() const { return m_key; }
    QString label() const { return m_label; }
    QString value() const { return m_value; }
    QString iconName() const { return m_icon; }

private:
    QString m_key, m_label, m_value, m_icon;
};

} // namespace bookclub::client
