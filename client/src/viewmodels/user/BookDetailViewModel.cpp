// =============================================================================
//  BookDetailViewModel.cpp
// =============================================================================
#include "viewmodels/user/BookDetailViewModel.h"
#include "services/BookService.h"
#include "services/CartService.h"
#include "services/ReaderService.h"
#include "services/BookDto.h"
#include "services/RatingDistDto.h"
#include "services/AuthService.h"

#include <QDateTime>
#include <algorithm>
#include <memory>
#include <QQmlEngine>

namespace bookclub::client {

BookDetailViewModel::BookDetailViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void BookDetailViewModel::setBookService(BookService* s) {
    if (m_bookService == s) return;
    if (m_bookService) disconnect(m_bookService, nullptr, this, nullptr);
    m_bookService = s;
    if (m_bookService) {
        // BUG FIX (Issue 15): the 2-arg overload `wishlistChanged(QString,bool)`
        // was renamed to `wishlistItemChanged` because QML can't disambiguate
        // overloaded signals by signature. We can now use the plain
        // function-pointer connect (no static_cast needed).
        connect(m_bookService, &BookService::wishlistItemChanged,
                this, [this](const QString& id, bool in){
            if (id == m_bookId) {
                m_inWishlist = in;
                // Issue 7 — keep the BookDto in sync so any QML element
                // bound to `book.inWishlist` (e.g. BookCard on the home
                // page when navigated back) reflects the new state.
                if (m_book) m_book->setInWishlist(in);
                emit wishlistChanged();
            }
        });
        connect(m_bookService, &BookService::reviewsChangedForBook, this, [this](const QString& id){
            if (id == m_bookId) _refreshReviews();
        });
        connect(m_bookService, &BookService::booksChanged, this, [this](){ _refreshBook(); });
    }
    emit bookServiceChanged();
}

void BookDetailViewModel::setCartService(CartService* s) {
    if (m_cartService == s) return;
    if (m_cartService) disconnect(m_cartService, nullptr, this, nullptr);
    m_cartService = s;
    if (m_cartService) {
        connect(m_cartService, &CartService::cartChanged, this, [this](){
            bool in = m_cartService && m_cartService->isInCart(m_bookId);
            if (in != m_inCart) { m_inCart = in; emit cartChanged(); }
        });
    }
    emit cartServiceChanged();
}

void BookDetailViewModel::setReaderService(ReaderService* s) {
    if (m_readerService == s) return;
    if (m_readerService) disconnect(m_readerService, nullptr, this, nullptr);
    m_readerService = s;
    emit readerServiceChanged();
}

void BookDetailViewModel::loadBook(const QString& bookId) {
    m_bookId = bookId;
    if (m_bookService) m_bookService->markRecentlyViewed(bookId);
    // BUG FIX (Issue 32): if a previous op (SubmitReview / DeleteReview)
    // is still in flight, force-cancel it — the user navigated to a new
    // book, so the previous op is no longer relevant. Without this,
    // m_pending would be overwritten and the in-flight timer would fire
    // the wrong branch (e.g. delete the user's review instead of
    // loading the new book).
    if (m_isBusy) {
        m_timer.stop();
        m_isBusy = false;
        emit isBusyChanged(false);
    }
    m_pending = PendingOp::LoadBook;
    beginAsync(300);
}

void BookDetailViewModel::onAsyncReady() {
    if (m_pending == PendingOp::LoadBook) {
        _refreshBook();
        _refreshReviews();
        _refreshRatingDistribution();
        _refreshRelated();
        m_draftRating = 0;
        m_draftComment.clear();
        m_myReviewId.clear();
        emit draftChanged();
        emit bookChanged();
        emit reviewsChanged();
        emit relatedBooksChanged();
        emit wishlistChanged();
        emit cartChanged();
        finishAsync();
    } else if (m_pending == PendingOp::SubmitReview) {
        if (m_bookService && !m_bookId.isEmpty() && canSubmitReview()) {
            if (!m_myReviewId.isEmpty()) {
                m_bookService->updateReview(m_myReviewId, m_draftRating, m_draftComment.trimmed());
            } else {
                m_bookService->submitReview(m_bookId, m_draftRating, m_draftComment.trimmed());
            }
            _refreshReviews();
            _refreshRatingDistribution();
            _refreshBook();
            emit reviewsChanged();
            emit bookChanged();
            emit reviewSubmitted();
        }
        finishAsync();
    } else if (m_pending == PendingOp::DeleteReview) {
        if (m_bookService && !m_myReviewId.isEmpty()) {
            m_bookService->deleteReview(m_myReviewId);
            m_myReviewId.clear();
            m_draftRating = 0;
            m_draftComment.clear();
            _refreshReviews();
            _refreshRatingDistribution();
            _refreshBook();
            emit reviewsChanged();
            emit bookChanged();
            emit draftChanged();
            emit reviewDeleted();
        }
        finishAsync();
    } else {
        finishAsync();
    }
    m_pending = PendingOp::None;
}

void BookDetailViewModel::_refreshBook() {
    if (!m_bookService || m_bookId.isEmpty()) return;
    // BUG FIX: previously did `delete m_book; m_book = bookById(...)` before
    // emitting bookChanged. Any QML binding that read `viewModel.book.xxx`
    // between the delete and the emit would dereference a dangling pointer.
    // We now allocate the new DTO first, then transfer the old DTO to QML's
    // GC (JavaScriptOwnership) so QML can safely release it when no
    // references remain. This prevents use-after-free without requiring the
    // caller to defer the delete.
    BookDto* newBook = qobject_cast<BookDto*>(m_bookService->bookById(m_bookId));
    if (m_book) {
        // Transfer old book to QML GC instead of deleting — QML may still
        // hold references via bindings that haven't been re-evaluated yet.
        QQmlEngine::setObjectOwnership(m_book, QQmlEngine::JavaScriptOwnership);
    }
    m_book = newBook;
    if (m_book) {
        m_inWishlist = m_book->inWishlist();
        m_purchased = m_book->purchased();
    }
    if (m_cartService) m_inCart = m_cartService->isInCart(m_bookId);

    if (m_purchased) {
        if (m_readerService) {
            m_readingPage = m_readerService->lastReadPage(m_bookId);
        } else {
            m_readingPage = 1;
        }
        const QString desc = m_book ? m_book->description() : QString();
        m_readingPageCount = qMax(8, desc.length() / 30);
    } else {
        m_readingPage = 0;
        m_readingPageCount = 0;
    }
}

void BookDetailViewModel::_refreshReviews() {
    if (!m_bookService || m_bookId.isEmpty()) return;
    // BUG FIX: transfer old DTOs to QML GC instead of qDeleteAll — QML
    // may still hold references via the Repeater that hasn't rebuilt yet.
    for (auto* o : m_reviews) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_reviews.clear();
    auto raw = m_bookService->reviewsForBook(m_bookId);

    // Filter
    QList<QObject*> filtered;
    for (auto* o : raw) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && _matchesFilter(r)) {
            // BUG FIX (reviews-not-displayed): give the DTO C++ ownership
            // so QML's garbage collector doesn't collect it while the
            // Repeater is still iterating over the model. Without this,
            // the Repeater would sometimes receive a QObject* whose
            // properties had already been collected, producing empty
            // review cards (or TypeError: cannot read property of null).
            QQmlEngine::setObjectOwnership(r, QQmlEngine::CppOwnership);
            filtered.append(r);
        } else {
            QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
        }
    }

    // Sort — use isPinned() and stars() (the actual ReviewDto API)
    // BUG FIX (Issue 23): previously the comparator had no case for
    // "newest" (the default sort mode) — it fell through to `return false`,
    // leaving reviews in server order. The "oldest" branch reversed the
    // list, which only made sense if the server returned newest-first.
    // We now use `createdAt()` for both "newest" and "oldest" so the
    // sort is correct regardless of server order.
    std::sort(filtered.begin(), filtered.end(), [this](QObject* aObj, QObject* bObj){
        auto a = qobject_cast<ReviewDto*>(aObj);
        auto b = qobject_cast<ReviewDto*>(bObj);
        if (!a || !b) return false;
        if (a->isPinned() != b->isPinned()) return a->isPinned();
        if (m_sortMode == "highest") return a->stars() > b->stars();
        if (m_sortMode == "lowest")  return a->stars() < b->stars();
        if (m_sortMode == "helpful") return a->helpfulCount() > b->helpfulCount();
        if (m_sortMode == "newest")  return a->createdAt() > b->createdAt();
        if (m_sortMode == "oldest")  return a->createdAt() < b->createdAt();
        return false;
    });

    m_reviews = filtered;

    // Find the current user's review — compare userId with AuthService
    const QString currentUserId = AuthService::instance().currentUserId();
    m_myReviewId.clear();
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r) {
            if (r->userId() == currentUserId) {
                r->setByCurrentUser(true);
                m_myReviewId = r->id();
            }
        }
    }
}

void BookDetailViewModel::_refreshRatingDistribution() {
    // BUG FIX: transfer old DTOs to QML GC instead of qDeleteAll.
    for (auto* o : m_ratingDist) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_ratingDist.clear();
    if (!m_bookService || m_bookId.isEmpty()) return;

    int counts[6] = {0,0,0,0,0,0};
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->stars() >= 1 && r->stars() <= 5) counts[r->stars()]++;
    }

    int totalFromReviews = counts[1]+counts[2]+counts[3]+counts[4]+counts[5];
    // BUG FIX (Issue 22): previously, when `m_book` was null, this called
    // `m_bookService->bookById(m_bookId)` which allocates a NEW BookDto
    // that was never deleted → memory leak on every call to
    // `_refreshRatingDistribution` when `m_book` was null. We now use
    // `std::unique_ptr` to ensure the temporary BookDto is freed.
    BookDto* b = m_book;
    std::unique_ptr<BookDto> tmpBook;
    if (!b && m_bookService) {
        tmpBook.reset(qobject_cast<BookDto*>(m_bookService->bookById(m_bookId)));
        b = tmpBook.get();
    }
    int totalCount = b ? b->ratingCount() : totalFromReviews;
    if (totalCount > totalFromReviews && b) {
        int missing = totalCount - totalFromReviews;
        double avg = b->averageRating();
        double w5 = qMax(0.0, avg - 3.0);
        double w4 = qMax(0.0, 4.0 - qAbs(avg - 4.0));
        double w3 = qMax(0.0, 1.0 - qAbs(avg - 3.0));
        double w2 = qMax(0.0, 1.0 - qAbs(avg - 2.0));
        double w1 = qMax(0.0, 1.0 - qAbs(avg - 1.0));
        double sumW = w5+w4+w3+w2+w1;
        if (sumW > 0) {
            counts[5] += int(missing * w5 / sumW);
            counts[4] += int(missing * w4 / sumW);
            counts[3] += int(missing * w3 / sumW);
            counts[2] += int(missing * w2 / sumW);
            counts[1] += missing - counts[5] - counts[4] - counts[3] - counts[2];
        }
    }

    for (int s = 5; s >= 1; --s) {
        m_ratingDist.append(new RatingDistDto(s, counts[s]));
    }
}

int BookDetailViewModel::totalRatings() const {
    int n = 0;
    for (auto* o : m_ratingDist) {
        auto r = qobject_cast<RatingDistDto*>(o);
        if (r) n += r->count();
    }
    return n;
}

int BookDetailViewModel::totalReviewCount() const {
    // BUG FIX (Issue 21): previously this method called
    // `m_bookService->reviewsForBook(m_bookId)` on every QML re-evaluation
    // — a blocking `GetBookDetails` round-trip — because it's bound to a
    // Q_PROPERTY with `NOTIFY reviewsChanged`. Every scroll, tab switch,
    // or `reviewsChanged` emission fired another network request. We now
    // return the count from the already-cached `m_reviews` list (filtered
    // through `_matchesFilter`) instead of re-fetching from the server.
    if (m_bookId.isEmpty()) return 0;
    int n = 0;
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && _matchesFilter(r)) ++n;
    }
    return n;
}

bool BookDetailViewModel::_matchesFilter(ReviewDto* r) const {
    if (!r) return false;
    // verifiedPurchase not tracked — return true for "verified" filter
    if (m_filterMode == "verified") return true;
    if (m_filterMode == "positive") return r->stars() >= 4;
    if (m_filterMode == "negative") return r->stars() <= 2;
    if (m_filterMode == "images")   return false;
    return true;
}

void BookDetailViewModel::_refreshRelated() {
    // BUG FIX: transfer old DTOs to QML GC instead of qDeleteAll.
    for (auto* o : m_relatedBooks)  QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_sameAuthor)    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_samePublisher) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_relatedBooks.clear();
    m_sameAuthor.clear();
    m_samePublisher.clear();
    if (!m_bookService || m_bookId.isEmpty()) return;

    m_relatedBooks  = m_bookService->relatedTo(m_bookId);
    m_sameAuthor    = m_bookService->bySameAuthor(m_bookId);
    m_samePublisher = m_bookService->bySamePublisher(m_bookId);
}

// ----- Review actions -----

void BookDetailViewModel::markHelpful(const QString& reviewId) {
    if (m_bookService) m_bookService->markHelpful(reviewId, true);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::markNotHelpful(const QString& reviewId) {
    if (m_bookService) m_bookService->markHelpful(reviewId, false);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::togglePin(const QString& reviewId) {
    if (!m_bookService) return;
    bool currentlyPinned = false;
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->id() == reviewId) { currentlyPinned = r->isPinned(); break; }
    }
    m_bookService->pinReview(reviewId, !currentlyPinned);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::reportReview(const QString& reviewId) {
    if (m_bookService) m_bookService->flagReview(reviewId);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::addReply(const QString& reviewId, const QString& comment) {
    if (m_bookService) m_bookService->addReply(reviewId, comment);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::deleteReply(const QString& reviewId, const QString& replyId) {
    if (m_bookService) m_bookService->deleteReply(reviewId, replyId);
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::setSortMode(const QString& mode) {
    if (m_sortMode == mode) return;
    m_sortMode = mode;
    emit sortModeChanged();
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::setFilterMode(const QString& mode) {
    if (m_filterMode == mode) return;
    m_filterMode = mode;
    emit filterModeChanged();
    _refreshReviews();
    emit reviewsChanged();
}

void BookDetailViewModel::loadMoreReviews() {
    emit reviewsChanged();
}

void BookDetailViewModel::toggleWishlist() {
    if (!m_bookService || m_bookId.isEmpty()) return;
    m_bookService->toggleWishlist(m_bookId);
}

void BookDetailViewModel::addToCart() {
    if (!m_cartService || m_bookId.isEmpty()) return;
    m_cartService->add(m_bookId);
    m_inCart = true;
    emit cartChanged();
    emit addedToCart(m_bookId);
}

void BookDetailViewModel::buyNow() {
    if (!m_cartService || m_bookId.isEmpty()) return;
    // BUG FIX (payment-flow): add the book to the cart, then immediately
    // call checkout so the book is purchased and added to the user's
    // library. Previously this only emitted purchaseRequested which the
    // UserShell wired to "navigate to cart" — the user had to manually
    // click "Proceed to checkout" on the cart page. Now the "Buy now"
    // button on the BookDetailPage opens a payment dialog, and when the
    // user confirms, this method is called to complete the purchase.
    m_cartService->add(m_bookId);
    m_cartService->checkout();
    emit purchaseRequested(m_bookId);
}

void BookDetailViewModel::submitReview() {
    // BUG FIX (Issue 32): re-entrancy guard.
    if (m_isBusy) return;
    if (!canSubmitReview()) return;
    m_pending = PendingOp::SubmitReview;
    beginAsync(450);
}

void BookDetailViewModel::loadMyReviewIntoDraft() {
    if (!m_bookService || m_bookId.isEmpty() || m_myReviewId.isEmpty()) return;
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->id() == m_myReviewId) {
            m_draftRating = r->stars();
            m_draftComment = r->text();
            emit draftChanged();
            return;
        }
    }
}

void BookDetailViewModel::deleteMyReview() {
    // BUG FIX (Issue 32): re-entrancy guard.
    if (m_isBusy) return;
    if (m_myReviewId.isEmpty()) return;
    m_pending = PendingOp::DeleteReview;
    beginAsync(350);
}

} // namespace bookclub::client
