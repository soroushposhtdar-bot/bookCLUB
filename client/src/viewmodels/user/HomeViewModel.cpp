// =============================================================================
//  HomeViewModel.cpp
// =============================================================================
#include "viewmodels/user/HomeViewModel.h"
#include "services/BookService.h"
#include "services/UserService.h"
#include "services/BookDto.h"

#include <QTime>
#include <QQmlEngine>

namespace bookclub::client {

HomeViewModel::HomeViewModel(QObject* parent)
    : UserViewModelBase(parent)
{
    m_wave2Timer.setSingleShot(true);
    connect(&m_wave2Timer, &QTimer::timeout, this, [this](){ _loadWave2(); });
}

void HomeViewModel::setBookService(BookService* s) {
    if (m_bookService == s) return;
    if (m_bookService) disconnect(m_bookService, nullptr, this, nullptr);
    m_bookService = s;
    if (m_bookService) {
        connect(m_bookService, &BookService::booksChanged, this, [this](){ refresh(); });
        // BUG FIX: listen to wishlistItemChanged so the BookCard hearts on
        // the Home page update in real-time when the user toggles wishlist
        // from any page. Without this, the heart icon stays in its old
        // state until the user navigates away and back.
        connect(m_bookService, &BookService::wishlistItemChanged, this, [this](const QString& bookId, bool inWishlist) {
            // Walk every cached book list and update the matching DTO's
            // inWishlist property so QML bindings re-evaluate.
            auto updateList = [bookId, inWishlist](QList<QObject*>& list) {
                for (auto* o : list) {
                    auto* b = qobject_cast<BookDto*>(o);
                    if (b && b->id() == bookId) {
                        b->setInWishlist(inWishlist);
                    }
                }
            };
            updateList(m_continueReading);
            updateList(m_recommended);
            updateList(m_newReleases);
            updateList(m_bestsellers);
            updateList(m_freeBooks);
            updateList(m_trending);
            updateList(m_editorsPicks);
            updateList(m_discounted);
            updateList(m_recentlyViewed);
            updateList(m_newArrivals);
            updateList(m_becauseYouRead);
            // Force a re-render by emitting dataChanged.
            emit dataChanged();
        });
    }
    emit bookServiceChanged();
    refresh();
}

void HomeViewModel::setUserService(UserService* s) {
    if (m_userService == s) return;
    m_userService = s;
    emit userServiceChanged();
    emit dataChanged();
}

QString HomeViewModel::greeting() const {
    if (!m_userService) return QStringLiteral("Welcome back");
    const int h = QTime::currentTime().hour();
    QString part = QStringLiteral("Good evening");
    if (h < 12)      part = QStringLiteral("Good morning");
    else if (h < 18) part = QStringLiteral("Good afternoon");
    return part + QStringLiteral(", ") + m_userService->displayName() + QStringLiteral(".");
}

void HomeViewModel::refresh() {
    if (!m_bookService) return;
    // BUG FIX (Issue 38): guard against re-entrant refresh. `setBookService`
    // connects `BookService::booksChanged → refresh()`. If a real-time
    // `EvtBookAdded` fires during the 500ms wave-2 window, `booksChanged`
    // → `refresh()` → another `_loadWave1` (which `qDeleteAll`s the
    // previous DTOs while QML may still be binding to them) and restarts
    // `m_wave2Timer`. Re-entrant refreshes can crash the UI. We now bail
    // out if a refresh is already in flight.
    if (m_refreshing) return;
    m_refreshing = true;
    _loadWave1();
    // Defer wave 2 by 500ms so the user sees above-the-fold content first.
    m_wave2Timer.start(500);
}

void HomeViewModel::_loadWave1() {
    if (!m_bookService) return;
    _setLoadingWave1(true);

    // BUG FIX: previously `qDeleteAll`'d the old DTOs before assigning new
    // ones. If QML still holds references to the old DTOs (e.g. a Repeater
    // delegate that hasn't been destroyed yet), the delete causes a use-
    // after-free. We now transfer ownership of the old DTOs to QML's GC
    // (JavaScriptOwnership) and just clear our list — QML's GC will clean
    // them up when no QML references remain.
    for (auto* o : m_continueReading) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_recommended)     QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_newReleases)     QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_continueReading.clear();
    m_recommended.clear();
    m_newReleases.clear();

    m_continueReading = m_bookService->continueReading();
    m_recommended     = m_bookService->recommended();
    m_newReleases     = m_bookService->newReleases();

    emit dataChanged();
    _setLoadingWave1(false);
}

void HomeViewModel::_loadWave2() {
    if (!m_bookService) return;
    _setLoadingWave2(true);

    // BUG FIX: same as _loadWave1 — transfer old DTOs to QML GC instead
    // of qDeleteAll (prevents use-after-free when QML still references them).
    for (auto* o : m_bestsellers)     QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_freeBooks)       QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_trending)        QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_editorsPicks)    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_discounted)      QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_recentlyViewed)  QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_newArrivals)     QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    for (auto* o : m_becauseYouRead)  QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_bestsellers.clear();
    m_freeBooks.clear();
    m_trending.clear();
    m_editorsPicks.clear();
    m_discounted.clear();
    m_recentlyViewed.clear();
    m_newArrivals.clear();
    m_becauseYouRead.clear();

    m_bestsellers      = m_bookService->bestsellers();
    m_freeBooks        = m_bookService->freeBooks();
    m_trending         = m_bookService->trending();
    m_editorsPicks     = m_bookService->editorsPicks();
    m_discounted       = m_bookService->discounted();
    m_recentlyViewed   = m_bookService->recentlyViewed();
    m_newArrivals      = m_bookService->newArrivals();
    // Because-you-read uses the first recommended book as the seed.
    if (!m_recommended.isEmpty()) {
        auto seed = qobject_cast<BookDto*>(m_recommended.first());
        if (seed) m_becauseYouRead = m_bookService->becauseYouRead(seed->id());
    }

    m_popularGenres    = m_bookService->availableGenres().mid(0, 8);
    m_featuredPublishers = m_bookService->featuredPublishers();

    emit dataChanged();
    _setLoadingWave2(false);
    // BUG FIX (Issue 38): clear the re-entrancy guard now that the
    // full refresh (wave1 + wave2) has completed.
    m_refreshing = false;
}

void HomeViewModel::_setLoadingWave1(bool v) {
    if (m_loadingWave1 == v) return;
    m_loadingWave1 = v;
    emit loadingWave1Changed(v);
}

void HomeViewModel::_setLoadingWave2(bool v) {
    if (m_loadingWave2 == v) return;
    m_loadingWave2 = v;
    emit loadingWave2Changed(v);
}

void HomeViewModel::toggleWishlist(const QString& bookId) {
    if (!m_bookService) return;
    m_bookService->toggleWishlist(bookId);
}

} // namespace bookclub::client
