// =============================================================================
//  LibraryService.h
// =============================================================================
//  Mocked personal-library service for the Regular User role.
//
//  Covers all three Library tabs:
//      • My Books     — purchased books (open in PDF reader)
//      • Saved        — wishlist
//      • My Shelves   — custom shelves (create / rename / delete / add book /
//                       remove book / move book between shelves)
//
//  Real-backend mapping (see common/Network/Protocol.h):
//      purchasedBooks()         → REQ_LIB_PURCHASED   → RES_BOOK_LIST
//      savedBooks()             → REQ_LIB_SAVED       → RES_BOOK_LIST
//      shelves()                → REQ_SHELF_LIST      → RES_SHELF_LIST
//      createShelf(name,desc)   → REQ_SHELF_CREATE    → RES_SHELF
//      renameShelf(id,name)     → REQ_SHELF_RENAME    → RES_SHELF
//      deleteShelf(id)          → REQ_SHELF_DELETE    → RES_OK
//      addToShelf(shelfId,bookId) → REQ_SHELF_ADD     → RES_SHELF
//      removeFromShelf(...)     → REQ_SHELF_REMOVE    → RES_SHELF
//
//  All state lives in MockDataStore so the cart checkout flow can mark books
//  as purchased and have them immediately appear in "My Books".
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>

#include "services/MockDataStore.h"

namespace bookclub::client {

class LibraryService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int purchasedCount READ purchasedCount NOTIFY libraryChanged)
    Q_PROPERTY(int savedCount     READ savedCount     NOTIFY wishlistChanged)

public:
    explicit LibraryService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    int purchasedCount() const;
    int savedCount() const;

    // ----- My Books -----
    Q_INVOKABLE QList<QObject*> purchasedBooks() const;

    // ----- Downloaded Books (offline) -----
    Q_INVOKABLE QList<QObject*> downloadedBooks() const;
    Q_INVOKABLE bool isDownloaded(const QString& bookId) const;
    Q_INVOKABLE void toggleDownloaded(const QString& bookId);

    // ----- Saved / Wishlist -----
    Q_INVOKABLE QList<QObject*> savedBooks() const;
    Q_INVOKABLE bool isSaved(const QString& bookId) const;
    Q_INVOKABLE void toggleSaved(const QString& bookId);

    // ----- Shelves -----
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

signals:
    void libraryChanged();
    void wishlistChanged();
    void shelvesChanged();

private:
    MockDataStore* m_store = nullptr;
};

} // namespace bookclub::client
