// =============================================================================
//  BookDetailViewModel.cpp
// =============================================================================
#include "viewmodels/user/BookDetailViewModel.h"
#include "services/BookService.h"
#include "services/CartService.h"
#include "services/ReaderService.h"
#include "services/BookDto.h"
#include "services/MockTypes.h"
#include "services/RatingDistDto.h"

#include <QDateTime>
#include <algorithm>

namespace bookclub::client {

BookDetailViewModel::BookDetailViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void BookDetailViewModel::setBookService(BookService* s) {
    if (m_bookService == s) return;
    if (m_bookService) disconnect(m_bookService, nullptr, this, nullptr);
    m_bookService = s;
    if (m_bookService) {
        connect(m_bookService, &BookService::wishlistChanged, this, [this](const QString& id, bool in){
            if (id == m_bookId) { m_inWishlist = in; emit wishlistChanged(); }
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
    // Track this view in the store's recently-viewed list so the HomePage
    // "Recently viewed" carousel reflects real user activity. We do this
    // immediately (not in onAsyncReady) so the order is correct even if the
    // user opens several books quickly.
    if (m_bookService) m_bookService->markRecentlyViewed(bookId);
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
            // Edit existing if present, else create new.
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
    delete m_book;
    m_book = qobject_cast<BookDto*>(m_bookService->bookById(m_bookId));
    if (m_book) {
        m_inWishlist = m_book->inWishlist();
        m_purchased = m_book->purchased();
    }
    if (m_cartService) m_inCart = m_cartService->isInCart(m_bookId);

    // Reading progress — read the real last-read page from ReaderService
    // (which now persists to MockDataStore::upsertReadingProgress). For
    // purchased books that have never been opened, lastReadPage returns 1.
    // The page count is derived from the book's description length using
    // the same heuristic as ReaderService::openBook (qMax(8, len/30)).
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
    qDeleteAll(m_reviews);
    auto raw = m_bookService->reviewsForBook(m_bookId);

    // Filter
    QList<QObject*> filtered;
    for (auto* o : raw) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && _matchesFilter(r)) filtered.append(r);
        else delete o;
    }

    // Sort
    //   `reviewsForBook()` returns reviews newest-first (sorted on the raw
    //   QDateTime::createdAt inside MockDataStore). We preserve that order
    //   for "newest", reverse it for "oldest", and apply the other modes
    //   explicitly. The previous implementation tried to string-compare
    //   relativeTime() labels ("5m ago" vs "1d ago") which sorts
    //   lexicographically and is wrong.
    std::sort(filtered.begin(), filtered.end(), [this](QObject* aObj, QObject* bObj){
        auto a = qobject_cast<ReviewDto*>(aObj);
        auto b = qobject_cast<ReviewDto*>(bObj);
        if (!a || !b) return false;
        // Pinned always first
        if (a->pinned() != b->pinned()) return a->pinned();
        if (m_sortMode == "highest") return a->rating() > b->rating();
        if (m_sortMode == "lowest")  return a->rating() < b->rating();
        if (m_sortMode == "helpful") return a->helpfulCount() > b->helpfulCount();
        // "newest" / default — preserve the service's newest-first order.
        // "oldest" is handled by the reverse pass below.
        return false;
    });

    // "oldest" reverses the service's newest-first order, but keeps pinned
    // reviews at the top.
    if (m_sortMode == "oldest") {
        QList<QObject*> pinned, rest;
        for (auto* o : filtered) {
            auto r = qobject_cast<ReviewDto*>(o);
            if (r && r->pinned()) pinned.append(o);
            else rest.append(o);
        }
        std::reverse(rest.begin(), rest.end());
        filtered = pinned + rest;
    }

    m_reviews = filtered;

    // Find the current user's review (if any) — used for the "edit/delete" UI.
    m_myReviewId.clear();
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->byCurrentUser()) {
            m_myReviewId = r->id();
            break;
        }
    }
}

void BookDetailViewModel::_refreshRatingDistribution() {
    qDeleteAll(m_ratingDist);
    m_ratingDist.clear();
    if (!m_bookService || m_bookId.isEmpty()) return;

    // Derive distribution from reviews. Books with many ratings get a
    // synthetic distribution based on the average (mock).
    int counts[6] = {0,0,0,0,0,0};   // index 1..5
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->rating() >= 1 && r->rating() <= 5) counts[r->rating()]++;
    }

    // If the book has way more total ratings than reviews (mock seeds this
    // gap), synthesize the missing counts from the average.
    int totalFromReviews = counts[1]+counts[2]+counts[3]+counts[4]+counts[5];
    auto b = m_book ? m_book : qobject_cast<BookDto*>(m_bookService->bookById(m_bookId));
    int totalCount = b ? b->ratingCount() : totalFromReviews;
    if (totalCount > totalFromReviews && b) {
        int missing = totalCount - totalFromReviews;
        // Distribute around the average (e.g. avg 4.4 → most 5s and 4s)
        double avg = b->averageRating();
        // Rough split: weight 5★ heaviest, then 4★, etc.
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
    if (!m_bookService || m_bookId.isEmpty()) return 0;
    // Count from the service's raw reviewsForBook — this includes all reviews
    // regardless of the current page/filter, since pagination is over the
    // filtered subset.
    auto all = m_bookService->reviewsForBook(m_bookId);
    int n = 0;
    for (auto* o : all) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (!r) continue;
        if (_matchesFilter(r)) ++n;
        delete o;
    }
    return n;
}

bool BookDetailViewModel::_matchesFilter(ReviewDto* r) const {
    if (!r) return false;
    if (m_filterMode == "verified") return r->verifiedPurchase();
    if (m_filterMode == "positive") return r->rating() >= 4;
    if (m_filterMode == "negative") return r->rating() <= 2;
    if (m_filterMode == "images")   return false;  // mock: no images yet
    return true;  // "all"
}

void BookDetailViewModel::_refreshRelated() {
    qDeleteAll(m_relatedBooks);
    qDeleteAll(m_sameAuthor);
    qDeleteAll(m_samePublisher);
    m_relatedBooks.clear();
    m_sameAuthor.clear();
    m_samePublisher.clear();
    if (!m_bookService || m_bookId.isEmpty()) return;

    m_relatedBooks  = m_bookService->relatedTo(m_bookId);
    m_sameAuthor    = m_bookService->bySameAuthor(m_bookId);
    m_samePublisher = m_bookService->bySamePublisher(m_bookId);
}

// ----- New review actions -----

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
    // Find current pin state
    bool currentlyPinned = false;
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->id() == reviewId) { currentlyPinned = r->pinned(); break; }
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
    // Stub for infinite scroll: in a real backend this would fetch the next
    // page from the server. Here we just emit to refresh UI.
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
    m_cartService->add(m_bookId);
    emit purchaseRequested(m_bookId);
}

void BookDetailViewModel::submitReview() {
    if (!canSubmitReview()) return;
    m_pending = PendingOp::SubmitReview;
    beginAsync(450);
}

void BookDetailViewModel::loadMyReviewIntoDraft() {
    if (!m_bookService || m_bookId.isEmpty() || m_myReviewId.isEmpty()) return;
    for (auto* o : m_reviews) {
        auto r = qobject_cast<ReviewDto*>(o);
        if (r && r->id() == m_myReviewId) {
            m_draftRating = r->rating();
            m_draftComment = r->comment();
            emit draftChanged();
            return;
        }
    }
}

void BookDetailViewModel::deleteMyReview() {
    if (m_myReviewId.isEmpty()) return;
    m_pending = PendingOp::DeleteReview;
    beginAsync(350);
}

} // namespace bookclub::client
