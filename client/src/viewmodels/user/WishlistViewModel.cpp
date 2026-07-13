// =============================================================================
//  WishlistViewModel.cpp
// =============================================================================
#include "viewmodels/user/WishlistViewModel.h"
#include "services/LibraryService.h"
#include "services/CartService.h"
#include "services/BookDto.h"

#include <QLocale>
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
        connect(m_libraryService, &LibraryService::wishlistChanged, this, &WishlistViewModel::booksChanged);
    }
    emit libraryServiceChanged();
    emit booksChanged();
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
//   These walk the full wishlist (not the filtered/sorted view) so the
//   summary card always reflects every saved book, regardless of the
//   active search query or sort mode.

QString WishlistViewModel::totalValueText() const {
    if (!m_libraryService) return QStringLiteral("$0.00");
    double total = 0.0;
    for (auto* o : m_libraryService->savedBooks()) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (dto) total += dto->price();
        delete o;
    }
    return QStringLiteral("$%1").arg(total, 0, 'f', 2);
}

int WishlistViewModel::discountedCount() const {
    if (!m_libraryService) return 0;
    int n = 0;
    for (auto* o : m_libraryService->savedBooks()) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (dto && dto->hasDiscount()) ++n;
        delete o;
    }
    return n;
}

int WishlistViewModel::maxDiscountPercent() const {
    if (!m_libraryService) return 0;
    int maxPct = 0;
    for (auto* o : m_libraryService->savedBooks()) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (dto && dto->hasDiscount()) {
            const int pct = dto->discountPercent();
            if (pct > maxPct) maxPct = pct;
        }
        delete o;
    }
    return maxPct;
}

QString WishlistViewModel::maxDiscountBookId() const {
    if (!m_libraryService) return {};
    int maxPct = 0;
    QString id;
    for (auto* o : m_libraryService->savedBooks()) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (dto && dto->hasDiscount()) {
            const int pct = dto->discountPercent();
            if (pct > maxPct) { maxPct = pct; id = dto->id(); }
        }
        delete o;
    }
    return id;
}

QString WishlistViewModel::maxDiscountBookTitle() const {
    if (!m_libraryService) return {};
    int maxPct = 0;
    QString title;
    for (auto* o : m_libraryService->savedBooks()) {
        auto* dto = qobject_cast<BookDto*>(o);
        if (dto && dto->hasDiscount()) {
            const int pct = dto->discountPercent();
            if (pct > maxPct) { maxPct = pct; title = dto->title(); }
        }
        delete o;
    }
    return title;
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
    if (m_libraryService) m_libraryService->toggleSaved(bookId);
    m_selected.removeAll(bookId);
    emit selectionChanged();
}

void WishlistViewModel::removeSelected() {
    if (m_selected.isEmpty() || !m_libraryService) return;
    QStringList snap = m_selected;
    for (const auto& id : snap) m_libraryService->toggleSaved(id);
    m_selected.clear();
    emit selectionChanged();
}

void WishlistViewModel::moveToCart(const QString& bookId) {
    if (m_cartService) m_cartService->add(bookId);
    if (m_libraryService) m_libraryService->toggleSaved(bookId);
    m_selected.removeAll(bookId);
    emit selectionChanged();
}

void WishlistViewModel::moveSelectedToCart() {
    if (m_selected.isEmpty() || !m_cartService || !m_libraryService) return;
    QStringList snap = m_selected;
    for (const auto& id : snap) {
        m_cartService->add(id);
        m_libraryService->toggleSaved(id);
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
            if (!b) { delete o; continue; }
            if (b->title().toLower().contains(q) ||
                b->authorName().toLower().contains(q)) {
                filtered.append(b);
            } else {
                delete b;
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
