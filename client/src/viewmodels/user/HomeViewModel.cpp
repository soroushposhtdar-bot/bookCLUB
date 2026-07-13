// =============================================================================
//  HomeViewModel.cpp
// =============================================================================
#include "viewmodels/user/HomeViewModel.h"
#include "services/BookService.h"
#include "services/UserService.h"
#include "services/BookDto.h"

#include <QTime>

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
    _loadWave1();
    // Defer wave 2 by 500ms so the user sees above-the-fold content first.
    m_wave2Timer.start(500);
}

void HomeViewModel::_loadWave1() {
    if (!m_bookService) return;
    _setLoadingWave1(true);

    // Free the previous DTOs (they were created with no parent).
    qDeleteAll(m_continueReading); m_continueReading.clear();
    qDeleteAll(m_recommended);     m_recommended.clear();
    qDeleteAll(m_newReleases);     m_newReleases.clear();

    m_continueReading = m_bookService->continueReading();
    m_recommended     = m_bookService->recommended();
    m_newReleases     = m_bookService->newReleases();

    emit dataChanged();
    _setLoadingWave1(false);
}

void HomeViewModel::_loadWave2() {
    if (!m_bookService) return;
    _setLoadingWave2(true);

    qDeleteAll(m_bestsellers);     m_bestsellers.clear();
    qDeleteAll(m_freeBooks);       m_freeBooks.clear();
    qDeleteAll(m_trending);        m_trending.clear();
    qDeleteAll(m_editorsPicks);    m_editorsPicks.clear();
    qDeleteAll(m_discounted);      m_discounted.clear();
    qDeleteAll(m_recentlyViewed);  m_recentlyViewed.clear();
    qDeleteAll(m_newArrivals);     m_newArrivals.clear();
    qDeleteAll(m_becauseYouRead);  m_becauseYouRead.clear();

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
