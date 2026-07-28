// common/Utils/CryptoUtils.h
#pragma once

#include <QString>
#include <QByteArray>

namespace bookclub::common {

class CryptoUtils {
public:
    // ---- Hashing ----
    static QString sha256(const QString& data);
    static QByteArray sha256(const QByteArray& data);
    static QString sha256File(const QString& filePath);

    // ---- Base64 ----
    static QString toBase64(const QByteArray& data);
    static QByteArray fromBase64(const QString& base64);

    // ---- Random ----
    static QByteArray generateRandomBytes(int size);
    static QString generateRandomString(int length, bool includeDigits = true);

    // ---- HMAC (for API tokens) ----
    static QByteArray hmacSha256(const QByteArray& data, const QByteArray& key);
};

} // namespace bookclub::common
