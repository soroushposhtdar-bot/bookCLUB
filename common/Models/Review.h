#pragma once

#include <QObject>
#include <QString>
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

    void setId(const QString& id);
    void setBookId(const QString& bookId);
    void setUserId(const QString& userId);
    void setUserDisplayName(const QString& name);
    void setText(const QString& text);
    void setCreatedAt(const QDateTime& createdAt);
    void setUpdatedAt(const QDateTime& updatedAt);
    void setStars(int stars);
    void setEdited(bool edited);

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
};

} // namespace bookclub::common
