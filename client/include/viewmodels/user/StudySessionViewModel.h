// =============================================================================
//  StudySessionViewModel.h
// =============================================================================
//  MVVM view-model for the Group Reading / Study Session page.
//
//  Backs GroupReadingPage.qml with real ViewModel state instead of local
//  ListModels. All data is in-memory mock — the real backend would broadcast
//  over the StudySession socket protocol (see common/Network/Protocol.h).
//
//  Surface:
//      • rooms         — list of active reading rooms
//      • chatMessages  — chat for the currently-joined room
//      • participants  — who's online in the current room
//      • currentRoomId — "" when not in a session
//      • inSession     — convenience bool derived from currentRoomId
//
//  Actions:
//      • createRoom / joinRoom / leaveRoom
//      • sendMessage / inviteUsers
//      • addNote / notes  (shared per-room notes)
// =============================================================================
#ifndef STUDYSESSIONVIEWMODEL_H
#define STUDYSESSIONVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class StudySessionViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList rooms         READ rooms         NOTIFY roomsChanged)
    Q_PROPERTY(QVariantList chatMessages  READ chatMessages  NOTIFY chatChanged)
    Q_PROPERTY(QVariantList participants  READ participants  NOTIFY participantsChanged)
    Q_PROPERTY(QString      currentRoomId READ currentRoomId NOTIFY currentRoomChanged)
    Q_PROPERTY(bool         inSession     READ inSession     NOTIFY currentRoomChanged)

public:
    explicit StudySessionViewModel(QObject* parent = nullptr);

    const QVariantList& rooms() const { return m_rooms; }
    const QVariantList& chatMessages() const { return m_chatMessages; }
    const QVariantList& participants() const { return m_participants; }
    const QString& currentRoomId() const { return m_currentRoomId; }
    bool inSession() const { return !m_currentRoomId.isEmpty(); }

    Q_INVOKABLE void refresh();

    // Room lifecycle
    Q_INVOKABLE void createRoom(const QString& name,
                                const QString& bookId,
                                const QString& bookTitle,
                                const QString& privacy,
                                int capacity);
    Q_INVOKABLE void joinRoom(const QString& roomId);
    Q_INVOKABLE void leaveRoom();

    // In-room actions
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void inviteUsers(const QStringList& usernames);

    // Shared per-room notes
    Q_INVOKABLE void addNote(const QString& text, int page);
    Q_INVOKABLE QVariantList notes() const;

signals:
    void roomsChanged();
    void chatChanged();
    void participantsChanged();
    void currentRoomChanged();
    void notesChanged();

private:
    // Loads mock chat + participants + notes for `roomId` into the
    // active state and emits the corresponding signals.
    void loadRoomState(const QString& roomId);

    QVariantList m_rooms;
    QVariantList m_chatMessages;
    QVariantList m_participants;
    QVariantList m_notes;

    QString m_currentRoomId;
};

} // namespace bookclub::client

#endif // STUDYSESSIONVIEWMODEL_H
