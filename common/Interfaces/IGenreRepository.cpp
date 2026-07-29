// common/Interfaces/IGenreRepository.cpp
#include "common/Interfaces/IGenreRepository.h"
#include "common/Models/Genre.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>

namespace bookclub::common {

namespace {
Genre genreFromCurrentRecord(QSqlQuery& q)
{
    Genre g;
    g.setId(q.value("id").toString());
    g.setName(q.value("name").toString());
    const QString aliasesJson = q.value("aliases").toString();
    if (!aliasesJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(aliasesJson.toUtf8());
        if (doc.isArray()) {
            QStringList aliases;
            for (const auto& v : doc.array()) aliases.append(v.toString());
            g.setAliases(aliases);
        }
    }
    return g;
}
} // namespace

class GenreRepositoryImpl : public IGenreRepository {
public:
    Genre findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Genres WHERE id = ?", {id});
        return q.next() ? genreFromCurrentRecord(q) : Genre();
    }

    Genre findByName(const QString& name) const override
    {
        auto q = DbConnection::run(
            "SELECT * FROM Genres WHERE name = ? COLLATE NOCASE", {name});
        return q.next() ? genreFromCurrentRecord(q) : Genre();
    }

    QVector<Genre> findAll() const override
    {
        QVector<Genre> out;
        auto q = DbConnection::run("SELECT * FROM Genres ORDER BY name");
        while (q.next()) out.append(genreFromCurrentRecord(q));
        return out;
    }

    QStringList allNames() const override
    {
        QStringList names;
        auto q = DbConnection::run("SELECT name FROM Genres ORDER BY name");
        while (q.next()) names.append(q.value(0).toString());
        return names;
    }

    bool save(const Genre& genre) override
    {
        QString id = genre.id();
        if (id.isEmpty()) {
            id = IdGenerator::generateUuid();
        }
        const QString aliasesJson = QString::fromUtf8(
            QJsonDocument(QJsonArray::fromStringList(genre.aliases()))
                .toJson(QJsonDocument::Compact)
        );
        return DbConnection::execOk(
            "INSERT INTO Genres (id, name, aliases) VALUES (?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET name = excluded.name, aliases = excluded.aliases",
            {id, genre.name(), aliasesJson}
        );
    }

    bool remove(const QString& id) override
    {
        return DbConnection::execOk("DELETE FROM Genres WHERE id = ?", {id});
    }

    bool attachToBook(const QString& bookId, const QString& genreId) override
    {
        return DbConnection::execOk(
            "INSERT OR IGNORE INTO BookGenres (bookId, genreId) VALUES (?, ?)",
            {bookId, genreId}
        );
    }

    bool detachFromBook(const QString& bookId, const QString& genreId) override
    {
        return DbConnection::execOk(
            "DELETE FROM BookGenres WHERE bookId = ? AND genreId = ?",
            {bookId, genreId}
        );
    }

    QStringList genresOfBook(const QString& bookId) const override
    {
        QStringList ids;
        auto q = DbConnection::run(
            "SELECT g.id FROM Genres g "
            "JOIN BookGenres bg ON bg.genreId = g.id "
            "WHERE bg.bookId = ?",
            {bookId}
        );
        while (q.next()) ids.append(q.value(0).toString());
        return ids;
    }
};

IGenreRepository* createGenreRepository() {
    static GenreRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
