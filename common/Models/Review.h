#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>

namespace bookclub::common {

class Review : public QObject {
    Q_OBJECT
public:
    explicit Review(QObject* parent = nullptr);
    Review(const QString& id, QObject* parent = nullptr);
    ~Review() override = default;

    const QString& id() const;
    const QString& bookId() const;
    const QString& userId() const;
    const QString& userDisplayName() const;
    const QString& text() const;
    const QDateTime& createdAt() const;
    const QDateTime& updatedAt() const;
    int stars() const;
    bool isEdited() const;
    // Issue 7: helpful-mark + reply counters backed by the Reviews
    // table columns `helpfulCount` and `replyCount`. The replies
    // themselves are not modelled as a separate table — we just
    // keep a count + (optionally) the rendered reply strings.
    int helpfulCount() const;
    int replyCount() const;
    QStringList replies() const;

    void setId(const QString& id);
    void setBookId(const QString& bookId);
    void setUserId(const QString& userId);
    void setUserDisplayName(const QString& name);
    void setText(const QString& text);
    void setCreatedAt(const QDateTime& createdAt);
    void setUpdatedAt(const QDateTime& updatedAt);
    void setStars(int stars);
    void setEdited(bool edited);
    void setHelpfulCount(int count);
    void setReplyCount(int count);
    void setReplies(const QStringList& replies);

signals:
    void reviewChanged();

private:
    QString m_id;
    QString m_bookId;
    QString m_userId;
    QString m_userDisplayName;
    QString m_text;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    int m_stars = 0;
    bool m_edited = false;
    int m_helpfulCount = 0;
    int m_replyCount = 0;
    QStringList m_replies;
};

} // namespace bookclub::common
