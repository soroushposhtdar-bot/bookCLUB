// =============================================================================
//  StudySessionViewModel.h
// =============================================================================
//  MVVM view-model for the Group Reading / Study Session page.
//
//  Backs GroupReadingPage.qml with server-backed state via NetworkService.
//
//  Surface:
//      • rooms         — list of active reading rooms (Bug 2: fetched from server)
//      • chatMessages  — chat for the currently-joined room (Bug 3: server-backed)
//      • participants  — who's online in the current room (Bug 8: live updates)
//      • notes         — shared per-room notes (Bug 5: server-backed)
//      • currentRoomId — "" when not in a session
//      • inSession     — convenience bool derived from currentRoomId
//
//  Actions:
//      • createRoom / joinRoom / leaveRoom
//      • sendMessage / inviteUsers
//      • addNote
//
//  Signals:
//      • roomCreated(roomId)       — emitted on successful createRoom (Bug 6)
//      • roomCreateFailed(reason)  — emitted on createRoom failure   (Bug 6)
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
    Q_PROPERTY(QVariantList notes         READ notes         NOTIFY notesChanged)
    Q_PROPERTY(QString      currentRoomId READ currentRoomId NOTIFY currentRoomChanged)
    Q_PROPERTY(bool         inSession     READ inSession     NOTIFY currentRoomChanged)

public:
    explicit StudySessionViewModel(QObject* parent = nullptr);

    const QVariantList& rooms() const { return m_rooms; }
    const QVariantList& chatMessages() const { return m_chatMessages; }
    const QVariantList& participants() const { return m_participants; }
    // Bug 1: notes is now a Q_PROPERTY (was Q_INVOKABLE). The getter returns
    // a const ref so QML can bind to it reactively.
    const QVariantList& notes() const { return m_notes; }
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

signals:
    void roomsChanged();
    void chatChanged();
    void participantsChanged();
    void currentRoomChanged();
    void notesChanged();

    // Bug 6: explicit success/failure signals so the QML create-room dialog
    // can react deterministically instead of guessing from list length.
    void roomCreated(QString roomId);
    void roomCreateFailed(QString reason);

private:
    void loadRoomState(const QString& roomId);
    static QString initialsFromName(const QString& name);

    QVariantList m_rooms;
    QVariantList m_chatMessages;
    QVariantList m_participants;
    QVariantList m_notes;

    QString m_currentRoomId;
};

} // namespace bookclub::client

#endif // STUDYSESSIONVIEWMODEL_H
