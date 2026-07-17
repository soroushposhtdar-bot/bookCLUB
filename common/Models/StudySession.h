#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include "common/AppEnums.h"

namespace bookclub::common {

class StudySession : public QObject {
    Q_OBJECT
public:
    explicit StudySession(QObject* parent = nullptr);
    StudySession(const QString& id, QObject* parent = nullptr);
    ~StudySession() override = default;

    const QString& id() const;
    const QString& bookId() const;
    const QString& hostUserId() const;
    const QStringList& participantUserIds() const;
    StudySessionState state() const;
    const QDateTime& createdAt() const;
    int currentPage() const;
    double zoomLevel() const;
    bool synced() const;

    void setId(const QString& id);
    void setBookId(const QString& bookId);
    void setHostUserId(const QString& hostUserId);
    void setParticipantUserIds(const QStringList& ids);
    void setState(StudySessionState state);
    void setCreatedAt(const QDateTime& createdAt);
    void setCurrentPage(int page);
    void setZoomLevel(double zoom);
    void setSynced(bool synced);

    void addParticipant(const QString& userId);
    void removeParticipant(const QString& userId);

signals:
    void sessionChanged();
    void participantJoined(const QString& userId);
    void participantLeft(const QString& userId);
    void pageChanged(int page);
    void zoomChanged(double zoom);

private:
    QString m_id;
    QString m_bookId;
    QString m_hostUserId;
    QStringList m_participantUserIds;
    StudySessionState m_state = StudySessionState::Created;
    QDateTime m_createdAt;
    int m_currentPage = 0;
    double m_zoomLevel = 1.0;
    bool m_synced = false;
};

} // namespace bookclub::common
