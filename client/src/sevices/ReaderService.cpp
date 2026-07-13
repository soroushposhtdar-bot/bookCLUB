// =============================================================================
//  ReaderService.cpp
// =============================================================================
#include "services/ReaderService.h"

#include <QVariantList>

namespace bookclub::client {

ReaderService::ReaderService(QObject* parent)
    : QObject(parent)
{}

void ReaderService::setDataStore(MockDataStore* store) {
    m_store = store;
}

bool ReaderService::openBook(const QString& bookId) {
    if (!m_store) return false;
    MockBook b = m_store->bookById(bookId);
    if (b.id.isEmpty()) return false;

    m_currentBookId = b.id;
    m_currentTitle = b.title;
    // Synthesize a plausible page count from the description length so the
    // mock reader has something to render. Real build: PDF page count.
    m_pageCount = qMax(8, b.description.length() / 30);
    _rebuildToc(b);

    auto it = m_lastPages.constFind(bookId);
    m_currentPage = (it != m_lastPages.constEnd()) ? it.value() : 1;

    // Opening a book in the reader counts as "recently viewed" — push it to
    // the store so HomePage's "Recently viewed" carousel reflects real usage.
    m_store->markRecentlyViewed(bookId);

    emit currentBookChanged();
    emit currentPageChanged();
    emit bookmarksChanged();
    emit bookOpened(bookId);
    return true;
}

void ReaderService::closeBook() {
    if (!m_currentBookId.isEmpty()) {
        m_lastPages.insert(m_currentBookId, m_currentPage);
        emit pageSaved(m_currentBookId, m_currentPage);
    }
    m_currentBookId.clear();
    m_currentTitle.clear();
    m_currentPage = 1;
    m_pageCount = 0;
    emit currentBookChanged();
    emit currentPageChanged();
    emit bookClosed();
}

void ReaderService::setPage(int page) {
    if (m_pageCount <= 0) return;
    page = qBound(1, page, m_pageCount);
    if (page == m_currentPage) return;
    m_currentPage = page;
    emit currentPageChanged();
    m_lastPages.insert(m_currentBookId, page);
    // Persist to the shared store so HomePage "Continue reading" + the
    // BookDetailPage progress indicator reflect real reader state across
    // app restarts. (MockDataStore::upsertReadingProgress is in-memory but
    // shared across every service that reads from the store.)
    if (m_store && !m_currentBookId.isEmpty()) {
        m_store->upsertReadingProgress(m_currentBookId, page, m_pageCount);
    }
    emit pageSaved(m_currentBookId, page);
}

void ReaderService::nextPage()     { setPage(m_currentPage + 1); }
void ReaderService::prevPage()     { setPage(m_currentPage - 1); }
void ReaderService::firstPage()    { setPage(1); }
void ReaderService::lastPage()     { setPage(m_pageCount); }

int ReaderService::lastReadPage(const QString& bookId) const {
    auto it = m_lastPages.constFind(bookId);
    return it != m_lastPages.constEnd() ? it.value() : 1;
}

// -----------------------------------------------------------------------------
//  Bookmarks
// -----------------------------------------------------------------------------

QVariantList ReaderService::bookmarks() const {
    QVariantList out;
    if (m_currentBookId.isEmpty()) return out;
    auto it = m_bookmarks.constFind(m_currentBookId);
    if (it == m_bookmarks.constEnd()) return out;
    for (int p : it.value()) out.append(p);
    return out;
}

bool ReaderService::isBookmarked(int page) const {
    if (m_currentBookId.isEmpty()) return false;
    auto it = m_bookmarks.constFind(m_currentBookId);
    if (it == m_bookmarks.constEnd()) return false;
    return it.value().contains(page);
}

void ReaderService::toggleBookmark(int page) {
    if (m_currentBookId.isEmpty() || page < 1 || page > m_pageCount) return;
    auto& list = m_bookmarks[m_currentBookId];
    int idx = list.indexOf(page);
    if (idx >= 0) {
        list.removeAt(idx);
    } else {
        list.append(page);
        std::sort(list.begin(), list.end());
    }
    emit bookmarksChanged();
}

void ReaderService::clearBookmarks() {
    if (m_currentBookId.isEmpty()) return;
    m_bookmarks.remove(m_currentBookId);
    emit bookmarksChanged();
}

// -----------------------------------------------------------------------------
//  Internal helpers
// -----------------------------------------------------------------------------

void ReaderService::_rebuildToc(const MockBook& b) {
    m_toc.clear();
    // Synthesize a 4–8 chapter TOC from the title and description so the
    // sidebar has real entries to render. Real build: parse PDF outline.
    const int chapterCount = qBound(4, m_pageCount / 4, 8);
    for (int i = 1; i <= chapterCount; ++i) {
        m_toc.append(QStringLiteral("Chapter %1 — %2")
                         .arg(i)
                         .arg(b.title));
    }
}

} // namespace bookclub::client
