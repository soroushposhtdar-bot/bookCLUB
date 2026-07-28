// =============================================================================
//  SearchViewModel.cpp
// =============================================================================
#include "viewmodels/user/SearchViewModel.h"
#include "services/BookService.h"
#include "services/CartService.h"
#include "services/BookDto.h"
#include "services/FilterChipDto.h"

#include <algorithm>
#include <QQmlEngine>

namespace bookclub::client {

SearchViewModel::SearchViewModel(QObject* parent)
    : UserViewModelBase(parent)
{
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(300);
    connect(&m_debounceTimer, &QTimer::timeout, this, [this](){ search(); });
    // BUG FIX (Issue 34): debounce suggestion refresh so we don't fire a
    // blocking `BookService::search(...)` round-trip on every keystroke.
    // The main results search was already debounced (300ms via
    // m_debounceTimer), but `_refreshSuggestions` was called synchronously
    // from `setQuery` — typing "hello" fired 4 blocking searches (he,
    // hel, hell, hello), each freezing the UI. We now use a separate
    // 150ms timer for suggestions.
    m_suggestionDebounceTimer.setSingleShot(true);
    m_suggestionDebounceTimer.setInterval(150);
    connect(&m_suggestionDebounceTimer, &QTimer::timeout, this, [this](){ _refreshSuggestions(); });
}

void SearchViewModel::setBookService(BookService* s) {
    if (m_bookService == s) return;
    if (m_bookService) disconnect(m_bookService, nullptr, this, nullptr);
    m_bookService = s;
    emit bookServiceChanged();
    if (m_bookService) {
        _refreshSuggestions();
        emit searchHistoryChanged();
        // BUG FIX: listen to wishlistItemChanged so the BookCard hearts in
        // the search results update in real-time when the user toggles
        // wishlist from any page. Without this, the heart icon stays in
        // its old state until the user re-runs the search.
        connect(m_bookService, &BookService::wishlistItemChanged, this, [this](const QString& bookId, bool inWishlist) {
            for (auto* o : m_results) {
                auto* b = qobject_cast<BookDto*>(o);
                if (b && b->id() == bookId) {
                    b->setInWishlist(inWishlist);
                }
            }
            emit resultsChanged();
        });
    }
}

void SearchViewModel::setCartService(CartService* s) {
    if (m_cartService == s) return;
    m_cartService = s;
    emit cartServiceChanged();
}

void SearchViewModel::addToCart(const QString& bookId) {
    if (m_cartService) m_cartService->add(bookId);
}

void SearchViewModel::toggleWishlist(const QString& bookId) {
    if (m_bookService) m_bookService->toggleWishlist(bookId);
}

QStringList SearchViewModel::availableGenres() const {
    return m_bookService ? m_bookService->availableGenres() : QStringList{};
}

QStringList SearchViewModel::recentSearches() const {
    return m_bookService ? m_bookService->recentSearches() : QStringList{};
}

QStringList SearchViewModel::popularSearches() const {
    return m_bookService ? m_bookService->popularSearches() : QStringList{};
}

void SearchViewModel::setQuery(const QString& v) {
    if (m_query == v) return;
    m_query = v;
    emit queryChanged();
    // BUG FIX (Issue 34): schedule the suggestion refresh on the
    // debounced timer instead of calling `_refreshSuggestions()`
    // synchronously, so we don't fire a blocking search per keystroke.
    m_suggestionDebounceTimer.start();
    _scheduleSearch();
}

void SearchViewModel::setField(const QString& v) {
    if (m_field == v) return;
    m_field = v;
    emit fieldChanged();
    _scheduleSearch();
}

void SearchViewModel::setMinPrice(double v) {
    if (qFuzzyCompare(m_minPrice, v)) return;
    m_minPrice = v; emit filtersChanged(); _scheduleSearch();
}

void SearchViewModel::setMaxPrice(double v) {
    if (qFuzzyCompare(m_maxPrice, v)) return;
    m_maxPrice = v; emit filtersChanged(); _scheduleSearch();
}

void SearchViewModel::setSort(const QString& v) {
    if (m_sort == v) return;
    m_sort = v; emit sortChanged();
    if (!m_results.isEmpty()) _reSort();
}

void SearchViewModel::toggleGenre(const QString& genre) {
    int idx = m_selectedGenres.indexOf(genre);
    if (idx >= 0) m_selectedGenres.removeAt(idx);
    else          m_selectedGenres.append(genre);
    emit selectedGenresChanged();
    emit filtersChanged();
    _scheduleSearch();
}

void SearchViewModel::clearGenres() {
    if (m_selectedGenres.isEmpty()) return;
    m_selectedGenres.clear();
    emit selectedGenresChanged();
    emit filtersChanged();
    _scheduleSearch();
}

void SearchViewModel::clearFilters() {
    m_query.clear();
    m_field = QStringLiteral("all");
    m_selectedGenres.clear();
    m_minPrice = 0.0;
    m_maxPrice = 100.0;
    m_minRating = 0.0;
    m_sort = QStringLiteral("relevance");
    m_language = "any";
    m_publicationYear = 0;
    m_onlyDiscounted = false;
    m_onlyFree = false;
    m_onlyPaid = false;
    m_onlyDownloaded = false;
    m_onlyFavorite = false;
    m_availability = "all";
    emit queryChanged();
    emit fieldChanged();
    emit selectedGenresChanged();
    emit filtersChanged();
    emit minRatingChanged();
    emit sortChanged();
    _scheduleSearch();
}

void SearchViewModel::clearFilter(const QString& key) {
    if (key == "query")       { setQuery(""); }
    else if (key == "field")  { setField("all"); }
    else if (key == "genres") { clearGenres(); }
    else if (key == "minPrice") { setMinPrice(0.0); }
    else if (key == "maxPrice") { setMaxPrice(100.0); }
    else if (key == "minRating") { setMinRating(0.0); }
    else if (key == "language") { setLanguage("any"); }
    else if (key == "publicationYear") { setPublicationYear(0); }
    else if (key == "onlyDiscounted") { setOnlyDiscounted(false); }
    else if (key == "onlyFree") { setOnlyFree(false); }
    else if (key == "onlyPaid") { setOnlyPaid(false); }
    else if (key == "onlyDownloaded") { setOnlyDownloaded(false); }
    else if (key == "onlyFavorite") { setOnlyFavorite(false); }
    else if (key == "availability") { setAvailability("all"); }
}

bool SearchViewModel::isGenreSelected(const QString& genre) const {
    return m_selectedGenres.contains(genre);
}

void SearchViewModel::selectSuggestion(const QString& suggestion) {
    setQuery(suggestion);
    search();
}

void SearchViewModel::clearRecentSearches() {
    if (m_bookService) m_bookService->clearRecentSearches();
    emit searchHistoryChanged();
}

void SearchViewModel::search() {
    if (!m_bookService) return;
    m_hasSearched = true;
    _setSearching(true);
    // Record search in history (if non-empty)
    if (!m_query.trimmed().isEmpty()) {
        m_bookService->recordSearch(m_query);
        emit searchHistoryChanged();
    }
    beginAsync(300);
}

void SearchViewModel::onAsyncReady() {
    if (!m_bookService) { _setSearching(false); finishAsync(); return; }
    // Transfer ownership of old results to QML GC instead of qDeleteAll.
    for (auto* o : m_results) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_results.clear();

    // Apply price/free filter adjustments
    double minP = m_minPrice;
    double maxP = m_maxPrice;
    if (m_onlyFree) { minP = 0.0; maxP = 0.0; }
    else if (m_onlyPaid) { minP = qMax(minP, 0.01); }

    m_results = m_bookService->search(m_query, m_field, m_selectedGenres,
                                       minP, maxP, m_minRating);

    // Apply post-filters that BookService.search doesn't handle
    if (m_onlyDiscounted || m_onlyDownloaded || m_onlyFavorite || m_availability != "all" || m_language != "any" || m_publicationYear > 0) {
        QList<QObject*> filtered;
        for (auto* o : m_results) {
            auto b = qobject_cast<BookDto*>(o);
            if (!b) continue;
            if (m_onlyDiscounted && !b->hasDiscount()) { delete b; continue; }
            if (m_onlyDownloaded && !b->purchased()) { delete b; continue; }
            // BUG FIX (Issue 36): the "Favorites" filter was always empty
            // because `BookDto::inWishlist` is set at parse time from the
            // server's SearchBooks response — which doesn't include
            // wishlist membership. We now cross-reference each book
            // against `BookService::isInWishlist(b->id())` and update the
            // DTO's `inWishlist` flag before the filter check, so the
            // Favorites filter actually works.
            if (m_onlyFavorite) {
                if (m_bookService && !b->inWishlist()) {
                    b->setInWishlist(m_bookService->isInWishlist(b->id()));
                }
                if (!b->inWishlist()) { delete b; continue; }
            }
            // Availability filter: "all" = no filter, "inStock" = active books,
            // "outOfStock" = inactive books.
            if (m_availability == "inStock" && b->status() != "active") { delete b; continue; }
            if (m_availability == "outOfStock" && b->status() == "active") { delete b; continue; }
            filtered.append(b);
        }
        m_results = filtered;
    }

    _reSort();
    emit resultsChanged();
    _setSearching(false);
    finishAsync();
}

void SearchViewModel::_scheduleSearch(int delayMs) {
    // BUG FIX (Issue 35): previously this method ignored the `delayMs`
    // argument (Q_UNUSED + always used the timer's fixed 300ms interval).
    // Callers can now actually customize the debounce delay.
    m_debounceTimer.start(delayMs);
}

void SearchViewModel::_refreshSuggestions() {
    QStringList out;
    if (m_bookService && m_query.trimmed().length() >= 2) {
        const QString q = m_query.trimmed().toLower();
        // Pull from the catalog via search()
        auto results = m_bookService->search(m_query, "all", {}, 0, 1000, 0);
        for (auto* o : results) {
            auto b = qobject_cast<BookDto*>(o);
            if (b) {
                if (b->title().toLower().startsWith(q) && !out.contains(b->title())) out.append(b->title());
                if (b->authorName().toLower().startsWith(q) && !out.contains(b->authorName())) out.append(b->authorName());
            }
            delete o;
            if (out.size() >= 5) break;
        }
    }
    if (out != m_suggestions) {
        m_suggestions = out;
        emit suggestionsChanged();
    }
}

void SearchViewModel::_setSearching(bool v) {
    if (m_isSearching == v) return;
    m_isSearching = v;
    emit isSearchingChanged();
}

int SearchViewModel::activeFilterCount() const {
    int n = 0;
    if (!m_query.trimmed().isEmpty()) ++n;
    if (m_field != "all") ++n;
    n += m_selectedGenres.size();
    if (m_minPrice > 0.0) ++n;
    if (m_maxPrice < 100.0) ++n;
    if (m_minRating > 0.0) ++n;
    if (m_language != "any") ++n;
    if (m_publicationYear > 0) ++n;
    if (m_onlyDiscounted) ++n;
    if (m_onlyFree) ++n;
    if (m_onlyPaid) ++n;
    if (m_onlyDownloaded) ++n;
    if (m_onlyFavorite) ++n;
    if (m_availability != "all") ++n;
    return n;
}

QList<QObject*> SearchViewModel::activeFilters() const {
    QList<QObject*> out;
    if (!m_query.trimmed().isEmpty())
        out.append(new FilterChipDto("query", "Search", m_query, "search"));
    if (m_field != "all")
        out.append(new FilterChipDto("field", "Field", m_field, "filter_alt"));
    for (const auto& g : m_selectedGenres)
        out.append(new FilterChipDto("genres", "Genre", g, "tag"));
    if (m_minPrice > 0.0)
        out.append(new FilterChipDto("minPrice", "Min price", QString("$%1").arg(m_minPrice, 0, 'f', 2), "payments"));
    if (m_maxPrice < 100.0)
        out.append(new FilterChipDto("maxPrice", "Max price", QString("$%1").arg(m_maxPrice, 0, 'f', 2), "payments"));
    if (m_minRating > 0.0)
        out.append(new FilterChipDto("minRating", "Min rating", QString("%1★").arg(int(m_minRating)), "star"));
    if (m_language != "any")
        out.append(new FilterChipDto("language", "Language", m_language, "language"));
    if (m_publicationYear > 0)
        out.append(new FilterChipDto("publicationYear", "Year", QString::number(m_publicationYear), "calendar_today"));
    if (m_onlyDiscounted)
        out.append(new FilterChipDto("onlyDiscounted", "Only", "Discounted", "local_offer"));
    if (m_onlyFree)
        out.append(new FilterChipDto("onlyFree", "Only", "Free", "savings"));
    if (m_onlyPaid)
        out.append(new FilterChipDto("onlyPaid", "Only", "Paid", "payments"));
    if (m_onlyDownloaded)
        out.append(new FilterChipDto("onlyDownloaded", "Only", "Downloaded", "download"));
    if (m_onlyFavorite)
        out.append(new FilterChipDto("onlyFavorite", "Only", "Favorites", "favorite"));
    if (m_availability != "all")
        out.append(new FilterChipDto("availability", "Availability", m_availability, "inventory"));
    return out;
}

void SearchViewModel::_reSort() {
    std::sort(m_results.begin(), m_results.end(),
              [this](QObject* aObj, QObject* bObj) -> bool {
        auto a = qobject_cast<BookDto*>(aObj);
        auto b = qobject_cast<BookDto*>(bObj);
        if (!a || !b) return false;
        if (m_sort == QStringLiteral("price_asc"))  return a->price() < b->price();
        if (m_sort == QStringLiteral("price_desc")) return a->price() > b->price();
        if (m_sort == QStringLiteral("rating"))     return a->averageRating() > b->averageRating();
        if (m_sort == QStringLiteral("newest"))     return a->createdAt() > b->createdAt();
        if (m_sort == QStringLiteral("oldest"))     return a->createdAt() < b->createdAt();
        if (m_sort == QStringLiteral("popular"))    return a->ratingCount() > b->ratingCount();
        if (m_sort == QStringLiteral("alphabetical")) return a->title().toLower() < b->title().toLower();
        // relevance: rating * count
        return (a->averageRating() * a->ratingCount()) > (b->averageRating() * b->ratingCount());
    });
}

} // namespace bookclub::client
