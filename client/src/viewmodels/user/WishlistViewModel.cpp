// =============================================================================
//  WishlistViewModel.cpp
// =============================================================================
#include "viewmodels/user/WishlistViewModel.h"
#include "services/LibraryService.h"
#include "services/CartService.h"
#include "services/BookDto.h"

#include <QLocale>
#include <QQmlEngine>
#include <algorithm>

namespace bookclub::client {

WishlistViewModel::WishlistViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void WishlistViewModel::setLibraryService(LibraryService* s) {
    if (m_libraryService == s) return;
    if (m_libraryService) disconnect(m_libraryService, nullptr, this, nullptr);
    m_libraryService = s;
    if (m_libraryService) {
        // Issue 13 — re-emit booksChanged on BOTH wishlistChanged and
        // libraryChanged so the page refreshes whenever the underlying
        // library data changes (toggleSaved emits both, but other
        // mutations like addToShelf may emit only libraryChanged).
        connect(m_libraryService, &LibraryService::wishlistChanged, this, [this]() {
            // BUG FIX: invalidate the stats cache when books change.
            m_statsCacheValid = false;
            emit booksChanged();
        });
        connect(m_libraryService, &LibraryService::libraryChanged, this, [this]() {
            m_statsCacheValid = false;
            emit booksChanged();
        });
    }
    emit libraryServiceChanged();
    m_statsCacheValid = false;
    emit booksChanged();
}

// Issue 13 — force a refresh of the wishlist from the server.
void WishlistViewModel::refresh() {
    if (m_libraryService) {
        m_libraryService->refresh();
        m_statsCacheValid = false;
        emit booksChanged();
    }
}

void WishlistViewModel::setCartService(CartService* s) {
    if (m_cartService == s) return;
    m_cartService = s;
    emit cartServiceChanged();
}

QList<QObject*> WishlistViewModel::books() const {
    return _filteredSorted();
}

int WishlistViewModel::count() const {
    if (!m_libraryService) return 0;
    return m_libraryService->savedCount();
}

// ---- Aggregate stats (spec-required) ----
//   BUG FIX: previously each of these methods fired a blocking
//   `savedBooks()` network round-trip. The WishlistPage binds to all 5
//   stats simultaneously → 5 blocking round-trips per render. We now
//   compute all stats in one pass via `_refreshStatsCache()` and return
//   the cached values. The cache is invalidated (lazily) whenever
//   `booksChanged` fires.

void WishlistViewModel::_refreshStatsCache() const {
    if (m_statsCacheValid) return;
    m_statsCacheValid = true;
    m_cachedTotalValue = 0.0;
    m_cachedDiscountedCount = 0;
    m_cachedMaxDiscountPercent = 0;
    m_cachedMaxDiscountBookId.clear();
    m_cachedMaxDiscountBookTitle.clear();

    if (!m_libraryService) return;
    // Fetch once, compute all stats, transfer ownership to QML GC (don't delete).
    QList<QObject*> books = m_libraryService->savedBooks();
    for (auto* o : books) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (!dto) {
            QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
            continue;
        }
        m_cachedTotalValue += dto->price();
        if (dto->hasDiscount()) {
            ++m_cachedDiscountedCount;
            const int pct = dto->discountPercent();
            if (pct > m_cachedMaxDiscountPercent) {
                m_cachedMaxDiscountPercent = pct;
                m_cachedMaxDiscountBookId = dto->id();
                m_cachedMaxDiscountBookTitle = dto->title();
            }
        }
        // Transfer ownership to QML GC — QML may still hold references.
        QQmlEngine::setObjectOwnership(dto, QQmlEngine::JavaScriptOwnership);
    }
}

QString WishlistViewModel::totalValueText() const {
    _refreshStatsCache();
    return QStringLiteral("$%1").arg(m_cachedTotalValue, 0, 'f', 2);
}

int WishlistViewModel::discountedCount() const {
    _refreshStatsCache();
    return m_cachedDiscountedCount;
}

int WishlistViewModel::maxDiscountPercent() const {
    _refreshStatsCache();
    return m_cachedMaxDiscountPercent;
}

QString WishlistViewModel::maxDiscountBookId() const {
    _refreshStatsCache();
    return m_cachedMaxDiscountBookId;
}

QString WishlistViewModel::maxDiscountBookTitle() const {
    _refreshStatsCache();
    return m_cachedMaxDiscountBookTitle;
}

void WishlistViewModel::setViewMode(const QString& v) {
    if (m_viewMode == v) return;
    m_viewMode = v; emit viewModeChanged();
}

void WishlistViewModel::setSortMode(const QString& v) {
    if (m_sortMode == v) return;
    m_sortMode = v; emit sortModeChanged(); emit booksChanged();
}

void WishlistViewModel::setSearchQuery(const QString& v) {
    if (m_searchQuery == v) return;
    m_searchQuery = v; emit searchQueryChanged(); emit booksChanged();
}

void WishlistViewModel::setBulkMode(bool v) {
    if (m_bulkMode == v) return;
    m_bulkMode = v;
    if (!v) clearSelection();
    emit bulkModeChanged();
}

void WishlistViewModel::toggleSelected(const QString& bookId) {
    int idx = m_selected.indexOf(bookId);
    if (idx >= 0) m_selected.removeAt(idx);
    else m_selected.append(bookId);
    emit selectionChanged();
}

void WishlistViewModel::selectAll() {
    if (!m_libraryService) return;
    m_selected.clear();
    for (auto* o : m_libraryService->savedBooks()) {
        auto b = qobject_cast<BookDto*>(o);
        if (b) m_selected.append(b->id());
        delete o;
    }
    emit selectionChanged();
}

void WishlistViewModel::clearSelection() {
    if (m_selected.isEmpty()) return;
    m_selected.clear();
    emit selectionChanged();
}

void WishlistViewModel::remove(const QString& bookId) {
    // BUG FIX: use idempotent `removeFromWishlist` instead of `toggleSaved`
    // so removing a book from the wishlist always removes it (toggleSaved
    // would re-add the book if it was already removed by a concurrent action).
    if (m_libraryService) m_libraryService->removeFromWishlist(bookId);
    m_selected.removeAll(bookId);
    emit selectionChanged();
}

void WishlistViewModel::removeSelected() {
    if (m_selected.isEmpty() || !m_libraryService) return;
    QStringList snap = m_selected;
    // BUG FIX: same idempotent fix as `remove`.
    for (const auto& id : snap) m_libraryService->removeFromWishlist(id);
    m_selected.clear();
    emit selectionChanged();
}

void WishlistViewModel::moveToCart(const QString& bookId) {
    if (m_cartService) m_cartService->add(bookId);
    // BUG FIX (Issue 33): use idempotent `removeFromWishlist` instead of
    // `toggleSaved`. `toggleSaved` is a TRUE toggle — if the book was
    // already removed (race, double-click), it re-ADDS the book to the
    // wishlist instead of being a no-op. `removeFromWishlist` always
    // sends RemoveBookFromShelf, so it's safe to call regardless.
    if (m_libraryService) m_libraryService->removeFromWishlist(bookId);
    m_selected.removeAll(bookId);
    emit selectionChanged();
}

void WishlistViewModel::moveSelectedToCart() {
    if (m_selected.isEmpty() || !m_cartService || !m_libraryService) return;
    QStringList snap = m_selected;
    for (const auto& id : snap) {
        m_cartService->add(id);
        // BUG FIX (Issue 33): same fix as moveToCart — use
        // `removeFromWishlist` for idempotency.
        m_libraryService->removeFromWishlist(id);
    }
    m_selected.clear();
    emit selectionChanged();
}

QList<QObject*> WishlistViewModel::_filteredSorted() const {
    if (!m_libraryService) return {};
    QList<QObject*> out = m_libraryService->savedBooks();

    // Filter by search query
    if (!m_searchQuery.trimmed().isEmpty()) {
        const QString q = m_searchQuery.trimmed().toLower();
        QList<QObject*> filtered;
        for (auto* o : out) {
            auto b = qobject_cast<BookDto*>(o);
            if (!b) {
                // BUG FIX: transfer to QML GC instead of delete — QML may
                // still hold references from a previous render.
                QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
                continue;
            }
            if (b->title().toLower().contains(q) ||
                b->authorName().toLower().contains(q)) {
                filtered.append(b);
            } else {
                // BUG FIX: same — transfer to QML GC instead of delete.
                QQmlEngine::setObjectOwnership(b, QQmlEngine::JavaScriptOwnership);
            }
        }
        out = filtered;
    }

    // Sort
    std::sort(out.begin(), out.end(), [this](QObject* aObj, QObject* bObj){
        auto a = qobject_cast<BookDto*>(aObj);
        auto b = qobject_cast<BookDto*>(bObj);
        if (!a || !b) return false;
        if (m_sortMode == "title")      return a->title().toLower() < b->title().toLower();
        if (m_sortMode == "price_asc")  return a->price() < b->price();
        if (m_sortMode == "price_desc") return a->price() > b->price();
        if (m_sortMode == "rating")     return a->averageRating() > b->averageRating();
        return false;  // "recent" — store already returns newest first
    });

    return out;
}

} // namespace bookclub::client
