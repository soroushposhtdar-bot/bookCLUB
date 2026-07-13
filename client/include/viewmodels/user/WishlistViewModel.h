// =============================================================================
//  WishlistViewModel.h
// =============================================================================
//  Dedicated wishlist page VM. Distinct from the Library "Saved" tab — this
//  page supports grid/list toggle, sort, filter, bulk-select, move-to-cart.
// =============================================================================
#ifndef WISHLISTVIEWMODEL_H
#define WISHLISTVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class LibraryService;
class CartService;
class BookDto;

class WishlistViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(LibraryService* libraryService READ libraryService WRITE setLibraryService NOTIFY libraryServiceChanged)
    Q_PROPERTY(CartService* cartService READ cartService WRITE setCartService NOTIFY cartServiceChanged)

    Q_PROPERTY(QList<QObject*> books READ books NOTIFY booksChanged)
    Q_PROPERTY(int count READ count NOTIFY booksChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY booksChanged)

    // Aggregate stats (spec-required) — computed from the wishlist books.
    // totalValueText: "$123.45" — sum of effective prices.
    // discountedCount: number of books currently on sale.
    // maxDiscountPercent: highest discount % across the wishlist.
    // maxDiscountBookId: id of the book with the biggest saving ("" if none).
    Q_PROPERTY(QString totalValueText READ totalValueText NOTIFY booksChanged)
    Q_PROPERTY(int discountedCount READ discountedCount NOTIFY booksChanged)
    Q_PROPERTY(int maxDiscountPercent READ maxDiscountPercent NOTIFY booksChanged)
    Q_PROPERTY(QString maxDiscountBookId READ maxDiscountBookId NOTIFY booksChanged)
    Q_PROPERTY(QString maxDiscountBookTitle READ maxDiscountBookTitle NOTIFY booksChanged)

    // View state
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)     // "grid" | "list"
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)     // "recent" | "title" | "price_asc" | "price_desc" | "rating"
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

    // Bulk select
    Q_PROPERTY(bool bulkMode READ bulkMode WRITE setBulkMode NOTIFY bulkModeChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)

public:
    explicit WishlistViewModel(QObject* parent = nullptr);

    LibraryService* libraryService() const { return m_libraryService; }
    CartService* cartService() const { return m_cartService; }
    void setLibraryService(LibraryService* s);
    void setCartService(CartService* s);

    QList<QObject*> books() const;
    int count() const;
    bool isEmpty() const { return count() == 0; }

    // Aggregate stats
    QString totalValueText() const;
    int discountedCount() const;
    int maxDiscountPercent() const;
    QString maxDiscountBookId() const;
    QString maxDiscountBookTitle() const;

    QString viewMode() const { return m_viewMode; }
    QString sortMode() const { return m_sortMode; }
    QString searchQuery() const { return m_searchQuery; }
    void setViewMode(const QString& v);
    void setSortMode(const QString& v);
    void setSearchQuery(const QString& v);

    bool bulkMode() const { return m_bulkMode; }
    void setBulkMode(bool v);
    int selectedCount() const { return m_selected.size(); }

    Q_INVOKABLE bool isSelected(const QString& bookId) const { return m_selected.contains(bookId); }
    Q_INVOKABLE void toggleSelected(const QString& bookId);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void remove(const QString& bookId);
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void moveToCart(const QString& bookId);
    Q_INVOKABLE void moveSelectedToCart();

signals:
    void libraryServiceChanged();
    void cartServiceChanged();
    void booksChanged();
    void viewModeChanged();
    void sortModeChanged();
    void searchQueryChanged();
    void bulkModeChanged();
    void selectionChanged();

private:
    LibraryService* m_libraryService = nullptr;
    CartService* m_cartService = nullptr;
    QString m_viewMode = "grid";
    QString m_sortMode = "recent";
    QString m_searchQuery;
    bool m_bulkMode = false;
    QStringList m_selected;

    QList<QObject*> _filteredSorted() const;
};

} // namespace bookclub::client

#endif // WISHLISTVIEWMODEL_H
