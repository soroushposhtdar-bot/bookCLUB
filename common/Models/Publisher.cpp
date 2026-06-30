#include "common/Models/Publisher.h"

namespace bookclub::common {

Publisher::Publisher(QObject* parent) : UserAccount(parent) {}

Publisher::Publisher(const QString& id, const QString& username, QObject* parent)
    : UserAccount(id, username, parent) {}

AccountRole Publisher::role() const {
    return AccountRole::Publisher;
}

QString Publisher::roleName() const {
    return QStringLiteral("ناشر");
}

// ---- Getter ----
const QString& Publisher::publisherName() const { return m_publisherName; }
const QString& Publisher::biography() const { return m_biography; }
const QString& Publisher::website() const { return m_website; }
const QString& Publisher::taxId() const { return m_taxId; }

// ---- Setter ----
void Publisher::setPublisherName(const QString& name) {
    if (m_publisherName != name) {
        m_publisherName = name;
        emit publisherInfoChanged();
        emit profileChanged();
    }
}

void Publisher::setBiography(const QString& biography) {
    if (m_biography != biography) {
        m_biography = biography;
        emit publisherInfoChanged();
    }
}

void Publisher::setWebsite(const QString& website) {
    if (m_website != website) {
        m_website = website;
        emit publisherInfoChanged();
    }
}

void Publisher::setTaxId(const QString& taxId) {
    if (m_taxId != taxId) {
        m_taxId = taxId;
        emit publisherInfoChanged();
    }
}

} // namespace bookclub::common
