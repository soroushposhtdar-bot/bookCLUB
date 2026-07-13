// =============================================================================
//  LibraryService.cpp
// =============================================================================
#include "services/LibraryService.h"
#include "services/BookDto.h"
#include "services/LibraryDtos.h"

namespace bookclub::client {

LibraryService::LibraryService(QObject* parent)
    : QObject(parent)
{}

void LibraryService::setDataStore(MockDataStore* store) {
    if (m_store == store) return;
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    m_store = store;
    if (m_store) {
        connect(m_store, &MockDataStore::libraryChanged,  this, &LibraryService::libraryChanged);
        connect(m_store, &MockDataStore::wishlistChanged, this, &LibraryService::wishlistChanged);
        connect(m_store, &MockDataStore::shelvesChanged,  this, &LibraryService::shelvesChanged);
    }
    emit libraryChanged();
    emit wishlistChanged();
    emit shelvesChanged();
}

int LibraryService::purchasedCount() const {
    return m_store ? m_store->purchasedBookIds().size() : 0;
}

int LibraryService::savedCount() const {
    return m_store ? m_store->wishlist().size() : 0;
}

QList<QObject*> LibraryService::purchasedBooks() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& id : m_store->purchasedBookIds()) {
        auto b = m_store->bookById(id);
        if (b.id.isEmpty()) continue;
        auto dto = new BookDto(b);
        dto->setPurchased(true);
        out.append(dto);
    }
    return out;
}

QList<QObject*> LibraryService::savedBooks() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& id : m_store->wishlist()) {
        auto b = m_store->bookById(id);
        if (b.id.isEmpty()) continue;
        auto dto = new BookDto(b);
        dto->setInWishlist(true);
        out.append(dto);
    }
    return out;
}

bool LibraryService::isSaved(const QString& bookId) const {
    return m_store && m_store->wishlist().contains(bookId);
}

void LibraryService::toggleSaved(const QString& bookId) {
    if (!m_store) return;
    m_store->toggleWishlist(bookId);
}

QList<QObject*> LibraryService::shelves() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& s : m_store->shelves()) out.append(new ShelfDto(s));
    return out;
}

QString LibraryService::createShelf(const QString& name, const QString& description, const QString& color, bool isPrivate) {
    if (!m_store || name.trimmed().isEmpty()) return {};
    return m_store->addShelf(name.trimmed(), description.trimmed(), color, isPrivate);
}

bool LibraryService::renameShelf(const QString& id, const QString& name) {
    if (!m_store || name.trimmed().isEmpty()) return false;
    m_store->renameShelf(id, name.trimmed());
    return true;
}

bool LibraryService::deleteShelf(const QString& id) {
    if (!m_store) return false;
    m_store->deleteShelf(id);
    return true;
}

QString LibraryService::duplicateShelf(const QString& id) {
    if (!m_store) return {};
    return m_store->duplicateShelf(id);
}

bool LibraryService::setShelfColor(const QString& id, const QString& color) {
    if (!m_store) return false;
    return m_store->setShelfColor(id, color);
}

bool LibraryService::setShelfFavorite(const QString& id, bool favorite) {
    if (!m_store) return false;
    return m_store->setShelfFavorite(id, favorite);
}

bool LibraryService::setShelfPrivate(const QString& id, bool isPrivate) {
    if (!m_store) return false;
    return m_store->setShelfPrivate(id, isPrivate);
}

bool LibraryService::moveShelfUp(const QString& id) {
    if (!m_store) return false;
    return m_store->moveShelf(id, -1);
}

bool LibraryService::moveShelfDown(const QString& id) {
    if (!m_store) return false;
    return m_store->moveShelf(id, +1);
}

bool LibraryService::reorderShelves(const QStringList& orderedIds) {
    if (!m_store) return false;
    return m_store->reorderShelves(orderedIds);
}

bool LibraryService::addToShelf(const QString& shelfId, const QString& bookId) {
    if (!m_store) return false;
    m_store->addBookToShelf(shelfId, bookId);
    return true;
}

bool LibraryService::removeFromShelf(const QString& shelfId, const QString& bookId) {
    if (!m_store) return false;
    m_store->removeBookFromShelf(shelfId, bookId);
    return true;
}

bool LibraryService::moveBookBetweenShelves(const QString& fromShelfId, const QString& toShelfId, const QString& bookId) {
    if (!m_store) return false;
    return m_store->moveBookBetweenShelves(fromShelfId, toShelfId, bookId);
}

bool LibraryService::copyBookBetweenShelves(const QString& fromShelfId, const QString& toShelfId, const QString& bookId) {
    if (!m_store) return false;
    return m_store->copyBookBetweenShelves(fromShelfId, toShelfId, bookId);
}

QList<QObject*> LibraryService::searchShelves(const QString& query) const {
    QList<QObject*> out;
    if (!m_store) return out;
    const QString q = query.trimmed().toLower();
    for (const auto& s : m_store->shelves()) {
        if (q.isEmpty() || s.name.toLower().contains(q) || s.description.toLower().contains(q)) {
            out.append(new ShelfDto(s));
        }
    }
    return out;
}

QList<QObject*> LibraryService::booksInShelf(const QString& shelfId) const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& s : m_store->shelves()) {
        if (s.id == shelfId) {
            for (const auto& id : s.bookIds) {
                auto b = m_store->bookById(id);
                if (b.id.isEmpty()) continue;
                out.append(new BookDto(b));
            }
            break;
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
//  Downloaded books (offline tracking)
// -----------------------------------------------------------------------------

QList<QObject*> LibraryService::downloadedBooks() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& b : m_store->downloadedBooks()) {
        out.append(new BookDto(b));
    }
    return out;
}

bool LibraryService::isDownloaded(const QString& bookId) const {
    return m_store && m_store->isDownloaded(bookId);
}

void LibraryService::toggleDownloaded(const QString& bookId) {
    if (!m_store) return;
    if (m_store->isDownloaded(bookId)) {
        m_store->removeDownloaded(bookId);
    } else {
        m_store->markDownloaded(bookId);
    }
    emit libraryChanged();
}

} // namespace bookclub::client
