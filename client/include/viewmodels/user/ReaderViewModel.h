// =============================================================================
//  ReaderViewModel.h
// =============================================================================
//  MVVM view-model for the in-app PDF reader.
// =============================================================================
#ifndef READERVIEWMODEL_H
#define READERVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class ReaderService;

class ReaderViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ReaderService* readerService READ readerService WRITE setReaderService NOTIFY readerServiceChanged)

    Q_PROPERTY(QString bookId    READ bookId    NOTIFY bookChanged)
    Q_PROPERTY(QString bookTitle READ bookTitle NOTIFY bookChanged)
    Q_PROPERTY(int     page      READ page      NOTIFY pageChanged)
    Q_PROPERTY(int     pageCount READ pageCount NOTIFY bookChanged)
    Q_PROPERTY(bool    hasBook   READ hasBook   NOTIFY bookChanged)
    Q_PROPERTY(qreal   zoom      READ zoom      NOTIFY zoomChanged)
    Q_PROPERTY(bool    fitWidth  READ fitWidth  NOTIFY fitModeChanged)
    Q_PROPERTY(bool    cleanMode READ cleanMode NOTIFY cleanModeChanged)
    Q_PROPERTY(QString pageText  READ pageText  NOTIFY pageChanged)

    // Table of contents + bookmarks (driven by ReaderService)
    Q_PROPERTY(QStringList   tableOfContents READ tableOfContents NOTIFY bookChanged)
    Q_PROPERTY(QVariantList  bookmarks       READ bookmarks       NOTIFY bookmarksChanged)
    Q_PROPERTY(bool          pageBookmarked  READ pageBookmarked  NOTIFY pageChanged)
    Q_PROPERTY(bool          loading         READ loading         NOTIFY loadingChanged)
    Q_PROPERTY(QString       error           READ error           NOTIFY errorChanged)
    Q_PROPERTY(bool          hasError        READ hasError        NOTIFY errorChanged)

public:
    explicit ReaderViewModel(QObject* parent = nullptr);

    ReaderService* readerService() const { return m_readerService; }
    void setReaderService(ReaderService* s);

    QString bookId() const;
    QString bookTitle() const;
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
    QString error() const { return m_error; }
    bool hasError() const { return !m_error.isEmpty(); }

    Q_INVOKABLE void openBook(const QString& bookId);
    Q_INVOKABLE void close();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE void goToPage(int page);
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void toggleFitWidth();
    Q_INVOKABLE void toggleCleanMode();
    Q_INVOKABLE void toggleBookmark();   // toggles bookmark on current page
    Q_INVOKABLE void removeBookmark(int page);  // removes bookmark on a specific page
    Q_INVOKABLE void clearBookmarks();
    Q_INVOKABLE bool isBookmarked(int page) const;
    Q_INVOKABLE QString pageTextFor(int page) const;  // find-in-book support

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
    QString m_error;
};

} // namespace bookclub::client

#endif // READERVIEWMODEL_H
