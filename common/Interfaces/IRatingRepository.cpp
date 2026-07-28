// common/Interfaces/IRatingRepository.cpp
#include "common/Interfaces/IRatingRepository.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

namespace {
// Recalculate and persist the book's averageRating + ratingCount,
// combining Reviews.stars and Ratings.stars.
void recalcBookRating(const QString& bookId)
{
    auto q = DbConnection::run(
        "SELECT AVG(stars), COUNT(*) FROM ("
        "  SELECT stars FROM Reviews WHERE bookId = ?"
        "  UNION ALL"
        "  SELECT stars FROM Ratings WHERE bookId = ?"
        ")",
        {bookId, bookId}
    );
    if (q.next()) {
        const double avg = q.value(0).isNull() ? 0.0 : q.value(0).toDouble();
        const int cnt   = q.value(1).toInt();
        DbConnection::execOk(
            "UPDATE Books SET averageRating = ?, ratingCount = ?, updatedAt = ? WHERE id = ?",
            {avg, cnt, QDateTime::currentDateTime(), bookId}
        );
    }
}
} // namespace

class RatingRepositoryImpl : public IRatingRepository {
public:
    int ratingOfUser(const QString& bookId, const QString& userId) const override
    {
        auto q = DbConnection::run(
            "SELECT stars FROM Ratings WHERE bookId = ? AND userId = ?",
            {bookId, userId}
        );
        return q.next() ? q.value(0).toInt() : 0;
    }

    bool setRating(const QString& bookId, const QString& userId, int stars) override
    {
        if (stars < 1 || stars > 5) return false;
        const bool ok = DbConnection::execOk(
            "INSERT INTO Ratings (id, bookId, userId, stars) VALUES (?, ?, ?, ?) "
            "ON CONFLICT(bookId, userId) DO UPDATE SET stars = excluded.stars",
            {IdGenerator::generateUuid(), bookId, userId, stars}
        );
        if (ok) recalcBookRating(bookId);
        return ok;
    }

    bool removeRating(const QString& bookId, const QString& userId) override
    {
        const bool ok = DbConnection::execOk(
            "DELETE FROM Ratings WHERE bookId = ? AND userId = ?",
            {bookId, userId}
        );
        if (ok) recalcBookRating(bookId);
        return ok;
    }

    double averageRating(const QString& bookId) const override
    {
        auto q = DbConnection::run(
            "SELECT AVG(stars) FROM Ratings WHERE bookId = ?", {bookId}
        );
        return q.next() && !q.value(0).isNull() ? q.value(0).toDouble() : 0.0;
    }

    int count(const QString& bookId) const override
    {
        auto q = DbConnection::run(
            "SELECT COUNT(*) FROM Ratings WHERE bookId = ?", {bookId}
        );
        return q.next() ? q.value(0).toInt() : 0;
    }
};

IRatingRepository* createRatingRepository() {
    static RatingRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
