// =============================================================================
//  BookDetailViewModel.h
// =============================================================================
//  MVVM view-model for the Book Detail page.
//
//  Owns the currently-loaded book, its reviews, and the user's review-draft
//  state (rating + comment). Handles wishlist toggle, add-to-cart, and
//  review submit/edit/delete.
// =============================================================================
#ifndef BOOKDETAILVIEWMODEL_H
#define BOOKDETAILVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class BookService;
class CartService;
class ReaderService;
class BookDto;
class ReviewDto;

class BookDetailViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(BookService* bookService READ bookService WRITE setBookService NOTIFY bookServiceChanged)
    Q_PROPERTY(CartService* cartService READ cartService WRITE setCartService NOTIFY cartServiceChanged)
    Q_PROPERTY(ReaderService* readerService READ readerService WRITE setReaderService NOTIFY readerServiceChanged)

    Q_PROPERTY(QString bookId    READ bookId    NOTIFY bookChanged)
    Q_PROPERTY(BookDto* book     READ book      NOTIFY bookChanged)
    Q_PROPERTY(QList<QObject*> reviews READ reviews NOTIFY reviewsChanged)
    Q_PROPERTY(int reviewCount   READ reviewCount NOTIFY reviewsChanged)
    Q_PROPERTY(bool inWishlist   READ inWishlist NOTIFY wishlistChanged)
    Q_PROPERTY(bool inCart       READ inCart     NOTIFY cartChanged)
    Q_PROPERTY(bool purchased    READ purchased  NOTIFY bookChanged)

    // Rating distribution (5..1 stars)
    Q_PROPERTY(QList<QObject*> ratingDistribution READ ratingDistribution NOTIFY reviewsChanged)
    Q_PROPERTY(int totalRatings READ totalRatings NOTIFY reviewsChanged)

    // Related book lists
    Q_PROPERTY(QList<QObject*> relatedBooks READ relatedBooks NOTIFY relatedBooksChanged)
    Q_PROPERTY(QList<QObject*> sameAuthor  READ sameAuthor  NOTIFY relatedBooksChanged)
    Q_PROPERTY(QList<QObject*> samePublisher READ samePublisher NOTIFY relatedBooksChanged)

    // Reading progress (for the "Continue reading" indicator on a purchased book)
    Q_PROPERTY(int readingPage READ readingPage NOTIFY bookChanged)
    Q_PROPERTY(int readingPageCount READ readingPageCount NOTIFY bookChanged)
    Q_PROPERTY(bool hasReadingProgress READ hasReadingProgress NOTIFY bookChanged)
    Q_PROPERTY(qreal readingProgress READ readingProgress NOTIFY bookChanged)

    // Review draft
    Q_PROPERTY(int draftRating    READ draftRating    WRITE setDraftRating    NOTIFY draftChanged)
    Q_PROPERTY(QString draftComment READ draftComment WRITE setDraftComment NOTIFY draftChanged)
    Q_PROPERTY(bool canSubmitReview READ canSubmitReview NOTIFY draftChanged)
    Q_PROPERTY(QString myReviewId READ myReviewId NOTIFY reviewsChanged)

    // Review sort + filter + pagination
    Q_PROPERTY(QString sortMode  READ sortMode  WRITE setSortMode  NOTIFY sortModeChanged)
    Q_PROPERTY(QString filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged)
    Q_PROPERTY(int visibleReviewCount READ visibleReviewCount NOTIFY reviewsChanged)
    Q_PROPERTY(int totalReviewCount READ totalReviewCount NOTIFY reviewsChanged)
    Q_PROPERTY(bool canLoadMoreReviews READ canLoadMoreReviews NOTIFY reviewsChanged)

public:
    explicit BookDetailViewModel(QObject* parent = nullptr);

    BookService* bookService() const { return m_bookService; }
    CartService* cartService() const { return m_cartService; }
    ReaderService* readerService() const { return m_readerService; }
    void setBookService(BookService* s);
    void setCartService(CartService* s);
    void setReaderService(ReaderService* s);

    QString bookId() const { return m_bookId; }
    BookDto* book() const { return m_book; }
    QList<QObject*> reviews() const { return m_reviews; }
    int reviewCount() const { return m_reviews.size(); }
    bool inWishlist() const { return m_inWishlist; }
    bool inCart() const { return m_inCart; }
    bool purchased() const { return m_purchased; }

    int draftRating() const { return m_draftRating; }
    QString draftComment() const { return m_draftComment; }
    bool canSubmitReview() const { return m_draftRating >= 1 && m_draftRating <= 5 && m_draftComment.trimmed().length() >= 2; }
    QString myReviewId() const { return m_myReviewId; }

    QString sortMode() const { return m_sortMode; }
    QString filterMode() const { return m_filterMode; }
    int visibleReviewCount() const { return m_reviews.size(); }
    int totalReviewCount() const;
    bool canLoadMoreReviews() const { return m_reviews.size() < totalReviewCount(); }

    // Rating distribution + related
    QList<QObject*> ratingDistribution() const { return m_ratingDist; }
    int totalRatings() const;
    QList<QObject*> relatedBooks() const { return m_relatedBooks; }
    QList<QObject*> sameAuthor() const { return m_sameAuthor; }
    QList<QObject*> samePublisher() const { return m_samePublisher; }

    int readingPage() const { return m_readingPage; }
    int readingPageCount() const { return m_readingPageCount; }
    bool hasReadingProgress() const { return m_readingPage > 0; }
    qreal readingProgress() const {
        return (m_readingPageCount > 0 && m_readingPage > 0)
               ? qreal(m_readingPage) / m_readingPageCount : 0.0;
    }

public slots:
    void setDraftRating(int v) { if (m_draftRating != v) { m_draftRating = v; emit draftChanged(); } }
    void setDraftComment(const QString& v) { if (m_draftComment != v) { m_draftComment = v; emit draftChanged(); } }

    Q_INVOKABLE void loadBook(const QString& bookId);
    Q_INVOKABLE void toggleWishlist();
    Q_INVOKABLE void addToCart();
    Q_INVOKABLE void buyNow();
    Q_INVOKABLE void submitReview();
    Q_INVOKABLE void loadMyReviewIntoDraft();   // prefill draft from existing review
    Q_INVOKABLE void deleteMyReview();
    Q_INVOKABLE void markHelpful(const QString& reviewId);
    Q_INVOKABLE void markNotHelpful(const QString& reviewId);
    Q_INVOKABLE void togglePin(const QString& reviewId);
    Q_INVOKABLE void reportReview(const QString& reviewId);
    Q_INVOKABLE void addReply(const QString& reviewId, const QString& comment);
    Q_INVOKABLE void deleteReply(const QString& reviewId, const QString& replyId);
    Q_INVOKABLE void setSortMode(const QString& mode);
    Q_INVOKABLE void setFilterMode(const QString& mode);
    Q_INVOKABLE void loadMoreReviews();

signals:
    void bookServiceChanged();
    void cartServiceChanged();
    void readerServiceChanged();
    void bookChanged();
    void reviewsChanged();
    void wishlistChanged();
    void cartChanged();
    void draftChanged();
    void relatedBooksChanged();
    void sortModeChanged();
    void filterModeChanged();

    void reviewSubmitted();
    void reviewDeleted();
    void addedToCart(const QString& bookId);
    void purchaseRequested(const QString& bookId);

protected:
    void onAsyncReady() override;

private:
    void _refreshReviews();
    void _refreshBook();
    void _refreshRelated();
    void _refreshRatingDistribution();
    bool _matchesFilter(ReviewDto* r) const;

    BookService* m_bookService = nullptr;
    CartService* m_cartService = nullptr;
    ReaderService* m_readerService = nullptr;

    QString m_bookId;
    BookDto* m_book = nullptr;
    QList<QObject*> m_reviews;
    QList<QObject*> m_ratingDist;
    QList<QObject*> m_relatedBooks;
    QList<QObject*> m_sameAuthor;
    QList<QObject*> m_samePublisher;
    bool m_inWishlist = false;
    bool m_inCart = false;
    bool m_purchased = false;
    int m_readingPage = 0;
    int m_readingPageCount = 0;

    int m_draftRating = 0;
    QString m_draftComment;
    QString m_myReviewId;
    QString m_sortMode = "newest";       // newest | oldest | highest | lowest | helpful
    QString m_filterMode = "all";        // all | verified | positive | negative | images
    int m_reviewPage = 1;
    int m_reviewsPerPage = 5;

    enum class PendingOp { None, LoadBook, SubmitReview, DeleteReview };
    PendingOp m_pending = PendingOp::None;
};

} // namespace bookclub::client

#endif // BOOKDETAILVIEWMODEL_H
