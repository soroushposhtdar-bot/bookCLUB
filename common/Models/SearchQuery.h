#pragma once

#include <QString>
#include "common/AppEnums.h"
#include "common/Models/BookFilter.h"

namespace bookclub::common {

class SearchQuery {
public:
    QString keyword;
    SearchField field = SearchField::All;
    BookFilter filter;
    int page = 1;
    int pageSize = 20;
};

} // namespace bookclub::common
