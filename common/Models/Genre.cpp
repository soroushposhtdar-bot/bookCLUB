#include "common/Models/Genre.h"

namespace bookclub::common {

Genre::Genre(QString id, QString name)
    : m_id(std::move(id)), m_name(std::move(name)) {}

const QString& Genre::id() const { return m_id; }
const QString& Genre::name() const { return m_name; }
const QStringList& Genre::aliases() const { return m_aliases; }

void Genre::setId(const QString& id) { m_id = id; }
void Genre::setName(const QString& name) { m_name = name; }
void Genre::setAliases(const QStringList& aliases) { m_aliases = aliases; }

bool Genre::isValid() const {
    return !m_id.isEmpty() && !m_name.isEmpty();
}

} // namespace bookclub::common
