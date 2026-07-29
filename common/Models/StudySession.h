#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>

#include "common/AppEnums.h"

namespace bookclub::common {

// A single shared note attached to a StudySession. Notes are authored by
// a participant on a specific page; the server broadcasts new notes via
// EvtStudyNote so every participant's notes panel updates in real time.
struct NoteEntry {
    QString id;
    QString userId;
    QString displayName;
    QString text;
    int page = 0;
    QDateTime timestamp;
};

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

    // Shared per-room notes. addNote appends and returns the new entry's
    // index; notes() exposes the list for serialisation on the server side.
    void addNote(const NoteEntry& entry);
    const QList<NoteEntry>& notes() const;

signals:
    void sessionChanged();
    void participantJoined(const QString& userId);
    void participantLeft(const QString& userId);
    void pageChanged(int page);
    void zoomChanged(double zoom);
    void noteAdded(const NoteEntry& entry);

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

    QList<NoteEntry> m_notes;
};

} // namespace bookclub::common
