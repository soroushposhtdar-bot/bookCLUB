// common/Utils/ValidationUtils.h
#pragma once

#include <QString>
#include <QRegularExpression>

namespace bookclub::common {

class ValidationUtils {
public:
    // ---- Email ----
    static bool isValidEmail(const QString& email);

    // ---- Phone ----
    static bool isValidPhoneNumber(const QString& phone);

    // ---- Username ----
    static bool isValidUsername(const QString& username);
    static bool isValidUsernameLength(const QString& username);

    // ---- Password ----
    static bool isValidPassword(const QString& password);
    static bool isStrongPassword(const QString& password);

    // ---- ISBN (Book-specific) ----
    static bool isValidISBN(const QString& isbn);
    static bool isValidISBN10(const QString& isbn);
    static bool isValidISBN13(const QString& isbn);

    // ---- URL ----
    static bool isValidUrl(const QString& url);

    // ---- General ----
    static bool isAlphanumeric(const QString& str);
    static bool isNumeric(const QString& str);
    static bool hasOnlyAllowedChars(const QString& str, const QString& allowed);
    static bool isEmptyOrWhitespace(const QString& str);
};

} // namespace bookclub::common
