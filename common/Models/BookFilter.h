#pragma once

#include <QStringList>

namespace bookclub::common {

class BookFilter {
public:
    QString keyword;
    QStringList genreIds;
    QString authorName;
    QString publisherName;
    bool onlyFree = false;
    bool onlyActive = true;
};

} // namespace bookclub::common
