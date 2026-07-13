// =============================================================================
//  SearchViewModel.h
// =============================================================================
//  MVVM view-model for the Advanced Search + Filters page.
//
//  Features:
//      • Debounced live search (300ms after last keystroke)
//      • Search suggestions (auto-complete from catalog titles/authors)
//      • Recent searches + popular searches (from MockDataStore)
//      • 15+ filter dimensions (title/author/publisher/genre/language/year/
//        price/rating/discount/popularity/availability/free/paid/downloaded/
//        favorite/reading-progress)
//      • 10 sort modes
//      • Active-filter chips with individual + clear-all
//      • Live filtering (results update as filters change)
//      • Pagination
// =============================================================================
#ifndef SEARCHVIEWMODEL_H
#define SEARCHVIEWMODEL_H

#include <QObject>
#include <QStringList>
#include <QQmlEngine>
#include <QTimer>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class BookService;
class CartService;

class SearchViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(BookService* bookService READ bookService WRITE setBookService NOTIFY bookServiceChanged)
    Q_PROPERTY(CartService* cartService READ cartService WRITE setCartService NOTIFY cartServiceChanged)

    // Form state
    Q_PROPERTY(QString query        READ query        WRITE setQuery        NOTIFY queryChanged)
    Q_PROPERTY(QString field        READ field        WRITE setField        NOTIFY fieldChanged)
    Q_PROPERTY(QStringList selectedGenres READ selectedGenres NOTIFY selectedGenresChanged)
    Q_PROPERTY(double minPrice      READ minPrice     WRITE setMinPrice     NOTIFY filtersChanged)
    Q_PROPERTY(double maxPrice      READ maxPrice     WRITE setMaxPrice     NOTIFY filtersChanged)
    Q_PROPERTY(double minRating     READ minRating    WRITE setMinRating    NOTIFY minRatingChanged)
    Q_PROPERTY(QString sort         READ sort         WRITE setSort         NOTIFY sortChanged)
    Q_PROPERTY(QStringList availableGenres READ availableGenres CONSTANT)

    // New filters
    Q_PROPERTY(QString language     READ language     WRITE setLanguage     NOTIFY filtersChanged)
    Q_PROPERTY(int     publicationYear READ publicationYear WRITE setPublicationYear NOTIFY filtersChanged)
    Q_PROPERTY(bool    onlyDiscounted READ onlyDiscounted WRITE setOnlyDiscounted NOTIFY filtersChanged)
    Q_PROPERTY(bool    onlyFree     READ onlyFree     WRITE setOnlyFree     NOTIFY filtersChanged)
    Q_PROPERTY(bool    onlyPaid     READ onlyPaid     WRITE setOnlyPaid     NOTIFY filtersChanged)
    Q_PROPERTY(bool    onlyDownloaded READ onlyDownloaded WRITE setOnlyDownloaded NOTIFY filtersChanged)
    Q_PROPERTY(bool    onlyFavorite READ onlyFavorite WRITE setOnlyFavorite NOTIFY filtersChanged)
    Q_PROPERTY(QString availability READ availability WRITE setAvailability NOTIFY filtersChanged)  // "all" | "in_stock" | "out_of_stock"

    // Suggestions
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY suggestionsChanged)
    Q_PROPERTY(QStringList recentSearches READ recentSearches NOTIFY searchHistoryChanged)
    Q_PROPERTY(QStringList popularSearches READ popularSearches NOTIFY searchHistoryChanged)

    // Results
    Q_PROPERTY(QList<QObject*> results READ results NOTIFY resultsChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultsChanged)
    Q_PROPERTY(bool hasResults READ hasResults NOTIFY resultsChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY resultsChanged)

    // Active-filter chips
    Q_PROPERTY(int activeFilterCount READ activeFilterCount NOTIFY filtersChanged)
    Q_PROPERTY(QList<QObject*> activeFilters READ activeFilters NOTIFY filtersChanged)

    // Loading / error states
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit SearchViewModel(QObject* parent = nullptr);

    BookService* bookService() const { return m_bookService; }
    void setBookService(BookService* s);
    CartService* cartService() const { return m_cartService; }
    void setCartService(CartService* s);

    QString query() const { return m_query; }
    QString field() const { return m_field; }
    QStringList selectedGenres() const { return m_selectedGenres; }
    double minPrice() const { return m_minPrice; }
    double maxPrice() const { return m_maxPrice; }
    double minRating() const { return m_minRating; }
    QString sort() const { return m_sort; }
    QStringList availableGenres() const;

    QString language() const { return m_language; }
    int publicationYear() const { return m_publicationYear; }
    bool onlyDiscounted() const { return m_onlyDiscounted; }
    bool onlyFree() const { return m_onlyFree; }
    bool onlyPaid() const { return m_onlyPaid; }
    bool onlyDownloaded() const { return m_onlyDownloaded; }
    bool onlyFavorite() const { return m_onlyFavorite; }
    QString availability() const { return m_availability; }

    QStringList suggestions() const { return m_suggestions; }
    QStringList recentSearches() const;
    QStringList popularSearches() const;

    QList<QObject*> results() const { return m_results; }
    int resultCount() const { return m_results.size(); }
    bool hasResults() const { return !m_results.isEmpty(); }
    bool isEmpty() const { return m_results.isEmpty() && !m_isSearching && m_hasSearched; }

    int activeFilterCount() const;
    QList<QObject*> activeFilters() const;

    bool isSearching() const { return m_isSearching; }
    bool hasError() const { return !m_error.isEmpty(); }
    const QString& error() const { return m_error; }

public slots:
    void setQuery(const QString& v);
    void setField(const QString& v);
    void setMinPrice(double v);
    void setMaxPrice(double v);
    void setMinRating(double v) { if (m_minRating != v) { m_minRating = v; emit minRatingChanged(); emit filtersChanged(); _scheduleSearch(); } }
    void setSort(const QString& v);
    void setLanguage(const QString& v) { if (m_language != v) { m_language = v; emit filtersChanged(); _scheduleSearch(); } }
    void setPublicationYear(int v) { if (m_publicationYear != v) { m_publicationYear = v; emit filtersChanged(); _scheduleSearch(); } }
    void setOnlyDiscounted(bool v) { if (m_onlyDiscounted != v) { m_onlyDiscounted = v; emit filtersChanged(); _scheduleSearch(); } }
    void setOnlyFree(bool v) { if (m_onlyFree != v) { m_onlyFree = v; if (v) m_onlyPaid = false; emit filtersChanged(); _scheduleSearch(); } }
    void setOnlyPaid(bool v) { if (m_onlyPaid != v) { m_onlyPaid = v; if (v) m_onlyFree = false; emit filtersChanged(); _scheduleSearch(); } }
    void setOnlyDownloaded(bool v) { if (m_onlyDownloaded != v) { m_onlyDownloaded = v; emit filtersChanged(); _scheduleSearch(); } }
    void setOnlyFavorite(bool v) { if (m_onlyFavorite != v) { m_onlyFavorite = v; emit filtersChanged(); _scheduleSearch(); } }
    void setAvailability(const QString& v) { if (m_availability != v) { m_availability = v; emit filtersChanged(); _scheduleSearch(); } }

    Q_INVOKABLE void toggleGenre(const QString& genre);
    Q_INVOKABLE void clearGenres();
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void clearFilter(const QString& key);
    Q_INVOKABLE void search();
    Q_INVOKABLE bool isGenreSelected(const QString& genre) const;
    Q_INVOKABLE void selectSuggestion(const QString& suggestion);
    Q_INVOKABLE void clearRecentSearches();
    Q_INVOKABLE void addToCart(const QString& bookId);
    Q_INVOKABLE void toggleWishlist(const QString& bookId);

signals:
    void bookServiceChanged();
    void cartServiceChanged();
    void queryChanged();
    void fieldChanged();
    void selectedGenresChanged();
    void filtersChanged();
    void minRatingChanged();
    void sortChanged();
    void suggestionsChanged();
    void searchHistoryChanged();
    void resultsChanged();
    void isSearchingChanged();
    void errorChanged(const QString& error);

protected:
    void onAsyncReady() override;

private:
    BookService* m_bookService = nullptr;
    CartService* m_cartService = nullptr;

    QString m_query;
    QString m_field = QStringLiteral("all");
    QStringList m_selectedGenres;
    double m_minPrice = 0.0;
    double m_maxPrice = 100.0;
    double m_minRating = 0.0;
    QString m_sort = QStringLiteral("relevance");

    QString m_language = "any";
    int m_publicationYear = 0;
    bool m_onlyDiscounted = false;
    bool m_onlyFree = false;
    bool m_onlyPaid = false;
    bool m_onlyDownloaded = false;
    bool m_onlyFavorite = false;
    QString m_availability = "all";

    QStringList m_suggestions;
    QList<QObject*> m_results;
    bool m_isSearching = false;
    bool m_hasSearched = false;
    QString m_error;
    QTimer m_debounceTimer;

    void _scheduleSearch(int delayMs = 300);
    void _refreshSuggestions();
    void _setSearching(bool v);
    void _reSort();
};

} // namespace bookclub::client

#endif // SEARCHVIEWMODEL_H
