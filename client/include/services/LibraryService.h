#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

class LibraryService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int purchasedCount READ purchasedCount NOTIFY libraryChanged)
    Q_PROPERTY(int savedCount     READ savedCount     NOTIFY wishlistChanged)

public:
    explicit LibraryService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op

    int purchasedCount() const;
    int savedCount() const;

    Q_INVOKABLE QList<QObject*> purchasedBooks() const;
    Q_INVOKABLE QList<QObject*> downloadedBooks() const;
    Q_INVOKABLE bool isDownloaded(const QString& bookId) const;
    Q_INVOKABLE void toggleDownloaded(const QString& bookId);
    Q_INVOKABLE QList<QObject*> savedBooks() const;
    Q_INVOKABLE bool isSaved(const QString& bookId) const;
    Q_INVOKABLE void toggleSaved(const QString& bookId);
    // BUG FIX (Issue 33): idempotent "remove from wishlist" — always
    // sends RemoveBookFromShelf, so it's safe to call even if the book
    // is no longer in the wishlist (race, double-click).
    Q_INVOKABLE void removeFromWishlist(const QString& bookId);

    // Shelves
    Q_INVOKABLE QList<QObject*> shelves() const;
    Q_INVOKABLE QString createShelf(const QString& name, const QString& description, const QString& color = "#1A73E8", bool isPrivate = false);
    Q_INVOKABLE bool renameShelf(const QString& id, const QString& name);
    Q_INVOKABLE bool deleteShelf(const QString& id);
    Q_INVOKABLE QString duplicateShelf(const QString& id);
    Q_INVOKABLE bool setShelfColor(const QString& id, const QString& color);
    Q_INVOKABLE bool setShelfFavorite(const QString& id, bool favorite);
    Q_INVOKABLE bool setShelfPrivate(const QString& id, bool isPrivate);
    Q_INVOKABLE bool moveShelfUp(const QString& id);
    Q_INVOKABLE bool moveShelfDown(const QString& id);
    Q_INVOKABLE bool reorderShelves(const QStringList& orderedIds);
    Q_INVOKABLE bool addToShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE bool removeFromShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE bool moveBookBetweenShelves(const QString& fromShelfId, const QString& toShelfId, const QString& bookId);
    Q_INVOKABLE bool copyBookBetweenShelves(const QString& fromShelfId, const QString& toShelfId, const QString& bookId);
    Q_INVOKABLE QList<QObject*> booksInShelf(const QString& shelfId) const;
    Q_INVOKABLE QList<QObject*> searchShelves(const QString& query) const;
    Q_INVOKABLE QStringList shelfNames() const;
    Q_INVOKABLE QObject* shelfById(const QString& id) const;
    Q_INVOKABLE bool isFavoriteShelf(const QString& id) const;
    Q_INVOKABLE void refresh();

signals:
    void libraryChanged();
    void wishlistChanged();
    void shelvesChanged();

private:
    mutable QJsonObject m_libraryData;
    mutable QStringList m_downloaded;
    void refreshFromServer() const;
};

} // namespace bookclub::client
