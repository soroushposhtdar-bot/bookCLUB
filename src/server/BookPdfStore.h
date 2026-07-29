// src/server/BookPdfStore.h
//
// Server-side storage helper for book PDF files.
//
// PDFs are stored on disk under <storageDir>/<bookId>.pdf where
// storageDir resolves to "<cwd>/storage/pdfs" by default. Only the
// relative filename ("pdfs/<bookId>.pdf") is stored in the Books.pdfFilePath
// column of the database, so the storage tree can be relocated without
// touching the DB.
//
// All operations are synchronous and thread-safe via a QMutex — the server
// runs request handlers in a thread pool, so concurrent uploads/downloads
// of the same book must not race on the underlying file.
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>

namespace bookclub::server {

class BookPdfStore {
public:
    static BookPdfStore& instance();

    // Resolved on first use. Default: <cwd>/storage/pdfs
    QString storageDir() const;
    void setStorageDir(const QString& dir);

    // Absolute path on disk for a given bookId.
    QString absolutePathForBook(const QString& bookId) const;

    // Relative path stored in the DB (e.g. "pdfs/<bookId>.pdf").
    static QString relativePathForBook(const QString& bookId);

    // I/O — all return false / empty on failure.
    bool savePdf(const QString& bookId, const QByteArray& data);
    QByteArray loadPdf(const QString& bookId) const;
    bool hasPdf(const QString& bookId) const;
    bool deletePdf(const QString& bookId);
    qint64 fileSize(const QString& bookId) const;

private:
    BookPdfStore();
    BookPdfStore(const BookPdfStore&) = delete;
    BookPdfStore& operator=(const BookPdfStore&) = delete;

    mutable QMutex m_mutex;
    QString m_storageDir;
};

} // namespace bookclub::server
