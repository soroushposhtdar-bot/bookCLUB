// =============================================================================
//  LibraryViewModel.cpp
// =============================================================================
#include "viewmodels/user/LibraryViewModel.h"
#include "services/LibraryService.h"

namespace bookclub::client {

LibraryViewModel::LibraryViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void LibraryViewModel::setLibraryService(LibraryService* s) {
    if (m_libraryService == s) return;
    if (m_libraryService) disconnect(m_libraryService, nullptr, this, nullptr);
    m_libraryService = s;
    if (m_libraryService) {
        connect(m_libraryService, &LibraryService::libraryChanged,  this, [this]() {
            emit myBooksChanged();
            emit downloadedBooksChanged();
        });
        connect(m_libraryService, &LibraryService::wishlistChanged, this, &LibraryViewModel::savedBooksChanged);
        connect(m_libraryService, &LibraryService::shelvesChanged,  this, &LibraryViewModel::shelvesChanged);
    }
    emit libraryServiceChanged();
    emit myBooksChanged();
    emit downloadedBooksChanged();
    emit savedBooksChanged();
    emit shelvesChanged();
}

QList<QObject*> LibraryViewModel::myBooks() const {
    return m_libraryService ? m_libraryService->purchasedBooks() : QList<QObject*>{};
}
int LibraryViewModel::myBooksCount() const {
    return m_libraryService ? m_libraryService->purchasedCount() : 0;
}
QList<QObject*> LibraryViewModel::downloadedBooks() const {
    return m_libraryService ? m_libraryService->downloadedBooks() : QList<QObject*>{};
}
int LibraryViewModel::downloadedCount() const {
    return m_libraryService ? m_libraryService->downloadedBooks().size() : 0;
}
QList<QObject*> LibraryViewModel::savedBooks() const {
    return m_libraryService ? m_libraryService->savedBooks() : QList<QObject*>{};
}
int LibraryViewModel::savedCount() const {
    return m_libraryService ? m_libraryService->savedCount() : 0;
}
QList<QObject*> LibraryViewModel::shelves() const {
    return m_libraryService ? m_libraryService->shelves() : QList<QObject*>{};
}

void LibraryViewModel::createShelf() {
    if (!canCreateShelf() || !m_libraryService) return;
    m_pending = PendingOp::CreateShelf;
    beginAsync(350);
}

void LibraryViewModel::onAsyncReady() {
    if (m_pending == PendingOp::CreateShelf) {
        if (m_libraryService) {
            QString id = m_libraryService->createShelf(m_newShelfName, m_newShelfDescription);
            m_newShelfName.clear();
            m_newShelfDescription.clear();
            emit newShelfChanged();
            emit shelfCreated(id);
        }
        m_pending = PendingOp::None;
    }
    finishAsync();
}

void LibraryViewModel::renameShelf(const QString& id, const QString& name) {
    if (m_libraryService) m_libraryService->renameShelf(id, name);
}

void LibraryViewModel::deleteShelf(const QString& id) {
    if (m_libraryService) m_libraryService->deleteShelf(id);
}

void LibraryViewModel::addToShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->addToShelf(shelfId, bookId);
}

void LibraryViewModel::removeFromShelf(const QString& shelfId, const QString& bookId) {
    if (m_libraryService) m_libraryService->removeFromShelf(shelfId, bookId);
}

QList<QObject*> LibraryViewModel::booksInShelf(const QString& shelfId) const {
    return m_libraryService ? m_libraryService->booksInShelf(shelfId) : QList<QObject*>{};
}

void LibraryViewModel::toggleSaved(const QString& bookId) {
    if (m_libraryService) m_libraryService->toggleSaved(bookId);
}

void LibraryViewModel::toggleDownloaded(const QString& bookId) {
    if (m_libraryService) m_libraryService->toggleDownloaded(bookId);
    emit downloadedBooksChanged();
}

bool LibraryViewModel::isDownloaded(const QString& bookId) const {
    return m_libraryService && m_libraryService->isDownloaded(bookId);
}

} // namespace bookclub::client
