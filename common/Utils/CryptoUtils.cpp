// common/Utils/CryptoUtils.cpp
#include "common/Utils/CryptoUtils.h"
#include <QCryptographicHash>
#include <QFile>
#include <QRandomGenerator>
#include <QCoreApplication>

namespace bookclub::common {

QString CryptoUtils::sha256(const QString& data) {
    return QString::fromLatin1(sha256(data.toUtf8()).toHex());
}

QByteArray CryptoUtils::sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString CryptoUtils::sha256File(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return {};
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString CryptoUtils::toBase64(const QByteArray& data) {
    return QString::fromLatin1(data.toBase64());
}

QByteArray CryptoUtils::fromBase64(const QString& base64) {
    return QByteArray::fromBase64(base64.toLatin1());
}

QByteArray CryptoUtils::generateRandomBytes(int size) {
    QByteArray bytes;
    bytes.resize(size);
    for (int i = 0; i < size; ++i) {
        bytes[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFF);
    }
    return bytes;
}

QString CryptoUtils::generateRandomString(int length, bool includeDigits) {
    const QString chars = QString("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") +
                          (includeDigits ? "0123456789" : "");
    QString result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));
    }
    return result;
}

QByteArray CryptoUtils::hmacSha256(const QByteArray& data, const QByteArray& key) {
    return QCryptographicHash::hash(data + key, QCryptographicHash::Sha256);
}

} // namespace bookclub::common
