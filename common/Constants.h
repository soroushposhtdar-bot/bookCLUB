// common/Constants.h
//
// Project-wide constants used by server and client.
#pragma once

#include <QString>

namespace bookclub::common {

namespace Constants {

// ── Server defaults ──────────────────────────────────────────
inline constexpr quint16  DEFAULT_SERVER_PORT   = 8080;
inline constexpr int     DEFAULT_TIMEOUT_MS    = 10000;   // 10 seconds
inline constexpr int     MAX_CONNECTIONS        = 200;
inline constexpr int     MESSAGE_SIZE_LIMIT     = 10 * 1024 * 1024; // 10 MB

// ── Database ─────────────────────────────────────────────────
inline const QString& DEFAULT_DB_PATH() {
    static const QString path = QStringLiteral("bookclub.db");
    return path;
}

// ── Auth ─────────────────────────────────────────────────────
inline constexpr int     PASSWORD_MIN_LENGTH    = 6;
inline constexpr int     PASSWORD_MAX_LENGTH    = 128;
inline constexpr int     SALT_LENGTH            = 32;
inline constexpr int     SESSION_EXPIRY_DAYS    = 30;

// ── Books ────────────────────────────────────────────────────
inline constexpr int     MAX_TITLE_LENGTH       = 255;
inline constexpr int     MAX_AUTHOR_LENGTH      = 255;
inline constexpr int     MAX_DESCRIPTION_LENGTH = 10000;
inline constexpr double  MAX_BOOK_PRICE         = 999999.99;
inline constexpr double  MIN_BOOK_PRICE         = 0.0;
inline constexpr int     MAX_SEARCH_RESULTS     = 100;
inline constexpr int     MAX_REVIEW_TEXT_LENGTH  = 5000;
inline constexpr int     MIN_REVIEW_STARS        = 1;
inline constexpr int     MAX_REVIEW_STARS        = 5;

// ── Cart / Orders ────────────────────────────────────────────
inline constexpr int     MAX_CART_ITEMS         = 50;
inline constexpr int     MAX_QUANTITY_PER_ITEM  = 99;

// ── Discounts ────────────────────────────────────────────────
inline constexpr double  MAX_DISCOUNT_PERCENTAGE = 100.0;
inline constexpr double  MIN_DISCOUNT_FIXED     = 0.0;

// ── Pagination ───────────────────────────────────────────────
inline constexpr int     DEFAULT_PAGE_SIZE      = 20;
inline constexpr int     MAX_PAGE_SIZE          = 100;

// ── File upload ──────────────────────────────────────────────
inline constexpr qint64  MAX_COVER_FILE_SIZE    = 5 * 1024 * 1024; // 5 MB
inline constexpr qint64  MAX_PDF_FILE_SIZE      = 100 * 1024 * 1024; // 100 MB
inline const QStringList ALLOWED_IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp"};
inline const QString& ALLOWED_PDF_EXTENSION() {
    static const QString ext = QStringLiteral(".pdf");
    return ext;
}

// ── Logging ──────────────────────────────────────────────────
inline const QString& LOG_DIR() {
    static const QString dir = QStringLiteral("logs");
    return dir;
}

// ── Application ──────────────────────────────────────────────
inline const QString& APP_NAME() {
    static const QString name = QStringLiteral("BookClub");
    return name;
}
inline const QString& APP_VERSION() {
    static const QString ver = QStringLiteral("1.0.0");
    return ver;
}

} // namespace Constants

} // namespace bookclub::common