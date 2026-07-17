#include "common/Models/UserLibrary.h"
#include "common/Utils/IdGenerator.h"
#include <algorithm>

namespace bookclub::common {

UserLibrary::UserLibrary(QObject* parent) : QObject(parent) {}

UserLibrary::~UserLibrary() {
    qDeleteAll(m_shelves);
}

const QString& UserLibrary::userId() const { return m_userId; }
const QStringList& UserLibrary::purchasedBookIds() const { return m_purchasedBookIds; }
const QStringList& UserLibrary::savedBookIds() const { return m_savedBookIds; }
const QVector<LibraryShelf*>& UserLibrary::shelves() const { return m_shelves; }
const QString& UserLibrary::lastOpenedBookId() const { return m_lastOpenedBookId; }
const QString& UserLibrary::lastOpenedPage() const { return m_lastOpenedPage; }

void UserLibrary::setUserId(const QString& userId) { m_userId = userId; }

void UserLibrary::setPurchasedBookIds(const QStringList& ids) {
    if (m_purchasedBookIds != ids) {
        m_purchasedBookIds = ids;
        emit libraryChanged();
    }
}

void UserLibrary::setSavedBookIds(const QStringList& ids) {
    if (m_savedBookIds != ids) {
        m_savedBookIds = ids;
        emit libraryChanged();
    }
}

void UserLibrary::setShelves(const QVector<LibraryShelf*>& shelves) {
    qDeleteAll(m_shelves);
    m_shelves = shelves;
    emit libraryChanged();
}

void UserLibrary::setLastOpenedBookId(const QString& bookId) {
    if (m_lastOpenedBookId != bookId) {
        m_lastOpenedBookId = bookId;
        emit libraryChanged();
    }
}

void UserLibrary::setLastOpenedPage(const QString& page) {
    if (m_lastOpenedPage != page) {
        m_lastOpenedPage = page;
        emit libraryChanged();
    }
}

void UserLibrary::addPurchasedBook(const QString& bookId) {
    if (!m_purchasedBookIds.contains(bookId)) {
        m_purchasedBookIds.append(bookId);
        emit libraryChanged();
    }
}

void UserLibrary::addSavedBook(const QString& bookId) {
    if (!m_savedBookIds.contains(bookId)) {
        m_savedBookIds.append(bookId);
        emit libraryChanged();
    }
}

void UserLibrary::removeSavedBook(const QString& bookId) {
    if (m_savedBookIds.removeOne(bookId)) {
        emit libraryChanged();
    }
}

LibraryShelf* UserLibrary::createShelf(const QString& name, const QString& description) {
    auto* shelf = new LibraryShelf(this);
    shelf->setId(IdGenerator::generateUuid());
    shelf->setUserId(m_userId);
    shelf->setName(name);
    shelf->setDescription(description);
    shelf->setSystemShelf(false);
    m_shelves.append(shelf);
    emit libraryChanged();
    return shelf;
}

bool UserLibrary::removeShelf(const QString& shelfId) {
    for (int i = 0; i < m_shelves.size(); ++i) {
        if (m_shelves[i]->id() == shelfId) {
            if (m_shelves[i]->isSystemShelf()) return false;
            delete m_shelves[i];
            m_shelves.removeAt(i);
            emit libraryChanged();
            return true;
        }
    }
    return false;
}

} // namespace bookclub::common
