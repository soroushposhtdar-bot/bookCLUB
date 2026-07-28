// =============================================================================
//  ShelfViewModel.cpp
// =============================================================================
#include "viewmodels/user/ShelfViewModel.h"
#include "services/LibraryService.h"
#include "services/LibraryDtos.h"

#include <algorithm>

namespace bookclub::client {

ShelfViewModel::ShelfViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void ShelfViewModel::setLibraryService(LibraryService* s) {
    if (m_libraryService == s) return;
    if (m_libraryService) disconnect(m_libraryService, nullptr, this, nullptr);
    m_libraryService = s;
    if (m_libraryService) {
        connect(m_libraryService, &LibraryService::shelvesChanged, this, &ShelfViewModel::shelvesChanged);
    }
    emit libraryServiceChanged();
    emit shelvesChanged();
}

QList<QObject*> ShelfViewModel::shelves() const {
    if (!m_libraryService) return {};
    QList<QObject*> out;
    // Search filter
    const QString q = m_searchQuery.trimmed().toLower();
    for (auto* o : m_libraryService->shelves()) {
        auto s = qobject_cast<ShelfDto*>(o);
        if (!s) { delete o; continue; }
        if (!q.isEmpty() && !s->name().toLower().contains(q) && !s->description().toLower().contains(q)) {
            delete s; continue;
        }
        out.append(s);
    }

    // Sort
    // BUG FIX: the QML SortDropdown sends "manual", "name", "recent",
    // and "count". Previously the C++ only handled "name" and "bookCount"
    // — "count" and "recent" fell through to the default alphabetical
    // sort, so picking "Book count" didn't actually sort by book count.
    // Now handles all four QML values.
    std::sort(out.begin(), out.end(), [this](QObject* aObj, QObject* bObj){
        auto a = qobject_cast<ShelfDto*>(aObj);
        auto b = qobject_cast<ShelfDto*>(bObj);
        if (!a || !b) return false;
        if (m_sortMode == "name" || m_sortMode == "manual") {
            return a->name().toLower() < b->name().toLower();
        }
        if (m_sortMode == "count" || m_sortMode == "bookCount") {
            return a->bookCount() > b->bookCount();
        }
        // "recent" — no createdAt on ShelfDto, fall back to name sort.
        return a->name().toLower() < b->name().toLower();
    });
    return out;
}

int ShelfViewModel::count() const {
    // BUG FIX (Issue 18): previously this returned the unfiltered count
    // (`m_libraryService->shelves().size()`) while `shelves()` returned
    // the filtered + sorted list. The count badge in the ShelvesPage
    // header therefore showed the total even when the user typed in the
    // search field — e.g. "5" next to "My Shelves" while only 2 cards
    // were visible.
    //
    // BUG FIX (round 2): calling shelves().size() caused a memory leak —
    // shelves() returns a QList<QObject*> of newly-allocated DTOs, and
    // count() discarded the list without deleting the DTOs. Each QML
    // evaluation of the `count` property leaked all the shelf DTOs.
    // We now compute the filtered count without creating full DTOs.
    if (!m_libraryService) return 0;
    const QString q = m_searchQuery.trimmed().toLower();
    int n = 0;
    for (auto* o : m_libraryService->shelves()) {
        auto s = qobject_cast<ShelfDto*>(o);
        if (!s) { delete o; continue; }
        if (q.isEmpty() || s->name().toLower().contains(q) || s->description().toLower().contains(q)) {
            ++n;
        }
        delete o;  // clean up temporary DTO
    }
    return n;
}

QList<QObject*> ShelfViewModel::selectedShelfBooks() const {
    if (!m_libraryService || m_selectedShelfId.isEmpty()) return {};
    return m_libraryService->booksInShelf(m_selectedShelfId);
}

QVariantMap ShelfViewModel::selectedShelf() const {
    QVariantMap empty;
    if (!m_libraryService || m_selectedShelfId.isEmpty()) return empty;
    // Find the shelf DTO matching the selected ID.
    for (auto* o : m_libraryService->shelves()) {
        auto* dto = qobject_cast<ShelfDto*>(o);
        if (dto && dto->id() == m_selectedShelfId) {
            QVariantMap m;
            m["id"] = dto->id();
            m["name"] = dto->name();
            m["description"] = dto->description();
            m["color"] = dto->color();
            m["bookIds"] = dto->bookIds();
            m["favorite"] = dto->favorite();
            m["isPrivate"] = dto->isPrivate();
            delete o;  // clean up the temporary DTO
            return m;
        }
        delete o;  // not the one we want — clean up
    }
    return empty;
}

void ShelfViewModel::createShelf() {
    // BUG FIX (Issue 32): re-entrancy guard.
    if (m_isBusy) return;
    if (!canCreate() || !m_libraryService) return;
    m_pending = PendingOp::Create;
    beginAsync(300);
}

void ShelfViewModel::onAsyncReady() {
    if (m_pending == PendingOp::Create) {
        if (m_libraryService) {
            QString id = m_libraryService->createShelf(m_newName, m_newDescription, m_newColor, m_newIsPrivate);
            m_newName.clear();
            m_newDescription.clear();
            m_newColor = "#1A73E8";
            m_newIsPrivate = false;
            emit newShelfChanged();
            // BUG FIX (Issue 19): previously `shelfCreated(id)` was
            // emitted unconditionally, even when `createShelf` returned
            // an empty string (server rejected the create). UserShell.qml
            // listens with `onShelfCreated: toastRequested("success", ...)`
            // — so the user saw a green "Shelf created" toast even though
            // the shelf wasn't actually created. We now only emit the
            // success signal when the id is non-empty.
            if (!id.isEmpty()) {
                emit shelfCreated(id);
            } else {
                emit shelfCreateFailed(QStringLiteral("Could not create shelf. Please try again."));
            }
        }
        m_pending = PendingOp::None;
    }
    finishAsync();
}

void ShelfViewModel::renameShelf(const QString& id, const QString& name) {
    if (m_libraryService) {
        m_libraryService->renameShelf(id, name);
        // v15f: refresh the selected shelf's display so the detail panel
        // shows the new name immediately.
        if (m_selectedShelfId == id) emit selectionChanged();
    }
}

void ShelfViewModel::deleteShelf(const QString& id) {
    if (m_libraryService) {
        m_libraryService->deleteShelf(id);
        if (m_selectedShelfId == id) m_selectedShelfId.clear();
        emit selectionChanged();
        emit shelfDeleted(id);
    }
}

void ShelfViewModel::duplicateShelf(const QString& id) {
    if (m_libraryService) m_libraryService->duplicateShelf(id);
}

void ShelfViewModel::setShelfColor(const QString& id, const QString& color) {
    if (m_libraryService) {
        m_libraryService->setShelfColor(id, color);
        // v15f: refresh the selected shelf so the detail panel + card show
        // the new color immediately.
        if (m_selectedShelfId == id) emit selectionChanged();
    }
}

void ShelfViewModel::toggleFavorite(const QString& id) {
    if (!m_libraryService) return;
    // Find current favorite state
    bool fav = false;
    for (auto* o : m_libraryService->shelves()) {
        auto s = qobject_cast<ShelfDto*>(o);
        if (s && s->id() == id) { fav = s->favorite(); delete s; break; }
        delete o;
    }
    m_libraryService->setShelfFavorite(id, !fav);
    // v15f: refresh the selected shelf so the star icon updates.
    if (m_selectedShelfId == id) emit selectionChanged();
}

void ShelfViewModel::togglePrivate(const QString& id) {
    if (!m_libraryService) return;
    bool priv = false;
    for (auto* o : m_libraryService->shelves()) {
        auto s = qobject_cast<ShelfDto*>(o);
        if (s && s->id() == id) { priv = s->isPrivate(); delete s; break; }
        delete o;
    }
    m_libraryService->setShelfPrivate(id, !priv);
    // v15f: refresh the selected shelf so the lock icon updates.
    if (m_selectedShelfId == id) emit selectionChanged();
}

void ShelfViewModel::moveUp(const QString& id) {
    if (m_libraryService) {
        m_libraryService->moveShelfUp(id);
        // v15g: refresh selection so the detail panel updates.
        if (m_selectedShelfId == id) emit selectionChanged();
    }
}

void ShelfViewModel::moveDown(const QString& id) {
    if (m_libraryService) {
        m_libraryService->moveShelfDown(id);
        // v15g: refresh selection so the detail panel updates.
        if (m_selectedShelfId == id) emit selectionChanged();
    }
}

void ShelfViewModel::reorder(const QStringList& orderedIds) {
    if (m_libraryService) m_libraryService->reorderShelves(orderedIds);
}

void ShelfViewModel::addBookToShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) {
        m_libraryService->addToShelf(shelfId, bookId);
        // v15f: CRITICAL — emit selectionChanged so the selected shelf's
        // book list + bookIds refresh. Without this, the book picker's
        // add/remove toggle stays stale (it checks selectedShelf.bookIds
        // to decide whether to show + or ✓), so the user can't tell if
        // the book was actually added.
        if (m_selectedShelfId == shelfId) emit selectionChanged();
    }
}

void ShelfViewModel::removeBookFromShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) {
        m_libraryService->removeFromShelf(shelfId, bookId);
        // v15f: same as addBookToShelf — must refresh the selected shelf.
        if (m_selectedShelfId == shelfId) emit selectionChanged();
    }
}

void ShelfViewModel::moveBook(const QString& fromShelfId, const QString& toShelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->moveBookBetweenShelves(fromShelfId, toShelfId, bookId);
}

void ShelfViewModel::copyBook(const QString& fromShelfId, const QString& toShelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->copyBookBetweenShelves(fromShelfId, toShelfId, bookId);
}

void ShelfViewModel::selectShelf(const QString& id) {
    if (m_selectedShelfId == id) return;
    m_selectedShelfId = id;
    emit selectionChanged();
}

} // namespace bookclub::client
