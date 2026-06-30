// common/Utils/FileSystemUtils.cpp
#include "common/Utils/FileSystemUtils.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>

namespace bookclub::common {

// ---- Directory ----
bool FileSystemUtils::ensureDirectoryExists(const QString& path) {
    if (directoryExists(path)) return true;
    return createDirectory(path);
}

bool FileSystemUtils::directoryExists(const QString& path) {
    return QDir(path).exists();
}

bool FileSystemUtils::createDirectory(const QString& path) {
    return QDir().mkpath(path);
}

bool FileSystemUtils::removeDirectory(const QString& path, bool recursive) {
    if (recursive) {
        return QDir(path).removeRecursively();
    }
    return QDir(path).rmdir(path);
}

// ---- File checks ----
bool FileSystemUtils::fileExists(const QString& path) {
    return QFileInfo(path).isFile();
}

qint64 FileSystemUtils::fileSize(const QString& path) {
    return QFileInfo(path).size();
}

bool FileSystemUtils::isFileEmpty(const QString& path) {
    return fileSize(path) == 0;
}

QString FileSystemUtils::fileExtension(const QString& path) {
    return QFileInfo(path).suffix();
}

QString FileSystemUtils::fileName(const QString& path) {
    return QFileInfo(path).fileName();
}

QString FileSystemUtils::baseName(const QString& path) {
    return QFileInfo(path).baseName();
}

// ---- Read ----
QString FileSystemUtils::readTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QByteArray FileSystemUtils::readBinaryFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

QStringList FileSystemUtils::readAllLines(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QStringList lines;
    while (!file.atEnd()) {
        lines.append(QString::fromUtf8(file.readLine()).trimmed());
    }
    return lines;
}

// ---- Write ----
bool FileSystemUtils::writeTextFile(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    return file.write(content.toUtf8()) > 0;
}

bool FileSystemUtils::writeBinaryFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(data) > 0;
}

bool FileSystemUtils::appendTextFile(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return false;
    }
    return file.write(content.toUtf8()) > 0;
}

// ---- Copy / Move / Delete ----
bool FileSystemUtils::copyFile(const QString& source, const QString& dest, bool overwrite) {
    if (fileExists(dest) && !overwrite) {
        return false;
    }
    ensureDirectoryExists(QFileInfo(dest).absolutePath());
    return QFile::copy(source, dest);
}

bool FileSystemUtils::moveFile(const QString& source, const QString& dest, bool overwrite) {
    if (fileExists(dest) && !overwrite) {
        return false;
    }
    if (fileExists(dest)) {
        deleteFile(dest);
    }
    ensureDirectoryExists(QFileInfo(dest).absolutePath());
    return QFile::rename(source, dest);
}

bool FileSystemUtils::renameFile(const QString& oldPath, const QString& newPath) {
    return QFile::rename(oldPath, newPath);
}

bool FileSystemUtils::deleteFile(const QString& path) {
    return QFile::remove(path);
}

// ---- Utility ----
QString FileSystemUtils::normalizePath(const QString& path) {
    return QDir::cleanPath(path);
}

QString FileSystemUtils::joinPath(const QString& base, const QString& relative) {
    return QDir(base).filePath(relative);
}

QString FileSystemUtils::getTempDirectory() {
    return QDir::tempPath();
}

QString FileSystemUtils::getCurrentDirectory() {
    return QDir::currentPath();
}

QString FileSystemUtils::getHomeDirectory() {
    return QDir::homePath();
}

QString FileSystemUtils::getAppDataDirectory(const QString& appName) {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + appName;
}

} // namespace bookclub::common
