#include "services/BookService.h"
#include "services/NetworkService.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QSet>
#include <QtMath>

namespace bookclub::client {

BookService::BookService(QObject* parent) : QObject(parent)
{
    // Real-time event subscriptions — refresh the catalog when:
    //   - a new book matching the user's favourite genres is published
    //     (EvtBookAdded)
    //   - a discount is applied to a book the user can see
    //     (EvtDiscountApplied)
    //   - a review is added/edited on a book (EvtReviewUpdated) — also
    //     re-emits reviewsChangedForBook so the BookDetailViewModel
    //     re-fetches the affected book's reviews
    NetworkService& net = NetworkService::instance();
    net.subscribeEvent(common::Command::EvtBookAdded, this,
        [this](const common::Message& /*msg*/) {
            m_homeSectionsCache = QJsonObject();
            m_homeSectionsCacheTime = 0;
            emit booksChanged();
            emit catalogChanged();
        });
    net.subscribeEvent(common::Command::EvtDiscountApplied, this,
        [this](const common::Message& msg) {
            m_homeSectionsCache = QJsonObject();
            m_homeSectionsCacheTime = 0;
            emit booksChanged();
            const QString bookId = msg.payload().value("bookId").toString();
            if (!bookId.isEmpty()) emit reviewsChangedForBook(bookId);
        });
    net.subscribeEvent(common::Command::EvtReviewUpdated, this,
        [this](const common::Message& msg) {
            const QString bookId = msg.payload().value("bookId").toString();
            if (!bookId.isEmpty()) {
                emit reviewsChanged(bookId);
                emit reviewsChangedForBook(bookId);
            }
        });
    // Issue: invalidate the continueReading cache when books change so the
    // Home page picks up new purchases immediately.
    connect(this, &BookService::booksChanged, this, [this]() {
        m_continueReadingCacheValid = false;
    });
}

QList<QObject*> BookService::parseBookList(const QJsonArray& arr) const {
    QList<QObject*> result;
    // BUG FIX (wishlist-heart-red): populate the inWishlist flag on every
    // BookDto so the heart icon renders red on every page (Home, Search,
    // BookDetail, etc.) for books the user has wishlisted. Previously the
    // flag was only set on the BookDetailViewModel's book, so the heart
    // appeared white on all other pages even when the book was in the
    // wishlist.
    ensureWishlistCache();
    // v15c: also populate the purchased flag so BookCard can hide the
    // cart + wishlist buttons for owned books.
    ensurePurchasedCache();
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        auto* dto = new BookDto();
        dto->fromJson(v.toObject());
        dto->setInWishlist(m_wishlist.contains(dto->id()));
        // v15c: prefer the server's `purchased` field if present (e.g.
        // GetBookDetails), otherwise fall back to the local cache.
        if (!dto->purchased()) {
            dto->setPurchased(m_purchasedIds.contains(dto->id()));
        }
        result.append(dto);
    }
    return result;
}

// v15c: force-refresh all local caches. Called after a successful checkout
// so the purchased flag on every BookDto updates immediately.
void BookService::refresh() {
    m_wishlist.clear();
    m_purchasedIds.clear();
    m_homeSectionsCache = QJsonObject();
    m_homeSectionsCacheTime = 0;
    m_continueReadingCacheValid = false;
    emit catalogChanged();
    emit booksChanged();
}

// BUG FIX (wishlist-heart-red): helper that ensures m_wishlist is populated
// from the server. Called by parseBookList() and bookById() so every BookDto
// gets the correct inWishlist flag.
void BookService::ensureWishlistCache() const {
    if (!m_wishlist.isEmpty()) return;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (!resp.isSuccess()) return;
    const QJsonArray shelves = resp.payload.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            for (const auto& bid : s.value("bookIds").toArray()) {
                m_wishlist.append(bid.toString());
            }
            break;
        }
    }
    // v15c: while we have the GetLibrary payload, also populate the
    // purchased-ids cache so we don't issue a second round-trip later.
    if (m_purchasedIds.isEmpty()) {
        const QJsonArray purchased = resp.payload.value("purchasedBookIds").toArray();
        for (const auto& bid : purchased) {
            m_purchasedIds.append(bid.toString());
        }
    }
}

// v15c: populates m_purchasedIds from the server's GetLibrary response.
// Called by parseBookList() and bookById() so every BookDto gets the
// correct `purchased` flag.
void BookService::ensurePurchasedCache() const {
    if (!m_purchasedIds.isEmpty()) return;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (!resp.isSuccess()) return;
    const QJsonArray purchased = resp.payload.value("purchasedBookIds").toArray();
    for (const auto& bid : purchased) {
        m_purchasedIds.append(bid.toString());
    }
    // Also populate wishlist if empty (free round-trip).
    if (m_wishlist.isEmpty()) {
        const QJsonArray shelves = resp.payload.value("shelves").toArray();
        for (const auto& v : shelves) {
            const QJsonObject s = v.toObject();
            if (s.value("name").toString() == "Wishlist") {
                for (const auto& bid : s.value("bookIds").toArray()) {
                    m_wishlist.append(bid.toString());
                }
                break;
            }
        }
    }
}

// ============================================================================
//  Catalog queries — all fetch from GetHomeSections
// ============================================================================

QList<QObject*> BookService::recommended() const  { return bestsellers(); }
QList<QObject*> BookService::newReleases() const   { return newArrivals(); }
QList<QObject*> BookService::popularBooks() const  { return bestsellers(); }
QList<QObject*> BookService::trending() const      { return bestsellers(); }
QList<QObject*> BookService::editorsPicks() const  { return bestsellers(); }
// Issue: discounted() now returns books that actually have a discount,
// not free books. We fetch all books and filter by hasDiscount locally
// because the server's GetHomeSections doesn't have a "discounted" section.
QList<QObject*> BookService::discounted() const {
    // Issue: discounted() now returns books that actually have a discount,
    // not free books. We fetch all books and filter by hasDiscount locally
    // because the server's GetHomeSections doesn't have a "discounted" section.
    QList<QObject*> all;
    all += bestsellers();
    all += freeBooks();
    all += newArrivals();
    QList<QObject*> result;
    QSet<QString> seen;
    for (auto* o : all) {
        auto* b = qobject_cast<BookDto*>(o);
        if (!b) { delete o; continue; }
        if (b->hasDiscount() && !seen.contains(b->id())) {
            seen.insert(b->id());
            result.append(b);
        } else {
            delete b;
        }
    }
    return result;
}
QList<QObject*> BookService::becauseYouRead(const QString&) const { return bestsellers(); }
QList<QObject*> BookService::relatedTo(const QString&) const      { return bestsellers(); }
QList<QObject*> BookService::bySameAuthor(const QString&) const   { return QList<QObject*>(); }
QList<QObject*> BookService::bySamePublisher(const QString&) const { return QList<QObject*>(); }

// Issue 5 — populate Continue Reading from the user's library.
// The server's GetLibrary response includes `lastOpenedBookId` and
// `lastOpenedPage` for the user, plus a `purchasedBookIds` array. We
// fetch the book details via GetBooksByIds and tag each DTO with the
// saved reading page so the HeroBanner can show real progress.
// Issue (this batch): added a 30-second cache so the Home page's 10-second
// refresh timer doesn't re-fetch the library + book details every time.
QList<QObject*> BookService::continueReading() const {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_continueReadingCacheValid && (now - m_continueReadingCacheTime) < 30000) {
        return m_continueReadingCache;
    }

    // Free the old cache.
    // v15j: CRITICAL FIX — transfer old DTOs to QML GC instead of
    // qDeleteAll. QML's Repeater may still hold references to these
    // DTOs via bindings that haven't been re-evaluated yet. Deleting
    // them here causes a use-after-free crash when QML tries to access
    // a property on a deleted DTO. This was the root cause of the
    // "crash after 3 minutes on random places" bug.
    for (auto* o : m_continueReadingCache) {
        QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    }
    m_continueReadingCache.clear();

    auto libResp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (!libResp.isSuccess()) {
        m_continueReadingCacheValid = false;
        return m_continueReadingCache;
    }

    const QString lastBookId = libResp.payload.value("lastOpenedBookId").toString();
    const int lastPage       = libResp.payload.value("lastOpenedPage").toInt(0);

    // Build a list of purchased book IDs, with the last-opened one first.
    QStringList purchasedIds;
    const QJsonArray purchasedArr = libResp.payload.value("purchasedBookIds").toArray();
    for (const auto& v : purchasedArr) {
        const QString id = v.toString();
        if (!id.isEmpty()) purchasedIds.append(id);
    }
    if (!lastBookId.isEmpty()) {
        purchasedIds.removeAll(lastBookId);
        purchasedIds.prepend(lastBookId);
    }
    if (purchasedIds.isEmpty()) {
        m_continueReadingCacheValid = true;
        m_continueReadingCacheTime = now;
        return m_continueReadingCache;
    }

    // Cap at 8 so the carousel stays manageable.
    while (purchasedIds.size() > 8) purchasedIds.removeLast();

    QJsonObject p;
    QJsonArray idsArr;
    for (const QString& id : purchasedIds) idsArr.append(id);
    p["bookIds"] = idsArr;
    auto booksResp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (!booksResp.isSuccess()) {
        m_continueReadingCacheValid = false;
        return m_continueReadingCache;
    }

    for (const auto& v : booksResp.payload.value("books").toArray()) {
        if (!v.isObject()) continue;
        auto* dto = new BookDto();
        dto->fromJson(v.toObject());
        dto->setPurchased(true);
        const int page = (dto->id() == lastBookId && lastPage > 0) ? lastPage : 1;
        const int pageCountEst = qMax(8, dto->description().length() / 30);
        dto->setProperty("readingPage", page);
        dto->setProperty("readingPageCount", pageCountEst);
        dto->setProperty("readingProgress", pageCountEst > 0 ? qreal(page) / qreal(pageCountEst) : 0.0);
        m_continueReadingCache.append(dto);
    }
    m_continueReadingCacheValid = true;
    m_continueReadingCacheTime = now;
    return m_continueReadingCache;
}

QList<QObject*> BookService::bestsellers() const {
    return parseBookList(fetchHomeSections().value("bestSellers").toArray());
}

QList<QObject*> BookService::freeBooks() const {
    const auto arr = fetchHomeSections().value("freeBooks").toArray();
    if (!arr.isEmpty()) return parseBookList(arr);
    // v11: server may return an empty freeBooks array due to a Book::isFree()
    // bug. Fall back to filtering all known books by price == 0 locally.
    QList<QObject*> result;
    // Combine bestsellers + newArrivals as the source set
    auto all = parseBookList(fetchHomeSections().value("bestSellers").toArray());
    auto newArrivals = parseBookList(fetchHomeSections().value("newBooks").toArray());
    QSet<QString> seenIds;
    for (const auto& list : {all, newArrivals}) {
        for (QObject* obj : list) {
            auto* dto = qobject_cast<BookDto*>(obj);
            if (!dto) continue;
            if (seenIds.contains(dto->id())) { delete dto; continue; }
            seenIds.insert(dto->id());
            if (dto->isFree() || dto->price() == 0.0) {
                result.append(dto);
            } else {
                delete dto;
            }
        }
    }
    return result;
}

QList<QObject*> BookService::newArrivals() const {
    return parseBookList(fetchHomeSections().value("newBooks").toArray());
}

QJsonObject BookService::fetchHomeSections() const {
    // 60-second TTL — the home feed shouldn't go more than a minute stale
    // during a long session. Real-time events (EvtBookAdded,
    // EvtDiscountApplied) clear the cache immediately.
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (!m_homeSectionsCache.isEmpty() && (now - m_homeSectionsCacheTime) < 60000) {
        return m_homeSectionsCache;
    }
    auto resp = NetworkService::instance().sendRequest(common::Command::GetHomeSections);
    if (resp.isSuccess()) {
        m_homeSectionsCache = resp.payload;
        m_homeSectionsCacheTime = now;
    }
    return m_homeSectionsCache;
}

QStringList BookService::featuredPublishers() const { return {}; }
QList<QObject*> BookService::booksByPublisher(const QString&) const { return {}; }

// ============================================================================
//  Recently viewed (local cache)
// ============================================================================

QList<QObject*> BookService::recentlyViewed() const {
    QList<QObject*> result;
    for (const QString& id : m_recentlyViewed) {
        QObject* b = bookById(id);
        if (b) result.append(b);
    }
    return result;
}

void BookService::markRecentlyViewed(const QString& bookId) {
    m_recentlyViewed.removeAll(bookId);
    m_recentlyViewed.prepend(bookId);
    if (m_recentlyViewed.size() > 20) m_recentlyViewed.removeLast();
    emit recentlyViewedChanged();
}

// ============================================================================
//  Book details
// ============================================================================

QObject* BookService::bookById(const QString& id) const {
    QJsonObject p;
    p["bookId"] = id;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    if (!resp.isSuccess()) return nullptr;
    auto* dto = new BookDto();
    dto->fromJson(resp.payload);
    // BUG FIX (wishlist-heart-red): set the inWishlist flag on the book
    // detail DTO too, so the heart on the BookDetailPage renders red when
    // the book is already in the user's wishlist.
    ensureWishlistCache();
    dto->setInWishlist(m_wishlist.contains(id));
    // v15c: also set the purchased flag. The server's GetBookDetails
    // response now includes a `purchased` field, but we also check the
    // local cache as a fallback (e.g. older server).
    ensurePurchasedCache();
    if (!dto->purchased()) {
        dto->setPurchased(m_purchasedIds.contains(id));
    }
    return dto;
}

// ============================================================================
//  Genres
// ============================================================================

QStringList BookService::availableGenres() const {
    // Issue 3: keep in sync with AuthService::_defaultGenres() AND the
    // server's `Genres` seed table (database/seeds/sample_data.sql).
    // The server stores book.genreIds by display name, so the client
    // must present the SAME names here — otherwise the UI shows raw
    // "genre-001" IDs from the server.
    return {
        QStringLiteral("Programming"),    QStringLiteral("Novel"),
        QStringLiteral("History"),        QStringLiteral("Poetry"),
        QStringLiteral("Biography"),      QStringLiteral("Self-Help"),
        QStringLiteral("Business"),       QStringLiteral("Science"),
        QStringLiteral("Fiction"),        QStringLiteral("Non-Fiction"),
        QStringLiteral("Mystery"),        QStringLiteral("Thriller"),
        QStringLiteral("Romance"),        QStringLiteral("Fantasy"),
        QStringLiteral("Technology"),     QStringLiteral("Young Adult"),
        QStringLiteral("Children's")
    };
}

// ============================================================================
//  Search
// ============================================================================

QStringList BookService::recentSearches() const { return m_recentSearches; }
QStringList BookService::popularSearches() const {
    return { QStringLiteral("Qt"), QStringLiteral("C++"), QStringLiteral("Fantasy") };
}

void BookService::recordSearch(const QString& query) {
    // Issue 15 — don't record empty or whitespace-only queries.
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) return;
    m_recentSearches.removeAll(trimmed);
    m_recentSearches.prepend(trimmed);
    if (m_recentSearches.size() > 10) m_recentSearches.removeLast();
}

void BookService::clearRecentSearches() { m_recentSearches.clear(); }

QList<QObject*> BookService::search(const QString& query) const {
    QJsonObject p;
    p["keyword"] = query;
    // Issue 15 — send the field so the server can do field-specific search.
    // The “all” field triggers the server's multi-field (title + author +
    // publisher) deduplicated search.
    auto resp = NetworkService::instance().sendRequest(common::Command::SearchBooks, p);
    if (!resp.isSuccess()) return {};
    return parseBookList(resp.payload.value("results").toArray());
}

QList<QObject*> BookService::search(const QString& query, const QString& field,
                                     const QStringList& genres, double minPrice,
                                     double maxPrice, int minRating) const {
    // Issue 15 — pass the field to the server so it can route to
    // searchBooksByField. The server still returns ALL matches for that
    // field; we filter by price + rating locally.
    QJsonObject p;
    p["keyword"] = query;
    if (!field.isEmpty() && field != QStringLiteral("all")) p["field"] = field;
    auto resp = NetworkService::instance().sendRequest(common::Command::SearchBooks, p);
    if (!resp.isSuccess()) return {};

    QList<QObject*> parsed = parseBookList(resp.payload.value("results").toArray());
    // Local post-filter by price + rating (server doesn't support these yet).
    QList<QObject*> filtered;
    for (QObject* o : parsed) {
        auto* b = qobject_cast<BookDto*>(o);
        if (!b) { delete o; continue; }
        if (b->price() < minPrice) { delete b; continue; }
        // When maxPrice is 0, we're filtering for free books (price == 0)
        if (maxPrice == 0 && minPrice == 0) {
            // Only free books
            if (b->price() > 0.0) { delete b; continue; }
        } else if (maxPrice > 0 && b->price() > maxPrice) {
            delete b; continue;
        }
        // Genre filter
        if (!genres.isEmpty()) {
            bool genreMatch = false;
            for (const QString& g : genres) {
                if (b->genreIds().contains(g, Qt::CaseInsensitive)) {
                    genreMatch = true;
                    break;
                }
            }
            if (!genreMatch) { delete b; continue; }
        }
        if (b->averageRating() < minRating) { delete b; continue; }
        filtered.append(b);
    }
    return filtered;
}

QList<QObject*> BookService::searchByAuthor(const QString& a) const   { return search(a); }
QList<QObject*> BookService::searchByTitle(const QString& t) const    { return search(t); }
QList<QObject*> BookService::searchByPublisher(const QString& p) const { return search(p); }
QList<QObject*> BookService::searchByGenre(const QString& g) const    { return search(g); }
QList<QObject*> BookService::searchWithFilters(const QString& q, const QStringList&, bool, const QString&) const {
    return search(q);
}

// ============================================================================
//  Reviews
// ============================================================================

QList<QObject*> BookService::reviewsFor(const QString& bookId) const {
    // Reviews come embedded in GetBookDetails response
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    if (!resp.isSuccess()) return {};

    QList<QObject*> result;
    const QJsonArray reviews = resp.payload.value("reviews").toArray();
    for (const auto& v : reviews) {
        if (!v.isObject()) continue;
        auto* r = new ReviewDto();
        r->fromJson(v.toObject());
        result.append(r);
    }
    return result;
}

bool BookService::submitReview(const QString& bookId, int stars, const QString& text) {
    QJsonObject p;
    p["bookId"] = bookId;
    p["stars"]  = stars;
    p["text"]   = text;
    auto resp = NetworkService::instance().sendRequest(common::Command::SubmitReview, p);
    if (resp.isSuccess()) {
        // Invalidate caches so the next read fetches fresh data.
        m_homeSectionsCache = QJsonObject();
        emit reviewsChangedForBook(bookId);
        emit reviewsChanged(bookId);
        return true;
    }
    return false;
}

bool BookService::updateReview(const QString& reviewId, int stars, const QString& text) {
    QJsonObject p;
    p["id"]    = reviewId;
    p["stars"] = stars;
    p["text"]  = text;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateReview, p);
    return resp.isSuccess();
}

bool BookService::deleteReview(const QString& reviewId) {
    QJsonObject p;
    p["id"] = reviewId;
    auto resp = NetworkService::instance().sendRequest(common::Command::DeleteReview, p);
    return resp.isSuccess();
}
bool BookService::canReview(const QString&) const { return false; }
QObject* BookService::userReviewFor(const QString&) const { return nullptr; }

// Review interactions — Issue 7.
// markHelpful + addReply now hit the server (see MarkReviewHelpful /
// AddReviewReply in Protocol.h). The other three are still stubs
// because the server doesn't model pin / flag / reply-delete yet —
// they're kept on the API surface so the QML call sites compile.
void BookService::markHelpful(const QString& reviewId, bool helpful) {
    if (reviewId.isEmpty()) return;
    QJsonObject p;
    p["reviewId"] = reviewId;
    p["helpful"]  = helpful;
    // Fire-and-forget — the server just bumps the helpfulCount column.
    // We emit reviewsChangedForBook("") so any open BookDetailPage can
    // refetch and update the badge (the page keys off bookId, but the
    // signal carries the bookId of the currently-open book; passing an
    // empty string is the established "invalidate all" convention used
    // elsewhere in this service).
    NetworkService::instance().sendAsync(common::Command::MarkReviewHelpful, p);
    emit reviewsChangedForBook(QString());
}
void BookService::pinReview(const QString&, bool) {}
void BookService::flagReview(const QString&) {}
void BookService::addReply(const QString& reviewId, const QString& comment) {
    if (reviewId.isEmpty() || comment.trimmed().isEmpty()) return;
    QJsonObject p;
    p["reviewId"] = reviewId;
    p["text"]     = comment.trimmed();
    NetworkService::instance().sendAsync(common::Command::AddReviewReply, p);
    emit reviewsChangedForBook(QString());
}
void BookService::deleteReply(const QString&, const QString&) {}

// ============================================================================
//  Wishlist (server-backed via the Wishlist system shelf)
// ============================================================================

// Helper: look up the user's Wishlist shelf ID via GetLibrary, with caching.
static QString wishlistShelfId()
{
    static QString cached;
    if (!cached.isEmpty()) return cached;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (!resp.isSuccess()) return {};
    const QJsonArray shelves = resp.payload.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            cached = s.value("id").toString();
            return cached;
        }
    }
    return {};
}

bool BookService::isInWishlist(const QString& bookId) const {
    // Always re-fetch from server to ensure accuracy.
    // The m_wishlist cache is updated by toggleWishlist, but if another
    // client modified the wishlist, the cache would be stale.
    // For performance, we use the cache if it's been populated.
    if (m_wishlist.isEmpty()) {
        auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
        if (resp.isSuccess()) {
            const QJsonArray shelves = resp.payload.value("shelves").toArray();
            for (const auto& v : shelves) {
                const QJsonObject s = v.toObject();
                if (s.value("name").toString() == "Wishlist") {
                    for (const auto& bid : s.value("bookIds").toArray()) {
                        m_wishlist.append(bid.toString());
                    }
                    break;
                }
            }
        }
    }
    return m_wishlist.contains(bookId);
}

void BookService::toggleWishlist(const QString& bookId) {
    // Always re-fetch the wishlist shelf ID + current state from the server
    // to avoid stale cache issues.
    auto libResp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (!libResp.isSuccess()) {
        // Server unreachable — fall back to local-only toggle.
        bool nowIn;
        if (m_wishlist.contains(bookId)) { m_wishlist.removeAll(bookId); nowIn = false; }
        else { m_wishlist.append(bookId); nowIn = true; }
        emit wishlistChanged();
        emit wishlistItemChanged(bookId, nowIn);
        return;
    }

    // Find the Wishlist shelf and check if the book is already in it.
    QString shelfId;
    bool currentlyIn = false;
    const QJsonArray shelves = libResp.payload.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            shelfId = s.value("id").toString();
            currentlyIn = s.value("bookIds").toArray().contains(bookId);
            break;
        }
    }

    if (shelfId.isEmpty()) {
        // No Wishlist shelf — fall back to local-only.
        bool nowIn;
        if (m_wishlist.contains(bookId)) { m_wishlist.removeAll(bookId); nowIn = false; }
        else { m_wishlist.append(bookId); nowIn = true; }
        emit wishlistChanged();
        emit wishlistItemChanged(bookId, nowIn);
        return;
    }

    QJsonObject p;
    p["shelfId"] = shelfId;
    p["bookId"]  = bookId;
    common::Command cmd = currentlyIn
        ? common::Command::RemoveBookFromShelf
        : common::Command::AddBookToShelf;
    auto resp = NetworkService::instance().sendRequest(cmd, p);
    if (resp.isSuccess()) {
        bool nowIn;
        if (currentlyIn) { m_wishlist.removeAll(bookId); nowIn = false; }
        else { m_wishlist.append(bookId); nowIn = true; }
        emit wishlistChanged();
        emit wishlistItemChanged(bookId, nowIn);
        // BUG FIX (wishlist-heart-red): emit booksChanged + catalogChanged
        // so the Home + Search pages re-fetch their book lists with the
        // updated inWishlist flag. Without this, the heart on a BookCard
        // wouldn't turn red until the user manually refreshed the page.
        m_homeSectionsCache = QJsonObject();
        m_homeSectionsCacheTime = 0;
        emit booksChanged();
        emit catalogChanged();
    }
}

QList<QObject*> BookService::wishlist() const {
    // Force a refresh of the local cache, then batch-fetch the book details.
    m_wishlist.clear();
    auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (resp.isSuccess()) {
        const QJsonArray shelves = resp.payload.value("shelves").toArray();
        for (const auto& v : shelves) {
            const QJsonObject s = v.toObject();
            if (s.value("name").toString() == "Wishlist") {
                for (const auto& bid : s.value("bookIds").toArray()) {
                    m_wishlist.append(bid.toString());
                }
                break;
            }
        }
    }

    QList<QObject*> result;
    if (m_wishlist.isEmpty()) return result;
    QJsonObject p;
    p["bookIds"] = QJsonArray::fromStringList(m_wishlist);
    auto booksResp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (booksResp.isSuccess()) {
        for (const auto& v : booksResp.payload.value("books").toArray()) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            dto->setInWishlist(true);
            result.append(dto);
        }
    }
    return result;
}

// ============================================================================
//  Rating distribution (computed from reviews)
// ============================================================================

QVariantList BookService::ratingDistribution(const QString& bookId) const {
    auto revs = reviewsFor(bookId);
    QVariantList dist;
    for (int star = 5; star >= 1; --star) {
        int count = 0;
        for (QObject* r : revs) {
            if (r->property("stars").toInt() == star) count++;
        }
        QVariantMap entry;
        entry["stars"] = star;
        entry["count"] = count;
        entry["percent"] = revs.isEmpty() ? 0 : (count * 100 / revs.size());
        dist.append(entry);
    }
    return dist;
}

} // namespace bookclub::client
