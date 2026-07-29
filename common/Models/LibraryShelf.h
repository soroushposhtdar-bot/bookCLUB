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
    // v15e: shelf metadata that was missing from the model.
    const QString& color() const;
    bool favorite() const;
    bool isPrivate() const;
    int sortOrder() const;

    void setId(const QString& id);
    void setUserId(const QString& userId);
    void setName(const QString& name);
    void setDescription(const QString& description);
    void setBookIds(const QStringList& ids);
    void addBook(const QString& bookId);
    void removeBook(const QString& bookId);
    void moveBookTo(int from, int to);
    void setSystemShelf(bool systemShelf);
    // v15e: setters for shelf metadata.
    void setColor(const QString& color);
    void setFavorite(bool favorite);
    void setIsPrivate(bool isPrivate);
    void setSortOrder(int order);

signals:
    void shelfChanged();

private:
    QString m_id;
    QString m_userId;
    QString m_name;
    QString m_description;
    QStringList m_bookIds;
    bool m_systemShelf = false;
    // v15e: persisted shelf metadata (matches the Shelves table columns).
    QString m_color = "#1A73E8";
    bool m_favorite = false;
    bool m_isPrivate = false;
    int m_sortOrder = 0;
};

} // namespace bookclub::common
