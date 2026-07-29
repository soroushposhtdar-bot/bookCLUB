// common/Models/Author.cpp
#include "common/Models/Author.h"

namespace bookclub::common {

Author::Author(QObject* parent) : QObject(parent) {}

Author::Author(const QString& id, const QString& name, QObject* parent)
    : QObject(parent), m_id(id), m_name(name) {}

} // namespace bookclub::common
