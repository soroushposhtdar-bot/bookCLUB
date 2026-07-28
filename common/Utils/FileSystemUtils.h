// common/Utils/FileSystemUtils.h
#pragma once

#include <QString>
#include <QByteArray>
#include <QStringList>

namespace bookclub::common {

class FileSystemUtils {
public:
    // ---- Directory ----
    static bool ensureDirectoryExists(const QString& path);
    static bool directoryExists(const QString& path);
    static bool createDirectory(const QString& path);
    static bool removeDirectory(const QString& path, bool recursive = false);

    // ---- File checks ----
    static bool fileExists(const QString& path);
    static qint64 fileSize(const QString& path);
    static bool isFileEmpty(const QString& path);
    static QString fileExtension(const QString& path);
    static QString fileName(const QString& path);
    static QString baseName(const QString& path);

    // ---- Read ----
    static QString readTextFile(const QString& path);
    static QByteArray readBinaryFile(const QString& path);
    static QStringList readAllLines(const QString& path);

    // ---- Write ----
    static bool writeTextFile(const QString& path, const QString& content);
    static bool writeBinaryFile(const QString& path, const QByteArray& data);
    static bool appendTextFile(const QString& path, const QString& content);

    // ---- Copy / Move / Delete ----
    static bool copyFile(const QString& source, const QString& dest, bool overwrite = false);
    static bool moveFile(const QString& source, const QString& dest, bool overwrite = false);
    static bool renameFile(const QString& oldPath, const QString& newPath);
    static bool deleteFile(const QString& path);

    // ---- Utility ----
    static QString normalizePath(const QString& path);
    static QString joinPath(const QString& base, const QString& relative);
    static QString getTempDirectory();
    static QString getCurrentDirectory();
    static QString getHomeDirectory();
    static QString getAppDataDirectory(const QString& appName);
};

} // namespace bookclub::common
