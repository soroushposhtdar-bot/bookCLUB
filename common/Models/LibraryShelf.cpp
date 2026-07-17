#include "common/Models/LibraryShelf.h"
#include "common/Utils/IdGenerator.h"

namespace bookclub::common {

LibraryShelf::LibraryShelf(QObject* parent) : QObject(parent) {
    m_id = IdGenerator::generateUuid();
}

LibraryShelf::LibraryShelf(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}

const QString& LibraryShelf::id() const { return m_id; }
const QString& LibraryShelf::userId() const { return m_userId; }
const QString& LibraryShelf::name() const { return m_name; }
const QString& LibraryShelf::description() const { return m_description; }
const QStringList& LibraryShelf::bookIds() const { return m_bookIds; }
bool LibraryShelf::isSystemShelf() const { return m_systemShelf; }

void LibraryShelf::setId(const QString& id) { m_id = id; }
void LibraryShelf::setUserId(const QString& userId) { m_userId = userId; }

void LibraryShelf::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit shelfChanged();
    }
}

void LibraryShelf::setDescription(const QString& description) {
    if (m_description != description) {
        m_description = description;
        emit shelfChanged();
    }
}

void LibraryShelf::setBookIds(const QStringList& ids) {
    if (m_bookIds != ids) {
        m_bookIds = ids;
        emit shelfChanged();
    }
}

void LibraryShelf::addBook(const QString& bookId) {
    if (!m_bookIds.contains(bookId)) {
        m_bookIds.append(bookId);
        emit shelfChanged();
    }
}

void LibraryShelf::removeBook(const QString& bookId) {
    if (m_bookIds.removeOne(bookId)) {
        emit shelfChanged();
    }
}

void LibraryShelf::moveBookTo(int from, int to) {
    if (from < 0 || from >= m_bookIds.size() || to < 0 || to >= m_bookIds.size()) return;
    if (from == to) return;
    m_bookIds.move(from, to);
    emit shelfChanged();
}

void LibraryShelf::setSystemShelf(bool systemShelf) {
    if (m_systemShelf != systemShelf) {
        m_systemShelf = systemShelf;
        emit shelfChanged();
    }
}

} // namespace bookclub::common
