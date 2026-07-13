// =============================================================================
//  ReaderViewModel.cpp
// =============================================================================
#include "viewmodels/user/ReaderViewModel.h"
#include "services/ReaderService.h"
#include "services/MockDataStore.h"
#include "services/MockTypes.h"

#include <QVariantList>

namespace bookclub::client {

ReaderViewModel::ReaderViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void ReaderViewModel::setReaderService(ReaderService* s) {
    if (m_readerService == s) return;
    if (m_readerService) disconnect(m_readerService, nullptr, this, nullptr);
    m_readerService = s;
    if (m_readerService) {
        connect(m_readerService, &ReaderService::currentBookChanged, this, [this]() {
            emit bookChanged();
            emit pageChanged();  // pageBookmarked depends on current page
        });
        connect(m_readerService, &ReaderService::currentPageChanged, this, [this]() {
            emit pageChanged();
        });
        connect(m_readerService, &ReaderService::bookmarksChanged, this, [this]() {
            emit bookmarksChanged();
            emit pageChanged();  // pageBookmarked depends on bookmarks
        });
    }
    emit readerServiceChanged();
}

QString ReaderViewModel::bookId()    const { return m_readerService ? m_readerService->currentBookId()    : QString{}; }
QString ReaderViewModel::bookTitle() const { return m_readerService ? m_readerService->currentBookTitle() : QString{}; }
int     ReaderViewModel::page()      const { return m_readerService ? m_readerService->currentPage()      : 0; }
int     ReaderViewModel::pageCount() const { return m_readerService ? m_readerService->pageCount()        : 0; }
bool    ReaderViewModel::hasBook()   const { return m_readerService && m_readerService->hasOpenBook(); }

QStringList ReaderViewModel::tableOfContents() const {
    return m_readerService ? m_readerService->tableOfContents() : QStringList{};
}

QVariantList ReaderViewModel::bookmarks() const {
    return m_readerService ? m_readerService->bookmarks() : QVariantList{};
}

bool ReaderViewModel::pageBookmarked() const {
    if (!m_readerService || !hasBook()) return false;
    return m_readerService->isBookmarked(page());
}

bool ReaderViewModel::isBookmarked(int p) const {
    return m_readerService && m_readerService->isBookmarked(p);
}

// -----------------------------------------------------------------------------
//  pageText — synthesize a page of readable content from the book metadata.
//
//  Real build: extract the page's text from the PDF via Poppler / QtPDF.
//  Mock build: build a deterministic paragraph from the book title, author,
//  and page number so the reader feels like real prose, not "Lorem ipsum".
// -----------------------------------------------------------------------------
QString ReaderViewModel::pageText() const {
    if (!hasBook()) return QString{};

    const int p = page();
    const int total = pageCount();
    const QString title = bookTitle();

    // Split the synthesis into 3 paragraphs to give the page visual rhythm.
    const QStringList openers = {
        QStringLiteral("In this section of %1, the narrative turns to the quieter rhythms that "
                       "define the characters' inner lives.").arg(title),
        QStringLiteral("The chapter opens with a moment of reflection — one that quietly ties "
                       "together the threads introduced earlier in %1.").arg(title),
        QStringLiteral("Here, %1 finds its rhythm. The prose slows; the imagery deepens; the "
                       "reader is invited to dwell.").arg(title),
        QStringLiteral("A turn of pace arrives in this part of %1 — events quicken, and the "
                       "stakes become unmistakably clear.").arg(title),
        QStringLiteral("This stretch of %1 is built around a single, sustained image — one "
                       "that the author returns to, varies, and finally resolves.").arg(title)
    };

    const QStringList bodies = {
        QStringLiteral("The scene is set with careful attention to detail: the time of day, "
                       "the quality of the light, the small gestures that betray what the "
                       "characters cannot say aloud. Each sentence is doing more than one "
                       "thing at once — moving the plot forward while shading in the "
                       "emotional register that will pay off chapters later."),
        QStringLiteral("What follows is a passage of sustained description. The narrator "
                       "lingers on the texture of the world — the worn wood of the table, "
                       "the smell of the rain, the half-heard conversation in the next room. "
                       "It is the kind of writing that asks the reader to slow down and "
                       "inhabit the moment alongside the characters."),
        QStringLiteral("Dialogue carries the weight here. The exchanges are short, almost "
                       "elliptical, but each line lands with the precision of a line break "
                       "in a poem. What is left unsaid matters as much as what is spoken, "
                       "and the silences between the lines do real work."),
        QStringLiteral("The pacing shifts. Where the previous pages moved with deliberate "
                       "slowness, here the prose tightens. Sentences shorten. The narrative "
                       "camera pulls in closer. The reader feels the change in their body "
                       "before they name it — which is, of course, exactly the intention."),
        QStringLiteral("This is a chapter of small recognitions. Nothing dramatic happens, "
                       "exactly, but by the end of the page the reader understands "
                       "something about the protagonist that they did not understand "
                       "before — and the protagonist, perhaps, understands something "
                       "about themselves.")
    };

    const QStringList closers = {
        QStringLiteral("The page ends with a line that quietly opens onto the next chapter, "
                       "leaving the reader in the hold of a question that will not be "
                       "answered for some time."),
        QStringLiteral("And then, with a turn of phrase that is almost casual, the section "
                       "closes — the kind of ending that makes you read the next paragraph "
                       "before you realise you've turned the page."),
        QStringLiteral("The final image is held for a beat longer than expected. It is a "
                       "small gesture, but it carries the whole weight of what has come "
                       "before it."),
        QStringLiteral("What lingers is not the event itself but the way it is observed. "
                       "The narrator steps back; the reader steps in; the chapter ends."),
        QStringLiteral("And so the page turns. The story does not pause — it never really "
                       "does — but for a moment, here, it seems to.")
    };

    const int openerIdx = (p - 1) % openers.size();
    const int bodyIdx    = (p - 1) % bodies.size();
    const int closerIdx  = (p - 1) % closers.size();

    return QStringLiteral("%1\n\n%2\n\n%3\n\n— page %4 of %5 —")
            .arg(openers.at(openerIdx))
            .arg(bodies.at(bodyIdx))
            .arg(closers.at(closerIdx))
            .arg(p).arg(total);
}

void ReaderViewModel::openBook(const QString& bookId) {
    if (!m_readerService) {
        m_error = QStringLiteral("Reader service is not available.");
        emit errorChanged(m_error);
        return;
    }
    m_error.clear();
    emit errorChanged(QString{});

    m_loading = true;
    emit loadingChanged();

    // The mock service resolves synchronously; emulate a small delay so the
    // loading state is visible and the UI can show its skeleton.
    QMetaObject::invokeMethod(this, [this, bookId]() {
        const bool ok = m_readerService->openBook(bookId);
        m_loading = false;
        emit loadingChanged();
        if (!ok) {
            m_error = QStringLiteral("Couldn't open this book. It may have been removed.");
            emit errorChanged(m_error);
            return;
        }
        m_zoom = 1.0;
        m_fitWidth = true;
        emit zoomChanged();
        emit fitModeChanged();
        emit bookChanged();
        emit pageChanged();
        emit bookmarksChanged();
    }, Qt::QueuedConnection);
}

void ReaderViewModel::close() {
    if (m_readerService) m_readerService->closeBook();
    m_cleanMode = false;
    m_zoom = 1.0;
    m_fitWidth = true;
    emit cleanModeChanged();
    emit zoomChanged();
    emit fitModeChanged();
    emit closed();
}

void ReaderViewModel::nextPage()  { if (m_readerService) m_readerService->nextPage(); }
void ReaderViewModel::prevPage()  { if (m_readerService) m_readerService->prevPage(); }
void ReaderViewModel::firstPage() { if (m_readerService) m_readerService->firstPage(); }
void ReaderViewModel::lastPage()  { if (m_readerService) m_readerService->lastPage(); }
void ReaderViewModel::goToPage(int p) { if (m_readerService) m_readerService->setPage(p); }

void ReaderViewModel::zoomIn()  { m_fitWidth = false; emit fitModeChanged(); m_zoom = qMin(m_zoom + 0.2, 3.0); emit zoomChanged(); }
void ReaderViewModel::zoomOut() { m_fitWidth = false; emit fitModeChanged(); m_zoom = qMax(m_zoom - 0.2, 0.5); emit zoomChanged(); }
void ReaderViewModel::resetZoom() {
    m_zoom = 1.0; m_fitWidth = true;
    emit zoomChanged(); emit fitModeChanged();
}
void ReaderViewModel::toggleFitWidth() {
    m_fitWidth = !m_fitWidth;
    if (m_fitWidth) m_zoom = 1.0;
    emit fitModeChanged(); emit zoomChanged();
}
void ReaderViewModel::toggleCleanMode() {
    m_cleanMode = !m_cleanMode;
    emit cleanModeChanged();
}

void ReaderViewModel::toggleBookmark() {
    if (!m_readerService || !hasBook()) return;
    m_readerService->toggleBookmark(page());
    // bookmarksChanged + pageChanged are wired through setReaderService
}

void ReaderViewModel::clearBookmarks() {
    if (m_readerService) m_readerService->clearBookmarks();
}

void ReaderViewModel::removeBookmark(int page) {
    // toggleBookmark(page) on the service will remove the bookmark if it
    // already exists (which it does — the user clicked an existing bookmark).
    if (m_readerService) m_readerService->toggleBookmark(page);
}

// pageTextFor — returns the synthesized text for an arbitrary page without
// navigating to it. Used by PdfReaderPage's find-in-book feature to search
// across all pages. Reuses the same synthesis logic as pageText() but with
// the given page number instead of the current page.
QString ReaderViewModel::pageTextFor(int targetPage) const {
    if (!hasBook()) return QString{};
    const int total = pageCount();
    if (total <= 0 || targetPage < 1 || targetPage > total) return QString{};

    const QString title = bookTitle();
    const int p = targetPage;

    const QStringList openers = {
        QStringLiteral("In this section of %1, the narrative turns to the quieter rhythms that "
                       "define the characters' inner lives.").arg(title),
        QStringLiteral("The chapter opens with a moment of reflection — one that quietly ties "
                       "together the threads introduced earlier in %1.").arg(title),
        QStringLiteral("Here, %1 finds its rhythm. The prose slows; the imagery deepens; the "
                       "reader is invited to dwell.").arg(title),
        QStringLiteral("A turn of pace arrives in this part of %1 — events quicken, and the "
                       "stakes become unmistakably clear.").arg(title),
        QStringLiteral("This stretch of %1 is built around a single, sustained image — one "
                       "that the author returns to, varies, and finally resolves.").arg(title)
    };
    const QStringList bodies = {
        QStringLiteral("The scene is set with careful attention to detail: the time of day, "
                       "the quality of the light, the small gestures that betray what the "
                       "characters cannot say aloud. Each sentence is doing more than one "
                       "thing at once — moving the plot forward while shading in the "
                       "emotional register that will pay off chapters later."),
        QStringLiteral("What follows is a passage of sustained description. The narrator "
                       "lingers on the texture of the world — the worn wood of the table, "
                       "the smell of the rain, the half-heard conversation in the next room. "
                       "It is the kind of writing that asks the reader to slow down and "
                       "inhabit the moment alongside the characters."),
        QStringLiteral("Dialogue carries the weight here. The exchanges are short, almost "
                       "elliptical, but each line lands with the precision of a line break "
                       "in a poem. What is left unsaid matters as much as what is spoken, "
                       "and the silences between the lines do real work."),
        QStringLiteral("The pacing shifts. Where the previous pages moved with deliberate "
                       "slowness, here the prose tightens. Sentences shorten. The narrative "
                       "camera pulls in closer. The reader feels the change in their body "
                       "before they name it — which is, of course, exactly the intention."),
        QStringLiteral("This is a chapter of small recognitions. Nothing dramatic happens, "
                       "exactly, but by the end of the page the reader understands "
                       "something about the protagonist that they did not understand "
                       "before — and the protagonist, perhaps, understands something "
                       "about themselves.")
    };
    const QStringList closers = {
        QStringLiteral("The page ends with a line that quietly opens onto the next chapter, "
                       "leaving the reader in the hold of a question that will not be "
                       "answered for some time."),
        QStringLiteral("And then, with a turn of phrase that is almost casual, the section "
                       "closes — the kind of ending that makes you read the next paragraph "
                       "before you realise you've turned the page."),
        QStringLiteral("The final image is held for a beat longer than expected. It is a "
                       "small gesture, but it carries the whole weight of what has come "
                       "before it."),
        QStringLiteral("What lingers is not the event itself but the way it is observed. "
                       "The narrator steps back; the reader steps in; the chapter ends."),
        QStringLiteral("And so the page turns. The story does not pause — it never really "
                       "does — but for a moment, here, it seems to.")
    };

    const int openerIdx = (p - 1) % openers.size();
    const int bodyIdx    = (p - 1) % bodies.size();
    const int closerIdx  = (p - 1) % closers.size();

    return QStringLiteral("%1\n\n%2\n\n%3\n\n— page %4 of %5 —")
            .arg(openers.at(openerIdx))
            .arg(bodies.at(bodyIdx))
            .arg(closers.at(closerIdx))
            .arg(p).arg(total);
}

} // namespace bookclub::client
