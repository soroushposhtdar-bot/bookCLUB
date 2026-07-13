#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>

namespace bookclub::server {

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override = default;

    bool open();
    void close();
    bool isOpen() const;
    QSqlDatabase database() const;

    bool migrate();
    bool initializeSchema();
    bool beginTransaction();
    bool commit();
    bool rollback();

signals:
    void databaseError(const QString& message);

private:
    QSqlDatabase m_database;
};

} // namespace bookclub::server
