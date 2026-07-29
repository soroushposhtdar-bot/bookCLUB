// common/Utils/PasswordHasher.cpp
#include "common/Utils/PasswordHasher.h"
#include "common/Utils/CryptoUtils.h"

namespace bookclub::common {

QString PasswordHasher::hash(const QString& plainPassword)
{
    const QString salt = CryptoUtils::generateRandomString(16);
    const QString hashed = CryptoUtils::sha256(salt + plainPassword);
    return salt + "$" + hashed;
}

bool PasswordHasher::verify(const QString& plainPassword, const QString& storedHash)
{
    const int separatorIndex = storedHash.indexOf('$');
    if (separatorIndex < 0) {
        return false;
    }

    const QString salt = storedHash.left(separatorIndex);
    const QString expectedHash = storedHash.mid(separatorIndex + 1);
    const QString actualHash = CryptoUtils::sha256(salt + plainPassword);

    return actualHash == expectedHash;
}

} // namespace bookclub::common
