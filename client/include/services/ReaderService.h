#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <QQmlEngine>

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
    // v15j: cover properties for the BookCover in the reader sidebar.
    Q_PROPERTY(QString coverColor  READ coverColor  NOTIFY currentBookChanged)
    Q_PROPERTY(QString coverAccent READ coverAccent NOTIFY currentBookChanged)
    Q_PROPERTY(QString coverImage  READ coverImage  NOTIFY currentBookChanged)
    Q_PROPERTY(QString title       READ currentBookTitle NOTIFY currentBookChanged)
    Q_PROPERTY(QString authorName  READ authorName  NOTIFY currentBookChanged)

public:
    explicit ReaderService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op

    QString currentBookId() const { return m_currentBookId; }
    QString currentBookTitle() const { return m_currentTitle; }
    QString currentPdfPath() const { return m_pdfPath; }  // v12: for openExternally
    // v15j: cover getters for BookCover in reader sidebar.
    QString coverColor() const { return m_coverColor; }
    QString coverAccent() const { return m_coverAccent; }
    QString coverImage() const { return m_coverImage; }
    QString authorName() const { return m_authorName; }
    int currentPage() const { return m_currentPage; }
    int pageCount() const { return m_pageCount; }
    bool hasOpenBook() const { return !m_currentBookId.isEmpty(); }
    QStringList tableOfContents() const { return m_toc; }
    QVariantList bookmarks() const;

    Q_INVOKABLE bool openBook(const QString& bookId);
    Q_INVOKABLE void closeBook();
    Q_INVOKABLE void setPage(int page);
    Q_INVOKABLE void setPageCount(int count);   // v15: QML pushes real PDF page count
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE int  lastReadPage(const QString& bookId) const;

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
    // Returns the local cache path for a book's PDF
    // (<AppDataLocation>/pdf_cache/<bookId>.pdf). Creates the cache dir
    // on first call. Returns an empty string on failure.
    QString localPdfCachePath(const QString& bookId) const;

    QString m_currentBookId;
    QString m_currentTitle;
    QString m_pdfPath;
    // v15j: cover metadata (populated from GetBookDetails response).
    QString m_coverColor;
    QString m_coverAccent;
    QString m_coverImage;
    QString m_authorName;
    int m_currentPage = 1;
    int m_pageCount = 0;
    QStringList m_toc;
    QHash<QString, int> m_lastPages;
    QList<int> m_bookmarks;
};

} // namespace bookclub::client
