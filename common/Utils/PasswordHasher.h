// common/Utils/PasswordHasher.h
#pragma once

#include <QString>

namespace bookclub::common {

// Thin wrapper around CryptoUtils for password hashing/verification.
// Uses a per-password random salt + SHA-256 (adequate for this project's scope;
// swap for a slow KDF like bcrypt/argon2 if this ever needs to be production-grade).
class PasswordHasher {
public:
    // Returns "salt$hash" ready to store.
    static QString hash(const QString& plainPassword);

    // Verifies a plain password against a previously stored "salt$hash" value.
    static bool verify(const QString& plainPassword, const QString& storedHash);
};

} // namespace bookclub::common
