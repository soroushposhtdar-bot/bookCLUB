#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace bookclub::common {

class LibraryShelf : public QObject {
    Q_OBJECT
public:
    explicit LibraryShelf(QObject* parent = nullptr);
    LibraryShelf(const QString& id, QObject* parent = nullptr);
    ~LibraryShelf() override = default;

    const QString& id() const;
    const QString& userId() const;
    const QString& name() const;
    const QString& description() const;
    const QStringList& bookIds() const;
    bool isSystemShelf() const;

    void setId(const QString& id);
    void setUserId(const QString& userId);
    void setName(const QString& name);
    void setDescription(const QString& description);
    void setBookIds(const QStringList& ids);
    void addBook(const QString& bookId);
    void removeBook(const QString& bookId);
    void moveBookTo(int from, int to);
    void setSystemShelf(bool systemShelf);

signals:
    void shelfChanged();

private:
    QString m_id;
    QString m_userId;
    QString m_name;
    QString m_description;
    QStringList m_bookIds;
    bool m_systemShelf = false;
};

} // namespace bookclub::common
