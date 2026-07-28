// common/Interfaces/IAuthorRepository.cpp
//
// SQLite-backed IAuthorRepository. Full CRUD.
#include "common/Interfaces/IAuthorRepository.h"
#include "common/Models/Author.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

namespace {
Author* authorFromCurrentRecord(QSqlQuery& q)
{
    auto* a = new Author;
    a->setId(q.value("id").toString());
    a->setName(q.value("name").toString());
    a->setBiography(q.value("biography").toString());
    a->setCreatedAt(q.value("createdAt").toDateTime());
    return a;
}
} // namespace

class AuthorRepositoryImpl : public IAuthorRepository {
public:
    Author* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Authors WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return authorFromCurrentRecord(q);
    }

    Author* findByName(const QString& name) const override
    {
        auto q = DbConnection::run(
            "SELECT * FROM Authors WHERE name = ? COLLATE NOCASE", {name});
        if (!q.next()) return nullptr;
        return authorFromCurrentRecord(q);
    }

    QVector<Author*> findAll() const override
    {
        QVector<Author*> out;
        auto q = DbConnection::run("SELECT * FROM Authors ORDER BY name");
        while (q.next()) out.append(authorFromCurrentRecord(q));
        return out;
    }

    bool save(Author* author) override
    {
        if (!author) return false;
        if (author->id().isEmpty()) author->setId(IdGenerator::generateUuid());

        return DbConnection::execOk(
            "INSERT INTO Authors (id, name, biography, createdAt) "
            "VALUES (?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET name = excluded.name, biography = excluded.biography",
            {author->id(), author->name(), author->biography(),
             author->createdAt().isValid() ? author->createdAt()
                                           : QDateTime::currentDateTime()}
        );
    }

    bool remove(const QString& id) override
    {
        return DbConnection::execOk("DELETE FROM Authors WHERE id = ?", {id});
    }
};

IAuthorRepository* createAuthorRepository() {
    static AuthorRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
