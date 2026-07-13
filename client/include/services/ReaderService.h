// =============================================================================
//  ReaderService.h
// =============================================================================
//  Mocked PDF reader service for the Regular User role.
//
//  Responsibilities:
//      • Tracks the last-read page per book so the user resumes where they
//        left off ( REQ_READER_LASTPAGE / RES_READER_LASTPAGE ).
//      • Exposes the PDF path for a given book (the mock ships a tiny
//        synthetic "PDF" rendered via QML — see PdfReaderPage.qml — so the
//        reader is fully functional without bundling a real PDF library).
//      • Records reading-progress events that, in production, would be
//        broadcast to the group-reading session via SyncEventType.
//
//  Real-backend mapping (see common/Network/Protocol.h + AppEnums.h):
//      openBook(bookId)        → REQ_READER_OPEN      → RES_READER_OPEN
//      savePage(bookId, page)  → REQ_READER_SAVE_PAGE → RES_OK
//      lastPage(bookId)        → (cached locally + synced on close)
//      SyncEventType::TurnPage, JumpToPage, ZoomChanged, LastPageSaved →
//          broadcast over the group-reading session socket.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QQmlEngine>

#include "services/MockDataStore.h"

namespace bookclub::client {

class ReaderService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString currentBookId    READ currentBookId    NOTIFY currentBookChanged)
    Q_PROPERTY(QString currentBookTitle READ currentBookTitle NOTIFY currentBookChanged)
    Q_PROPERTY(int     currentPage      READ currentPage      NOTIFY currentPageChanged)
    Q_PROPERTY(int     pageCount        READ pageCount        NOTIFY currentBookChanged)
    Q_PROPERTY(bool    hasOpenBook      READ hasOpenBook      NOTIFY currentBookChanged)
    Q_PROPERTY(QStringList tableOfContents READ tableOfContents NOTIFY currentBookChanged)
    Q_PROPERTY(QVariantList bookmarks   READ bookmarks        NOTIFY bookmarksChanged)

public:
    explicit ReaderService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    QString currentBookId() const { return m_currentBookId; }
    QString currentBookTitle() const { return m_currentTitle; }
    int currentPage() const { return m_currentPage; }
    int pageCount() const { return m_pageCount; }
    bool hasOpenBook() const { return !m_currentBookId.isEmpty(); }
    QStringList tableOfContents() const { return m_toc; }
    QVariantList bookmarks() const;

    // ----- Q_INVOKABLE actions -----
    Q_INVOKABLE bool openBook(const QString& bookId);
    Q_INVOKABLE void closeBook();
    Q_INVOKABLE void setPage(int page);
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE int  lastReadPage(const QString& bookId) const;

    // Bookmarks
    Q_INVOKABLE bool isBookmarked(int page) const;
    Q_INVOKABLE void toggleBookmark(int page);
    Q_INVOKABLE void clearBookmarks();

signals:
    void currentBookChanged();
    void currentPageChanged();
    void pageSaved(const QString& bookId, int page);
    void bookOpened(const QString& bookId);
    void bookClosed();
    void bookmarksChanged();

private:
    MockDataStore* m_store = nullptr;
    QString m_currentBookId;
    QString m_currentTitle;
    int m_currentPage = 1;
    int m_pageCount = 0;
    QStringList m_toc;
    QHash<QString, int> m_lastPages;  // bookId → page
    QHash<QString, QList<int>> m_bookmarks;  // bookId → sorted unique pages

    void _rebuildToc(const MockBook& b);
};

} // namespace bookclub::client
