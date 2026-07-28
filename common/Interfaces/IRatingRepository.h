// common/Interfaces/IRatingRepository.h
//
// Repository for the lightweight Ratings table (separate from Reviews).
#pragma once

#include <QString>

namespace bookclub::common {

class IRatingRepository {
public:
    virtual ~IRatingRepository() = default;

    virtual int  ratingOfUser(const QString& bookId, const QString& userId) const = 0;
    virtual bool setRating(const QString& bookId, const QString& userId, int stars) = 0;
    virtual bool removeRating(const QString& bookId, const QString& userId) = 0;
    virtual double averageRating(const QString& bookId) const = 0;
    virtual int  count(const QString& bookId) const = 0;
};

IRatingRepository* createRatingRepository();

} // namespace bookclub::common
