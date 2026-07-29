// common/Interfaces/IShelfRepository.cpp
#include "common/Interfaces/IShelfRepository.h"
#include "common/Models/LibraryShelf.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

namespace {
LibraryShelf* shelfFromCurrentRecord(QSqlQuery& q)
{
    auto* s = new LibraryShelf;
    s->setId(q.value("id").toString());
    s->setUserId(q.value("userId").toString());
    s->setName(q.value("name").toString());
    s->setDescription(q.value("description").toString());
    s->setSystemShelf(q.value("isSystemShelf").toInt() == 1);
    // v15e: load the shelf metadata that was previously discarded.
    s->setColor(q.value("color").toString());
    s->setFavorite(q.value("isFavorite").toInt() == 1);
    s->setIsPrivate(q.value("isPrivate").toInt() == 1);
    s->setSortOrder(q.value("sortOrder").toInt());
    return s;
}
} // namespace

class ShelfRepositoryImpl : public IShelfRepository {
public:
    LibraryShelf* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Shelves WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        auto* s = shelfFromCurrentRecord(q);
        s->setBookIds(bookIdsOf(id));
        return s;
    }

    QVector<LibraryShelf*> findByUser(const QString& userId) const override
    {
        QVector<LibraryShelf*> out;
        auto q = DbConnection::run(
            "SELECT * FROM Shelves WHERE userId = ? ORDER BY sortOrder, name",
            {userId}
        );
        while (q.next()) {
            auto* s = shelfFromCurrentRecord(q);
            s->setBookIds(bookIdsOf(s->id()));
            out.append(s);
        }
        return out;
    }

    bool save(LibraryShelf* shelf) override
    {
        if (!shelf) return false;
        if (shelf->id().isEmpty()) shelf->setId(IdGenerator::generateUuid());
        if (shelf->userId().isEmpty()) {
            LOG_ERROR("ShelfRepository::save called without userId");
            return false;
        }

        // v15e: persist color, isFavorite, isPrivate alongside name + desc.
        // The ON CONFLICT branch now updates ALL mutable columns so
        // setShelfColor / setShelfFavorite / setShelfPrivate actually
        // persist to the database.
        return DbConnection::execOk(
            "INSERT INTO Shelves (id, userId, name, description, color, "
            "  isFavorite, isPrivate, isSystemShelf, sortOrder) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET "
            "  name = excluded.name, "
            "  description = excluded.description, "
            "  color = excluded.color, "
            "  isFavorite = excluded.isFavorite, "
            "  isPrivate = excluded.isPrivate, "
            "  sortOrder = excluded.sortOrder",
            {shelf->id(), shelf->userId(), shelf->name(),
             shelf->description(), shelf->color(),
             shelf->favorite() ? 1 : 0,
             shelf->isPrivate() ? 1 : 0,
             shelf->isSystemShelf() ? 1 : 0,
             shelf->sortOrder()}
        );
    }

    bool remove(const QString& id) override
    {
        return DbConnection::execOk("DELETE FROM Shelves WHERE id = ?", {id});
    }

    bool addBook(const QString& shelfId, const QString& bookId) override
    {
        return DbConnection::execOk(
            "INSERT OR IGNORE INTO ShelfBooks (shelfId, bookId, addedAt) VALUES (?, ?, ?)",
            {shelfId, bookId, QDateTime::currentDateTime()}
        );
    }

    bool removeBook(const QString& shelfId, const QString& bookId) override
    {
        return DbConnection::execOk(
            "DELETE FROM ShelfBooks WHERE shelfId = ? AND bookId = ?",
            {shelfId, bookId}
        );
    }

    QStringList bookIdsOf(const QString& shelfId) const override
    {
        QStringList ids;
        auto q = DbConnection::run(
            "SELECT bookId FROM ShelfBooks WHERE shelfId = ? ORDER BY addedAt",
            {shelfId}
        );
        while (q.next()) ids.append(q.value(0).toString());
        return ids;
    }
};

IShelfRepository* createShelfRepository() {
    static ShelfRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
