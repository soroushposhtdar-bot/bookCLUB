#pragma once
#include <QString>
#include <QStringList>

namespace bookclub::common {

class Genre {
public:
    Genre() = default;
    explicit Genre(QString id, QString name);

    const QString& id() const;
    const QString& name() const;
    const QStringList& aliases() const;

    void setId(const QString& id);
    void setName(const QString& name);
    void setAliases(const QStringList& aliases);

    bool isValid() const;

private:
    QString m_id;
    QString m_name;
    QStringList m_aliases;
};

} // namespace bookclub::common
