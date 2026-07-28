// common/Models/Author.h
//
// Author domain model. Mirrors the Authors table.
#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

namespace bookclub::common {

class Author : public QObject {
    Q_OBJECT
public:
    explicit Author(QObject* parent = nullptr);
    Author(const QString& id, const QString& name, QObject* parent = nullptr);
    ~Author() override = default;

    const QString& id() const { return m_id; }
    const QString& name() const { return m_name; }
    const QString& biography() const { return m_biography; }
    const QDateTime& createdAt() const { return m_createdAt; }

    void setId(const QString& id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setBiography(const QString& bio) { m_biography = bio; }
    void setCreatedAt(const QDateTime& ts) { m_createdAt = ts; }

private:
    QString m_id;
    QString m_name;
    QString m_biography;
    QDateTime m_createdAt;
};

} // namespace bookclub::common
