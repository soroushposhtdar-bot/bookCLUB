// common/Utils/ValidationUtils.cpp
#include "common/Utils/ValidationUtils.h"

namespace bookclub::common {

// ---- Email ----
bool ValidationUtils::isValidEmail(const QString& email) {
    static QRegularExpression regex(
        R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
    );
    return regex.match(email).hasMatch();
}

// ---- Phone ----
bool ValidationUtils::isValidPhoneNumber(const QString& phone) {
    // Supports Iranian mobile numbers (0912-1234567) and international format
    static QRegularExpression regex(
        R"(^(0|\+98)?9[0-9]{9}$|^(\+?[0-9]{8,15})?$)"
    );
    return regex.match(phone).hasMatch();
}

// ---- Username ----
bool ValidationUtils::isValidUsername(const QString& username) {
    static QRegularExpression regex(
        R"(^[a-zA-Z0-9_]{3,20}$)"
    );
    return regex.match(username).hasMatch();
}

bool ValidationUtils::isValidUsernameLength(const QString& username) {
    return username.length() >= 3 && username.length() <= 20;
}

// ---- Password ----
bool ValidationUtils::isValidPassword(const QString& password) {
    return password.length() >= 6 && password.length() <= 64;
}

bool ValidationUtils::isStrongPassword(const QString& password) {
    // At least: 8 chars, one uppercase, one lowercase, one digit, one special char
    if (password.length() < 8) return false;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    const QString specials = R"(!@#$%^&*()_+-=[]{}|;:,.<>?/)";
    for (const QChar& ch : password) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
        else if (specials.contains(ch)) hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

// ---- ISBN ----
bool ValidationUtils::isValidISBN(const QString& isbn) {
    QString cleaned = isbn;
    cleaned.remove('-');
    cleaned.remove(' ');
    return cleaned.length() == 10 ? isValidISBN10(cleaned) :
           cleaned.length() == 13 ? isValidISBN13(cleaned) : false;
}

bool ValidationUtils::isValidISBN10(const QString& isbn) {
    if (isbn.length() != 10) return false;
    int sum = 0;
    for (int i = 0; i < 9; ++i) {
        QChar ch = isbn[i];
        if (!ch.isDigit()) return false;
        sum += (i + 1) * ch.digitValue();
    }
    QChar last = isbn[9];
    if (last == 'X' || last == 'x') {
        sum += 10 * 10; // check digit = 10
    } else if (last.isDigit()) {
        sum += 10 * last.digitValue();
    } else {
        return false;
    }
    return sum % 11 == 0;
}

bool ValidationUtils::isValidISBN13(const QString& isbn) {
    if (isbn.length() != 13) return false;
    int sum = 0;
    for (int i = 0; i < 13; ++i) {
        QChar ch = isbn[i];
        if (!ch.isDigit()) return false;
        int digit = ch.digitValue();
        sum += (i % 2 == 0) ? digit : digit * 3;
    }
    return sum % 10 == 0;
}

// ---- URL ----
bool ValidationUtils::isValidUrl(const QString& url) {
    static QRegularExpression regex(
        R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)"
    );
    return regex.match(url).hasMatch();
}

// ---- General ----
bool ValidationUtils::isAlphanumeric(const QString& str) {
    for (const QChar& ch : str) {
        if (!ch.isLetterOrNumber()) return false;
    }
    return true;
}

bool ValidationUtils::isNumeric(const QString& str) {
    bool ok;
    str.toLongLong(&ok);
    return ok;
}

bool ValidationUtils::hasOnlyAllowedChars(const QString& str, const QString& allowed) {
    for (const QChar& ch : str) {
        if (!allowed.contains(ch)) return false;
    }
    return true;
}

bool ValidationUtils::isEmptyOrWhitespace(const QString& str) {
    return str.trimmed().isEmpty();
}

} // namespace bookclub::common
