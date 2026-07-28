#include "services/ReaderService.h"
#include "services/NetworkService.h"
#include "services/BookDto.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

namespace bookclub::client {

ReaderService::ReaderService(QObject* parent) : QObject(parent)
{
    // Real-time group-reading sync — when another participant turns a page
    // in the same study session, the server pushes EvtStudySync. We update
    // the current page so the reader UI follows along.
    NetworkService::instance().subscribeEvent(common::Command::EvtStudySync, this,
        [this](const common::Message& msg) {
            const int page = msg.payload().value("page").toInt(-1);
            if (page < 0) return;
            // Only follow if the message is for the currently-open book.
            const QString bookId = msg.payload().value("bookId").toString();
            if (!m_currentBookId.isEmpty() && bookId == m_currentBookId) {
                setPage(page);
            }
        });
}

// -----------------------------------------------------------------------------
//  Local PDF cache path.
//
//  The reader needs a *local* file path for QtQuick.Pdf's PdfDocument.source.
//  The server stores PDFs under <server-cwd>/storage/pdfs/<bookId>.pdf and
//  only puts the relative string ("pdfs/<bookId>.pdf") in the DB. When the
//  client opens a book, we download the PDF bytes via the DownloadBookPdf
//  command and write them to <AppDataLocation>/pdf_cache/<bookId>.pdf.
//
//  This cache survives across sessions so we don't re-download every time.
// -----------------------------------------------------------------------------
QString ReaderService::localPdfCachePath(const QString& bookId) const {
    if (bookId.isEmpty()) return {};
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) return {};
    QDir().mkpath(base + "/pdf_cache");
    return base + "/pdf_cache/" + bookId + ".pdf";
}

bool ReaderService::openBook(const QString& bookId) {
    // Fetch book details to get pdfFilePath
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBookDetails, p);
    if (!resp.isSuccess()) return false;

    m_currentBookId = bookId;
    m_currentTitle = resp.payload.value("title").toString();
    m_pdfPath.clear();
    // v15j: populate cover metadata from the book details response.
    m_coverColor = resp.payload.value("coverColor").toString("#1A73E8");
    m_coverAccent = resp.payload.value("coverAccent").toString("#F39C12");
    m_coverImage = resp.payload.value("coverImagePath").toString();
    if (m_coverImage.isEmpty()) m_coverImage = resp.payload.value("coverImage").toString();
    m_authorName = resp.payload.value("authorName").toString();

    // v13: Download the actual PDF bytes from the server and cache locally.
    // The server's pdfFilePath is a relative string like "pdfs/<bookId>.pdf"
    // — useless on the client. We need a real local file path for
    // QtQuick.Pdf's PdfDocument.source. The cache survives across sessions.
    QJsonObject dl;
    dl["bookId"] = bookId;
    auto dlResp = NetworkService::instance().sendRequest(common::Command::DownloadBookPdf, dl,
                                                         /*timeoutMs=*/30000);
    if (dlResp.isSuccess() && dlResp.payload.contains("fileData")) {
        const QByteArray raw = QByteArray::fromBase64(
            dlResp.payload.value("fileData").toString().toLatin1());
        if (!raw.isEmpty()) {
            const QString cachePath = localPdfCachePath(bookId);
            if (!cachePath.isEmpty()) {
                QFile f(cachePath);
                if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    f.write(raw);
                    f.close();
                    m_pdfPath = cachePath;
                }
            }
        }
    }

    // Fallback: if the download failed but the server's pdfFilePath happens
    // to be a readable local file (e.g. dev environment where publisher and
    // user share a filesystem), use that path directly. Otherwise m_pdfPath
    // stays empty and the reader falls back to the synthetic text view.
    if (m_pdfPath.isEmpty()) {
        const QString serverPath = resp.payload.value("pdfFilePath").toString();
        if (!serverPath.isEmpty()) {
            QFileInfo info(serverPath);
            if (info.isFile() && info.isReadable()) {
                m_pdfPath = serverPath;
            }
        }
    }

    // Page count: start at 0 (unknown). The QML will push the real
    // page count via setPageCount() once the PdfDocument finishes
    // loading. This replaces the old hardcoded "100" which broke
    // page sync for any PDF with a different page count.
    m_pageCount = 0;
    // BUG FIX (Issue 5 + Issue 39): initialize `m_currentPage` to 1, not 0.
    // The QML reader displays 1-based page numbers everywhere (page grid
    // sends `goToPage(index + 1)`, indicator shows "page / pageCount",
    // prev/next buttons check `page > 1` / `page < pageCount`). With 0
    // as the initial value:
    //   - The indicator showed "0 / 100" instead of "1 / 100".
    //   - `setPage(100)` from the grid failed (100 >= 100).
    //   - `ReaderViewModel::pageText()` crashed via `(p-1) % openers.size()`
    //     with p=0 → -1 % 5 = -1 → openers.at(-1) → UB / crash.
    //
    // Issue 2: restore the last-read page for this book so the user
    // resumes where they left off. lastReadPage() defaults to 1 when
    // the book has never been opened.
    m_currentPage = lastReadPage(bookId);

    // Build a simple table of contents (used only for the synthetic-text
    // fallback when no real PDF is available).
    m_toc.clear();
    for (int i = 1; i <= 10; ++i) {
        m_toc.append(QStringLiteral("Chapter %1").arg(i));
    }

    emit currentBookChanged();
    emit bookOpened(bookId);
    return true;
}

void ReaderService::closeBook() {
    if (!m_currentBookId.isEmpty()) {
        // Save current page
        QJsonObject p;
        p["bookId"] = m_currentBookId;
        p["page"] = m_currentPage;
        NetworkService::instance().sendAsync(common::Command::SyncStudyPage, p);
    }
    m_currentBookId.clear();
    m_currentTitle.clear();
    m_pdfPath.clear();
    // v15j: clear cover metadata.
    m_coverColor.clear();
    m_coverAccent.clear();
    m_coverImage.clear();
    m_authorName.clear();
    // BUG FIX (Issue 5 + Issue 39): reset to 1 (1-based), not 0.
    m_currentPage = 1;
    m_pageCount = 0;
    m_toc.clear();
    emit currentBookChanged();
    emit bookClosed();
}

// BUG FIX (Issue 39): switch to 1-based page indexing throughout.
//   - Valid page range is now [1, m_pageCount] instead of [0, m_pageCount-1].
//   - firstPage() → 1, lastPage() → m_pageCount.
//   - This matches the QML reader which uses 1-based page numbers
//     everywhere (page grid, indicator, prev/next buttons).
//
// v15: When m_pageCount is 0 (PDF not yet loaded / page count unknown),
// accept any page >= 1. This prevents setPage() from silently rejecting
// the PdfMultiPageView's currentPage updates that arrive before the
// PdfDocument reports its final pageCount.
void ReaderService::setPage(int page) {
    if (page < 1) return;
    if (m_pageCount > 0 && page > m_pageCount) return;
    if (m_currentPage == page) return;
    m_currentPage = page;
    // Issue 2: persist the last-read page so the user resumes here
    // next time they open the same book.
    if (!m_currentBookId.isEmpty()) {
        m_lastPages[m_currentBookId] = page;
        emit pageSaved(m_currentBookId, page);
    }
    emit currentPageChanged();
}

// v15: setPageCount — called from QML when the PdfDocument finishes
// loading and reports its real page count. This replaces the old
// hardcoded "100" and ensures the sidebar page list, page indicator,
// prev/next button enabled states, and bookmark logic all use the
// actual PDF page count.
void ReaderService::setPageCount(int count) {
    if (count < 0) return;
    if (m_pageCount == count) return;
    m_pageCount = count;
    // Clamp current page into the new range.
    if (m_pageCount > 0 && m_currentPage > m_pageCount) {
        m_currentPage = m_pageCount;
        emit currentPageChanged();
    }
    // pageCount is exposed via Q_PROPERTY tied to currentBookChanged.
    emit currentBookChanged();
}

void ReaderService::nextPage() { setPage(m_currentPage + 1); }
void ReaderService::prevPage() { setPage(m_currentPage - 1); }
void ReaderService::firstPage() { setPage(1); }
void ReaderService::lastPage() { setPage(m_pageCount > 0 ? m_pageCount : 1); }

int ReaderService::lastReadPage(const QString& bookId) const {
    // Issue 2: return the persisted page for this book, defaulting
    // to 1 when the book has never been opened.
    return m_lastPages.value(bookId, 1);
}

bool ReaderService::isBookmarked(int page) const {
    return m_bookmarks.contains(page);
}

void ReaderService::toggleBookmark(int page) {
    if (m_bookmarks.contains(page)) m_bookmarks.removeAll(page);
    else m_bookmarks.append(page);
    emit bookmarksChanged();
}

void ReaderService::clearBookmarks() {
    m_bookmarks.clear();
    emit bookmarksChanged();
}

QVariantList ReaderService::bookmarks() const {
    QVariantList result;
    for (int p : m_bookmarks) result.append(p);
    return result;
}

} // namespace bookclub::client
