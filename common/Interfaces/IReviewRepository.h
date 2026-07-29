#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class Review;

class IReviewRepository {
public:
    virtual ~IReviewRepository() = default;

    virtual Review* findById(const QString& id) const = 0;
    virtual QVector<Review*> findByBook(const QString& bookId) const = 0;
    virtual QVector<Review*> findByUser(const QString& userId) const = 0;
    virtual bool save(Review* review) = 0;
    virtual bool update(Review* review) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual double averageRating(const QString& bookId) const = 0;
    virtual int ratingCount(const QString& bookId) const = 0;
    // Issue 7: helpful-mark + reply counters backed by the Reviews
    // table columns `helpfulCount` and `replyCount`. The schema does
    // not model a separate Replies table — addReply just bumps the
    // counter (and is logged for future expansion).
    virtual bool markHelpful(const QString& reviewId, const QString& userId) = 0;
    virtual bool addReply(const QString& reviewId,
                          const QString& userId,
                          const QString& displayName,
                          const QString& text) = 0;
};

IReviewRepository* createReviewRepository();

} // namespace bookclub::common
