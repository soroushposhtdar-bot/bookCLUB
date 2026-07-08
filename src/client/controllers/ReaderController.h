// src/client/controllers/ReaderController.h
#pragma once

#include <QObject>
#include <QString>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class ReaderController : public QObject {
    Q_OBJECT
public:
    explicit ReaderController(QObject* parent = nullptr);
    ~ReaderController() override;

    void openBook(const QString& bookId, const QString& pdfPath);
    void closeBook();
    void nextPage();
    void previousPage();
    void goToPage(int pageNumber);
    void setZoom(double zoomLevel);
    void saveCurrentPage();
    void loadLastPage();
    void syncStudySession(const QString& sessionId);

    // ---- Accessors ----
    QString getCurrentPageString() const;
    int getTotalPages() const;
    QString getProgressText() const;
    double getProgressPercent() const;
    bool hasNextPage() const;
    bool hasPreviousPage() const;

signals:
    void bookOpened(const QString& bookId);
    void pageChanged(int page);
    void zoomChanged(double zoomLevel);
    void lastPageSaved(int page);
    void readerError(const QString& message);

private:
    void handleSyncStudyPageResponse(const common::Message& response);

    QString m_bookId;
    QString m_pdfPath;
    QString m_sessionId;
    int m_currentPage;
    double m_zoomLevel;
    bool m_isBookOpen;
};

} // namespace bookclub::client
