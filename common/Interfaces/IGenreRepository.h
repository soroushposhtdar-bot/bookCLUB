// common/Interfaces/IGenreRepository.h
//
// Repository interface for Genre entities (CRUD + lookup).
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace bookclub::common {

class Genre;

class IGenreRepository {
public:
    virtual ~IGenreRepository() = default;

    virtual Genre findById(const QString& id) const = 0;
    virtual Genre findByName(const QString& name) const = 0;
    virtual QVector<Genre> findAll() const = 0;
    virtual QStringList allNames() const = 0;
    virtual bool save(const Genre& genre) = 0;     // INSERT or UPDATE
    virtual bool remove(const QString& id) = 0;
    virtual bool attachToBook(const QString& bookId, const QString& genreId) = 0;
    virtual bool detachFromBook(const QString& bookId, const QString& genreId) = 0;
    virtual QStringList genresOfBook(const QString& bookId) const = 0;
};

IGenreRepository* createGenreRepository();

} // namespace bookclub::common
