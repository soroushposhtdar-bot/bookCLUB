// =============================================================================
//  ReaderViewModel.h
// =============================================================================
//  MVVM view-model for the in-app PDF reader.
// =============================================================================
#ifndef READERVIEWMODEL_H
#define READERVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

#include "viewmodels/user/UserViewModelBase.h"

// Include full service headers so MOC sees complete types for Q_PROPERTY pointers.
#include "services/ReaderService.h"

namespace bookclub::client {


class ReaderViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ReaderService* readerService READ readerService WRITE setReaderService NOTIFY readerServiceChanged)

    Q_PROPERTY(QString bookId    READ bookId    NOTIFY bookChanged)
    Q_PROPERTY(QString bookTitle READ bookTitle NOTIFY bookChanged)
    Q_PROPERTY(QString bookAuthor READ bookAuthor NOTIFY bookChanged)
    // v15j: cover properties for BookCover in the reader sidebar.
    Q_PROPERTY(QString title      READ bookTitle NOTIFY bookChanged)
    Q_PROPERTY(QString coverColor READ coverColor NOTIFY bookChanged)
    Q_PROPERTY(QString coverAccent READ coverAccent NOTIFY bookChanged)
    Q_PROPERTY(QString coverImage READ coverImage NOTIFY bookChanged)
    Q_PROPERTY(int     page      READ page      NOTIFY pageChanged)
    Q_PROPERTY(int     pageCount READ pageCount NOTIFY bookChanged)
    Q_PROPERTY(bool    hasBook   READ hasBook   NOTIFY bookChanged)
    Q_PROPERTY(qreal   zoom      READ zoom      NOTIFY zoomChanged)
    Q_PROPERTY(bool    fitWidth  READ fitWidth  NOTIFY fitModeChanged)
    Q_PROPERTY(bool    cleanMode READ cleanMode NOTIFY cleanModeChanged)
    Q_PROPERTY(QString pageText  READ pageText  NOTIFY pageChanged)
    // v13: pdfUrl is a QUrl suitable for QtQuick.Pdf's PdfDocument.source.
    // Returns an empty URL when no PDF is available so the QML can show
    // the synthetic-text fallback instead.
    Q_PROPERTY(QUrl    pdfUrl    READ pdfUrl    NOTIFY bookChanged)
    // pdfFilePath is the local cached path to the PDF (for "Open externally").
    Q_PROPERTY(QString pdfFilePath READ pdfFilePath NOTIFY bookChanged)

    // Table of contents + bookmarks (driven by ReaderService)
    Q_PROPERTY(QStringList   tableOfContents READ tableOfContents NOTIFY bookChanged)
    Q_PROPERTY(QVariantList  bookmarks       READ bookmarks       NOTIFY bookmarksChanged)
    Q_PROPERTY(bool          pageBookmarked  READ pageBookmarked  NOTIFY pageChanged)
    Q_PROPERTY(bool          loading         READ loading         NOTIFY loadingChanged)
    // hasError and error are inherited from UserViewModelBase — removed
    // duplicate Q_PROPERTY declarations that shadowed the base class.
    // Previously the shadowing caused MOC warnings and broke setError()
    // (wrote to base::m_error while QML read from ReaderViewModel::m_error).

public:
    explicit ReaderViewModel(QObject* parent = nullptr);

    ReaderService* readerService() const { return m_readerService; }
    void setReaderService(ReaderService* s);

    QString bookId() const;
    QString bookTitle() const;
    QString bookAuthor() const;
    // v15j: cover getters for BookCover.
    QString coverColor() const;
    QString coverAccent() const;
    QString coverImage() const;
    int page() const;
    int pageCount() const;
    bool hasBook() const;
    qreal zoom() const { return m_zoom; }
    bool fitWidth() const { return m_fitWidth; }
    bool cleanMode() const { return m_cleanMode; }
    QString pageText() const;
    QStringList  tableOfContents() const;
    QVariantList bookmarks() const;
    bool pageBookmarked() const;
    bool loading() const { return m_loading; }
    // error() and hasError() are inherited from UserViewModelBase.
    // Use setError() / clearError() from the base class.

    Q_INVOKABLE void openBook(const QString& bookId);
    Q_INVOKABLE void close();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE void goToPage(int page);
    Q_INVOKABLE void setPageCount(int count);  // v15: QML pushes real PDF page count
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void toggleFitWidth();
    Q_INVOKABLE void toggleCleanMode();
    Q_INVOKABLE void toggleBookmark();   // toggles bookmark on current page
    Q_INVOKABLE void toggleBookmark(int page);  // v15: toggles bookmark on a specific page
    Q_INVOKABLE void removeBookmark(int page);  // removes bookmark on a specific page
    Q_INVOKABLE void clearBookmarks();
    Q_INVOKABLE bool isBookmarked(int page) const;
    Q_INVOKABLE QString pageTextFor(int page) const;  // find-in-book support

    // v12: open the PDF in the system's default viewer (fallback when
    // the internal reader can't render the actual PDF file).
    Q_INVOKABLE bool openExternally() const;
    Q_INVOKABLE QString pdfFilePath() const;
    // v13: pdfUrl — QUrl form of pdfFilePath, suitable for
    // QtQuick.Pdf's PdfDocument.source. Returns empty when no PDF is set.
    Q_INVOKABLE QUrl    pdfUrl() const;

signals:
    void readerServiceChanged();
    void bookChanged();
    void pageChanged();
    void zoomChanged();
    void fitModeChanged();
    void cleanModeChanged();
    void closed();
    void bookmarksChanged();
    void loadingChanged();
    void errorChanged(const QString& error);

private:
    ReaderService* m_readerService = nullptr;
    qreal m_zoom = 1.0;
    bool m_fitWidth = true;
    bool m_cleanMode = false;
    bool m_loading = false;
    // m_error removed — was shadowing UserViewModelBase::m_error.
    // Use setError() / clearError() from the base class instead.
};

} // namespace bookclub::client

#endif // READERVIEWMODEL_H
