// common/Interfaces/IShelfRepository.h
//
// Repository for user library shelves (CRUD + book membership).
#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class LibraryShelf;

class IShelfRepository {
public:
    virtual ~IShelfRepository() = default;

    virtual LibraryShelf* findById(const QString& id) const = 0;
    virtual QVector<LibraryShelf*> findByUser(const QString& userId) const = 0;
    virtual bool save(LibraryShelf* shelf) = 0;          // INSERT or UPDATE
    virtual bool remove(const QString& id) = 0;
    virtual bool addBook(const QString& shelfId, const QString& bookId) = 0;
    virtual bool removeBook(const QString& shelfId, const QString& bookId) = 0;
    virtual QStringList bookIdsOf(const QString& shelfId) const = 0;
};

IShelfRepository* createShelfRepository();

} // namespace bookclub::common
