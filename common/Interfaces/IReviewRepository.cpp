// common/Interfaces/IReviewRepository.cpp
//
// SQLite-backed IReviewRepository.
// On save/update/remove, the parent book's averageRating + ratingCount
// are recalculated so Books queries always return fresh aggregates.
#include "common/Interfaces/IReviewRepository.h"
#include "common/Models/Review.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace bookclub::common {

namespace {
Review* reviewFromCurrentRecord(QSqlQuery& q)
{
    auto* r = new Review;
    r->setId(q.value("id").toString());
    r->setBookId(q.value("bookId").toString());
    r->setUserId(q.value("userId").toString());
    r->setUserDisplayName(q.value("userDisplayName").toString());
    r->setText(q.value("text").toString());
    r->setStars(q.value("stars").toInt());
    r->setEdited(q.value("isEdited").toInt() == 1);
    // Issue 7: load helpful/reply counters if the columns are present.
    // (Guard with record().contains() so this still works on older
    // schema snapshots that lack the columns.)
    const QSqlRecord rec = q.record();
    const int helpfulIdx = rec.indexOf("helpfulCount");
    if (helpfulIdx >= 0) r->setHelpfulCount(q.value(helpfulIdx).toInt());
    const int replyIdx = rec.indexOf("replyCount");
    if (replyIdx >= 0) r->setReplyCount(q.value(replyIdx).toInt());
    r->setCreatedAt(q.value("createdAt").toDateTime());
    r->setUpdatedAt(q.value("updatedAt").toDateTime());
    return r;
}

// Recompute and persist averageRating + ratingCount for the given book.
// Combines Reviews.stars (weighted 1×) and Ratings.stars (weighted 1×).
void recalculateBookRating(const QString& bookId)
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

// ============== Implementation ==============
class ReviewRepositoryImpl : public IReviewRepository {
public:
    Review* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Reviews WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return reviewFromCurrentRecord(q);
    }

    QVector<Review*> findByBook(const QString& bookId) const override
    {
        QVector<Review*> reviews;
        auto q = DbConnection::run(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY isPinned DESC, createdAt DESC",
            {bookId}
        );
        while (q.next()) reviews.append(reviewFromCurrentRecord(q));
        return reviews;
    }

    QVector<Review*> findByUser(const QString& userId) const override
    {
        QVector<Review*> reviews;
        auto q = DbConnection::run(
            "SELECT * FROM Reviews WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (q.next()) reviews.append(reviewFromCurrentRecord(q));
        return reviews;
    }

    bool save(Review* review) override
    {
        if (!review) return false;
        if (review->id().isEmpty()) review->setId(IdGenerator::generateUuid());
        if (!review->createdAt().isValid()) review->setCreatedAt(QDateTime::currentDateTime());
        review->setUpdatedAt(QDateTime::currentDateTime());

        // UPSERT on (bookId, userId) — one review per user per book.
        const QString sql = QStringLiteral(
            "INSERT INTO Reviews (id, bookId, userId, userDisplayName, text, stars,"
            "  isEdited, createdAt, updatedAt) "
            "VALUES (?, ?, ?, ?, ?, ?, 0, ?, ?) "
            "ON CONFLICT(bookId, userId) DO UPDATE SET "
            "  text = excluded.text, stars = excluded.stars, isEdited = 1, updatedAt = excluded.updatedAt"
        );
        const bool ok = DbConnection::execOk(sql, {
            review->id(), review->bookId(), review->userId(),
            review->userDisplayName(), review->text(), review->stars(),
            review->createdAt(), review->updatedAt()
        });

        if (!ok) {
            LOG_ERROR("Failed to save review: " + review->id());
            return false;
        }
        recalculateBookRating(review->bookId());
        return true;
    }

    bool update(Review* review) override
    {
        if (!review || review->id().isEmpty()) return false;
        review->setEdited(true);
        review->setUpdatedAt(QDateTime::currentDateTime());
        const bool ok = DbConnection::execOk(
            "UPDATE Reviews SET text = ?, stars = ?, isEdited = 1, updatedAt = ? WHERE id = ?",
            {review->text(), review->stars(), review->updatedAt(), review->id()}
        );
        if (ok) recalculateBookRating(review->bookId());
        return ok;
    }

    bool remove(const QString& id) override
    {
        // Grab bookId first so we can recalc.
        auto q = DbConnection::run("SELECT bookId FROM Reviews WHERE id = ?", {id});
        QString bookId;
        if (q.next()) bookId = q.value(0).toString();

        const bool ok = DbConnection::execOk("DELETE FROM Reviews WHERE id = ?", {id});
        if (ok && !bookId.isEmpty()) recalculateBookRating(bookId);
        return ok;
    }

    double averageRating(const QString& bookId) const override
    {
        auto q = DbConnection::run("SELECT averageRating FROM Books WHERE id = ?", {bookId});
        return q.next() ? q.value(0).toDouble() : 0.0;
    }

    int ratingCount(const QString& bookId) const override
    {
        auto q = DbConnection::run("SELECT ratingCount FROM Books WHERE id = ?", {bookId});
        return q.next() ? q.value(0).toInt() : 0;
    }

    // Issue 7: increment the helpfulCount counter for a review. The
    // schema doesn't track per-user reactions (no ReviewReactions
    // table), so this is a straight UPDATE — the same user could
    // theoretically click "Helpful" twice. The userId parameter is
    // accepted for API compatibility with a future reactions table.
    bool markHelpful(const QString& reviewId, const QString& userId) override
    {
        Q_UNUSED(userId);
        if (reviewId.isEmpty()) return false;
        const bool ok = DbConnection::execOk(
            "UPDATE Reviews SET helpfulCount = COALESCE(helpfulCount, 0) + 1, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), reviewId}
        );
        if (!ok) LOG_ERROR("Failed to mark review helpful: " + reviewId);
        return ok;
    }

    // Issue 7: bump the replyCount counter for a review. The schema
    // doesn't model a separate Replies table, so we just increment
    // the counter — the reply text itself is logged for future
    // expansion. This keeps the BookDetailViewModel.addReply() flow
    // functional end-to-end without a schema migration.
    bool addReply(const QString& reviewId,
                  const QString& userId,
                  const QString& displayName,
                  const QString& text) override
    {
        if (reviewId.isEmpty()) return false;
        // Best-effort log so the reply content isn't silently lost
        // (a future ReviewReactions / Replies table can pick it up).
        LOG_INFO(QString("Review reply: review=%1 user=%2(%3) text=%4")
                     .arg(reviewId, userId, displayName, text));
        const bool ok = DbConnection::execOk(
            "UPDATE Reviews SET replyCount = COALESCE(replyCount, 0) + 1, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), reviewId}
        );
        if (!ok) LOG_ERROR("Failed to add review reply: " + reviewId);
        return ok;
    }
};

// ============== Factory ==============
IReviewRepository* createReviewRepository() {
    static ReviewRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
