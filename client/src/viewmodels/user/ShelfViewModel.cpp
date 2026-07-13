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
    std::sort(out.begin(), out.end(), [this](QObject* aObj, QObject* bObj){
        auto a = qobject_cast<ShelfDto*>(aObj);
        auto b = qobject_cast<ShelfDto*>(bObj);
        if (!a || !b) return false;
        if (m_sortMode == "name")      return a->name().toLower() < b->name().toLower();
        if (m_sortMode == "recent")   return a->createdAtText() > b->createdAtText();
        if (m_sortMode == "bookCount") return a->bookCount() > b->bookCount();
        return a->order() < b->order();   // "order" — manual order
    });
    return out;
}

int ShelfViewModel::count() const {
    return m_libraryService ? m_libraryService->shelves().size() : 0;
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
            emit shelfCreated(id);
        }
        m_pending = PendingOp::None;
    }
    finishAsync();
}

void ShelfViewModel::renameShelf(const QString& id, const QString& name) {
    if (m_libraryService) m_libraryService->renameShelf(id, name);
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
    if (m_libraryService) m_libraryService->setShelfColor(id, color);
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
}

void ShelfViewModel::moveUp(const QString& id) {
    if (m_libraryService) m_libraryService->moveShelfUp(id);
}

void ShelfViewModel::moveDown(const QString& id) {
    if (m_libraryService) m_libraryService->moveShelfDown(id);
}

void ShelfViewModel::reorder(const QStringList& orderedIds) {
    if (m_libraryService) m_libraryService->reorderShelves(orderedIds);
}

void ShelfViewModel::addBookToShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->addToShelf(shelfId, bookId);
}

void ShelfViewModel::removeBookFromShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->removeFromShelf(shelfId, bookId);
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
