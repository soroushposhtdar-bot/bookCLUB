#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class ReaderController : public QObject {
    Q_OBJECT
public:
    explicit ReaderController(QObject* parent = nullptr);
    ~ReaderController() override = default;

    void openBook(const QString& bookId, const QString& pdfPath);
    void closeBook();
    void nextPage();
    void previousPage();
    void goToPage(int pageNumber);
    void setZoom(double zoomLevel);
    void saveCurrentPage();
    void syncStudySession(const QString& sessionId);

signals:
    void bookOpened(const QString& bookId);
    void pageChanged(int pageNumber);
    void zoomChanged(double zoomLevel);
    void lastPageSaved(int pageNumber);
    void readerError(const QString& message);

private:
    QString m_bookId;
    QString m_pdfPath;
};

} // namespace bookclub::client
