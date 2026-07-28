// common/Interfaces/IAuthorRepository.h
//
// Repository interface for Author entities (CRUD + lookup).
#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class Author;

class IAuthorRepository {
public:
    virtual ~IAuthorRepository() = default;

    virtual Author* findById(const QString& id) const = 0;
    virtual Author* findByName(const QString& name) const = 0;
    virtual QVector<Author*> findAll() const = 0;
    virtual bool save(Author* author) = 0;     // INSERT or UPDATE
    virtual bool remove(const QString& id) = 0;
};

IAuthorRepository* createAuthorRepository();

} // namespace bookclub::common
