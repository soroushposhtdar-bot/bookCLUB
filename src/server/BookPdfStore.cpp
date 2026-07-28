// src/server/BookPdfStore.cpp
#include "src/server/BookPdfStore.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>

namespace bookclub::server {

BookPdfStore& BookPdfStore::instance() {
    static BookPdfStore s;
    return s;
}

BookPdfStore::BookPdfStore() {
    // Default location: <cwd>/storage/pdfs
    m_storageDir = QDir::currentPath() + "/storage/pdfs";
    common::FileSystemUtils::ensureDirectoryExists(m_storageDir);
    LOG_INFO("BookPdfStore: storage dir = " + m_storageDir);
}

QString BookPdfStore::storageDir() const {
    QMutexLocker lock(&m_mutex);
    return m_storageDir;
}

void BookPdfStore::setStorageDir(const QString& dir) {
    QMutexLocker lock(&m_mutex);
    m_storageDir = dir;
    common::FileSystemUtils::ensureDirectoryExists(m_storageDir);
}

QString BookPdfStore::absolutePathForBook(const QString& bookId) const {
    QMutexLocker lock(&m_mutex);
    // Sanitize bookId — strip any path separators so a malicious client
    // can't escape the storage directory with "../../etc/passwd".
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    return m_storageDir + "/" + safe + ".pdf";
}

QString BookPdfStore::relativePathForBook(const QString& bookId) {
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    return "pdfs/" + safe + ".pdf";
}

bool BookPdfStore::savePdf(const QString& bookId, const QByteArray& data) {
    QMutexLocker lock(&m_mutex);
    if (data.isEmpty()) {
        LOG_WARNING("BookPdfStore::savePdf: empty data for bookId=" + bookId);
        return false;
    }
    common::FileSystemUtils::ensureDirectoryExists(m_storageDir);

    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    const QString path = m_storageDir + "/" + safe + ".pdf";

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("BookPdfStore::savePdf: cannot open " + path + " for writing");
        return false;
    }
    const qint64 written = f.write(data);
    f.close();
    if (written != data.size()) {
        LOG_ERROR("BookPdfStore::savePdf: short write to " + path);
        return false;
    }
    LOG_INFO("BookPdfStore: saved " + QString::number(data.size()) +
             " bytes for bookId=" + bookId + " -> " + path);
    return true;
}

QByteArray BookPdfStore::loadPdf(const QString& bookId) const {
    QMutexLocker lock(&m_mutex);
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    const QString path = m_storageDir + "/" + safe + ".pdf";

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        LOG_DEBUG("BookPdfStore::loadPdf: cannot open " + path);
        return {};
    }
    return f.readAll();
}

bool BookPdfStore::hasPdf(const QString& bookId) const {
    QMutexLocker lock(&m_mutex);
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    const QString path = m_storageDir + "/" + safe + ".pdf";
    return QFileInfo(path).isFile();
}

bool BookPdfStore::deletePdf(const QString& bookId) {
    QMutexLocker lock(&m_mutex);
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    const QString path = m_storageDir + "/" + safe + ".pdf";
    return QFile::remove(path);
}

qint64 BookPdfStore::fileSize(const QString& bookId) const {
    QMutexLocker lock(&m_mutex);
    QString safe = bookId;
    safe.replace('/', '_').replace('\\', '_').replace("..", "_");
    if (safe.isEmpty()) safe = "unknown";
    const QString path = m_storageDir + "/" + safe + ".pdf";
    QFileInfo info(path);
    return info.isFile() ? info.size() : 0;
}

} // namespace bookclub::server
