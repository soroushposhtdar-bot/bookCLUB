// common/Utils/IdGenerator.h
#pragma once

#include <QString>

namespace bookclub::common {

// Generates unique identifiers used across models (Book::id, UserAccount::id, ...).
class IdGenerator {
public:
    // Standard UUID (without braces), e.g. "3f2504e0-4f89-11d3-9a0c-0305e82c3301"
    static QString generateUuid();

    // Shorter random alphanumeric id, useful for things like client connection ids.
    static QString generateShortId(int length = 8);
};

} // namespace bookclub::common
