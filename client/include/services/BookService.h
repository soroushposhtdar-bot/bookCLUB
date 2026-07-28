#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QQmlEngine>
#include <QJsonObject>
#include <QJsonArray>

#include "services/BookDto.h"
#include "services/ReviewDto.h"

namespace bookclub::client {

class BookService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit BookService(QObject* parent = nullptr);

    // Kept for QML compatibility — no-op (data comes from server now)
    Q_INVOKABLE void setDataStore(QObject*) {}

    // ----- Catalog queries (synchronous via NetworkService) -----
    Q_INVOKABLE QList<QObject*> recommended() const;
    Q_INVOKABLE QList<QObject*> newReleases() const;
    Q_INVOKABLE QList<QObject*> bestsellers() const;
    Q_INVOKABLE QList<QObject*> freeBooks() const;
    Q_INVOKABLE QList<QObject*> popularBooks() const;
    Q_INVOKABLE QList<QObject*> trending() const;
    Q_INVOKABLE QList<QObject*> editorsPicks() const;
    Q_INVOKABLE QList<QObject*> discounted() const;
    Q_INVOKABLE QList<QObject*> newArrivals() const;
    Q_INVOKABLE QList<QObject*> bySameAuthor(const QString& bookId) const;
    Q_INVOKABLE QList<QObject*> bySamePublisher(const QString& bookId) const;
    Q_INVOKABLE QList<QObject*> relatedTo(const QString& bookId) const;
    Q_INVOKABLE QList<QObject*> becauseYouRead(const QString& bookId) const;
    Q_INVOKABLE QStringList featuredPublishers() const;
    Q_INVOKABLE QList<QObject*> booksByPublisher(const QString& publisherName) const;

    // ----- Continue reading + recently viewed -----
    Q_INVOKABLE QList<QObject*> continueReading() const;
    Q_INVOKABLE QList<QObject*> recentlyViewed() const;
    Q_INVOKABLE void markRecentlyViewed(const QString& bookId);

    Q_INVOKABLE QObject* bookById(const QString& id) const;

    Q_INVOKABLE QStringList availableGenres() const;

    // ----- Search history -----
    Q_INVOKABLE QStringList recentSearches() const;
    Q_INVOKABLE QStringList popularSearches() const;
    Q_INVOKABLE void recordSearch(const QString& query);
    Q_INVOKABLE void clearRecentSearches();

    // ----- Search -----
    Q_INVOKABLE QList<QObject*> search(const QString& query) const;
    // Overload accepting ViewModel-style arguments (field, genres, minPrice, maxPrice, minRating)
    Q_INVOKABLE QList<QObject*> search(const QString& query, const QString& field,
                                       const QStringList& genres, double minPrice,
                                       double maxPrice, int minRating) const;
    Q_INVOKABLE QList<QObject*> searchByAuthor(const QString& author) const;
    Q_INVOKABLE QList<QObject*> searchByTitle(const QString& title) const;
    Q_INVOKABLE QList<QObject*> searchByPublisher(const QString& publisher) const;
    Q_INVOKABLE QList<QObject*> searchByGenre(const QString& genreId) const;
    Q_INVOKABLE QList<QObject*> searchWithFilters(const QString& query,
                                                   const QStringList& genres,
                                                   bool freeOnly,
                                                   const QString& sortBy) const;

    // ----- Reviews -----
    Q_INVOKABLE QList<QObject*> reviewsFor(const QString& bookId) const;
    Q_INVOKABLE QList<QObject*> reviewsForBook(const QString& bookId) const { return reviewsFor(bookId); }
    Q_INVOKABLE bool submitReview(const QString& bookId, int stars, const QString& text);
    Q_INVOKABLE bool updateReview(const QString& reviewId, int stars, const QString& text);
    Q_INVOKABLE bool deleteReview(const QString& reviewId);
    Q_INVOKABLE bool canReview(const QString& bookId) const;
    Q_INVOKABLE QObject* userReviewFor(const QString& bookId) const;

    // Review interactions (server has no endpoints for these yet — local stubs)
    Q_INVOKABLE void markHelpful(const QString& reviewId, bool helpful);
    Q_INVOKABLE void pinReview(const QString& reviewId, bool pinned);
    Q_INVOKABLE void flagReview(const QString& reviewId);
    Q_INVOKABLE void addReply(const QString& reviewId, const QString& comment);
    Q_INVOKABLE void deleteReply(const QString& reviewId, const QString& replyId);

    // ----- Wishlist -----
    Q_INVOKABLE bool isInWishlist(const QString& bookId) const;
    Q_INVOKABLE void toggleWishlist(const QString& bookId);
    Q_INVOKABLE QList<QObject*> wishlist() const;

    // v15c: force-refresh all local caches (wishlist, purchased IDs,
    // home sections). Called by UserShell after a successful checkout so
    // the purchased flag on every BookDto updates immediately and the
    // cart/wishlist buttons disappear for the just-bought book.
    Q_INVOKABLE void refresh();

    // ----- Rating distribution -----
    Q_INVOKABLE QVariantList ratingDistribution(const QString& bookId) const;

signals:
    void catalogChanged();
    void booksChanged();
    void wishlistChanged();
    // BUG FIX (Issue 15): renamed from `wishlistChanged(bookId, inWishlist)`
    // because Qt MOC + QML cannot disambiguate overloaded signals by
    // signature — QML's `onWishlistChanged` only ever bound to the
    // no-arg overload, so the (bookId, inWishlist) delta was invisible
    // to QML listeners. Renamed to `wishlistItemChanged` so QML can
    // address both signals unambiguously.
    void wishlistItemChanged(const QString& bookId, bool inWishlist);
    void reviewsChanged(const QString& bookId);
    void reviewsChangedForBook(const QString& bookId);
    void recentlyViewedChanged();

private:
    // Parse a JSON array of books into QList<QObject*>
    // BUG FIX (wishlist-heart-red): made non-static so it can call
    // ensureWishlistCache() to populate the inWishlist flag on each DTO.
    QList<QObject*> parseBookList(const QJsonArray& arr) const;

    // BUG FIX (wishlist-heart-red): populates m_wishlist from the server's
    // GetLibrary response if the cache is empty. Called by parseBookList()
    // and bookById() so every BookDto gets the correct inWishlist flag.
    void ensureWishlistCache() const;

    // v15c: populates m_purchasedIds from the server's GetLibrary response
    // (purchasedBookIds array). Called by parseBookList() and bookById()
    // so every BookDto gets the correct `purchased` flag — which lets the
    // BookCard hide the cart + wishlist buttons for owned books.
    void ensurePurchasedCache() const;

    // Fetches the home sections payload once and caches it so multiple
    // section getters (bestsellers / freeBooks / newArrivals / etc.) don't
    // each issue a separate round-trip. The cache has a 60-second TTL so
    // the home feed stays fresh during long sessions.
    QJsonObject fetchHomeSections() const;

    // Cache of recently viewed book IDs
    mutable QStringList m_recentlyViewed;
    mutable QStringList m_recentSearches;

    // Local wishlist (mirror of the server's Wishlist shelf)
    mutable QStringList m_wishlist;

    // v15c: cache of book IDs the user has already purchased (mirror of
    // the server's GetLibrary.purchasedBookIds array). Used to set the
    // `purchased` flag on every BookDto so the UI can hide cart/wishlist
    // buttons for owned books.
    mutable QStringList m_purchasedIds;

    // Cached GetHomeSections response (cleared by refresh, by any mutation,
    // or after 60 seconds).
    mutable QJsonObject m_homeSectionsCache;
    mutable qint64 m_homeSectionsCacheTime = 0;  // msecs since epoch

    // Issue: cache for continueReading() so the Home page doesn't re-fetch
    // the library + book details on every refresh. 30-second TTL.
    mutable QList<QObject*> m_continueReadingCache;
    mutable qint64 m_continueReadingCacheTime = 0;
    mutable bool m_continueReadingCacheValid = false;
};

} // namespace bookclub::client
