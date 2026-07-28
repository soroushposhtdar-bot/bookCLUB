#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QQmlEngine>
#include <QJsonObject>

namespace bookclub::client {

class ReviewDto : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id              READ id              CONSTANT)
    Q_PROPERTY(QString bookId          READ bookId          CONSTANT)
    Q_PROPERTY(QString userId          READ userId          CONSTANT)
    Q_PROPERTY(QString userDisplayName READ userDisplayName CONSTANT)
    Q_PROPERTY(QString text            READ text            CONSTANT)
    Q_PROPERTY(int     stars           READ stars           CONSTANT)
    Q_PROPERTY(QString createdAtText   READ createdAtText   CONSTANT)
    Q_PROPERTY(QString relativeTime    READ relativeTime    CONSTANT)
    Q_PROPERTY(bool    isEdited        READ isEdited        CONSTANT)
    Q_PROPERTY(bool    isPinned        READ isPinned        CONSTANT)
    Q_PROPERTY(bool    isFlagged       READ isFlagged       CONSTANT)
    Q_PROPERTY(int     helpfulCount    READ helpfulCount    CONSTANT)
    Q_PROPERTY(int     replyCount      READ replyCount      CONSTANT)
    Q_PROPERTY(bool    userFoundHelpful READ userFoundHelpful CONSTANT)

    // --- Alias properties for QML compatibility ---
    // Some QML files use the old property names. These aliases let both
    // old and new names work without changing every QML file.
    Q_PROPERTY(int     rating          READ stars           CONSTANT)
    Q_PROPERTY(QString comment         READ text            CONSTANT)
    Q_PROPERTY(bool    pinned          READ isPinned        CONSTANT)
    Q_PROPERTY(bool    byCurrentUser   READ byCurrentUser   NOTIFY byCurrentUserChanged)
    Q_PROPERTY(bool    verifiedPurchase READ verifiedPurchase CONSTANT)

public:
    explicit ReviewDto(QObject* parent = nullptr) : QObject(parent) {}

    QString id() const { return m_id; }
    QString bookId() const { return m_bookId; }
    QString userId() const { return m_userId; }
    QString userDisplayName() const { return m_userDisplayName; }
    QString text() const { return m_text; }
    int stars() const { return m_stars; }
    QString createdAtText() const { return m_createdAt.toString(Qt::ISODate); }
    QString relativeTime() const { return m_createdAt.toString("MMM d"); }
    // BUG FIX (Issue 23): expose the raw QDateTime so BookDetailViewModel's
    // sort comparator can compare review timestamps for "newest"/"oldest"
    // sort modes. Previously only string-formatted getters existed, which
    // made proper chronological comparison impossible.
    const QDateTime& createdAt() const { return m_createdAt; }
    bool isEdited() const { return m_isEdited; }
    bool isPinned() const { return m_isPinned; }
    bool isFlagged() const { return m_isFlagged; }
    int helpfulCount() const { return m_helpfulCount; }
    int replyCount() const { return m_replyCount; }
    bool userFoundHelpful() const { return m_userFoundHelpful; }

    // Alias getters
    bool byCurrentUser() const { return m_byCurrentUser; }
    bool verifiedPurchase() const { return false; }

    void fromJson(const QJsonObject& j) {
        m_id              = j.value("id").toString();
        m_bookId          = j.value("bookId").toString();
        m_userId          = j.value("userId").toString();
        m_userDisplayName = j.value("userDisplayName").toString();
        m_text            = j.value("text").toString();
        m_stars           = j.value("stars").toInt();
        m_createdAt       = QDateTime::fromString(j.value("createdAt").toString(), Qt::ISODate);
        m_isEdited        = j.value("isEdited").toBool();
        // Issue 7: load helpful-mark + reply counters from the server
        // payload (reviewToJson adds these for every review response).
        m_helpfulCount    = j.value("helpfulCount").toInt(0);
        m_replyCount      = j.value("replyCount").toInt(0);
    }

    // Setters for local state mutations
    void setHelpfulCount(int n) { m_helpfulCount = n; }
    void setUserFoundHelpful(bool v) { m_userFoundHelpful = v; }
    void setPinned(bool v) { m_isPinned = v; }
    void setFlagged(bool v) { m_isFlagged = v; }
    void setReplyCount(int n) { m_replyCount = n; }
    void setByCurrentUser(bool v) { if (m_byCurrentUser != v) { m_byCurrentUser = v; emit byCurrentUserChanged(); } }

signals:
    void byCurrentUserChanged();

private:
    QString m_id, m_bookId, m_userId, m_userDisplayName, m_text;
    QDateTime m_createdAt;
    int m_stars = 0, m_helpfulCount = 0, m_replyCount = 0;
    bool m_isEdited = false, m_isPinned = false, m_isFlagged = false, m_userFoundHelpful = false;
    bool m_byCurrentUser = false;
};

} // namespace bookclub::client
