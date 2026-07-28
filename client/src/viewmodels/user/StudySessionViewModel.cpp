// =============================================================================
//  StudySessionViewModel.cpp
// =============================================================================
//  Real socket-backed implementation. Talks to the StudySessionManager via
//  the matching socket commands (CreateStudySession, JoinStudySession, …).
//
//  - `rooms` is fetched from the server via GetStudySessions (Bug 2). The
//    view-model also re-fetches whenever the server pushes EvtSessionListUpdated.
//  - `createRoom / joinRoom / leaveRoom` hit the matching socket commands.
//  - `sendMessage` posts SendStudyMessage to the server, which broadcasts
//    EvtStudyMessage to every participant (Bug 3). The sender's own message
//    is added locally for instant feedback, and the server echo is ignored
//    by checking `userId == currentUserId` in the EvtStudyMessage handler.
//  - `participants` is built from the JoinStudySession response payload
//    (Bug 7: enriched with displayName / initials / color / page / isHost).
//    Live join/leave updates arrive via EvtStudyParticipantChanged (Bug 8).
//  - `notes` is fetched via GetStudyNotes on joinRoom and updated live via
//    EvtStudyNote (Bug 5).
//  - `inviteUsers` posts InviteToStudySession to the server (Bug 4).
// =============================================================================
#include "viewmodels/user/StudySessionViewModel.h"
#include "services/NetworkService.h"
#include "services/AuthService.h"
#include "common/Network/Protocol.h"
#include "common/AppEnums.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

namespace {

QString nowTimeStamp() {
    return QDateTime::currentDateTime().toString("HH:mm");
}

QVariantMap makeRoom(const QString& id,
                     const QString& name,
                     const QString& bookId,
                     const QString& bookTitle,
                     const QString& host,
                     int participants,
                     int capacity,
                     int page,
                     int pageCount,
                     const QString& privacy,
                     bool live,
                     const QString& color) {
    QVariantMap r;
    r["id"]          = id;
    r["name"]        = name;
    r["bookId"]      = bookId;
    r["bookTitle"]   = bookTitle;
    r["host"]        = host;
    r["participants"]= participants;
    r["capacity"]    = capacity;
    r["page"]        = page;
    r["pageCount"]   = pageCount;
    r["privacy"]     = privacy;
    r["live"]        = live;
    r["color"]       = color;
    return r;
}

QVariantMap makeChat(const QString& user,
                     const QString& initials,
                     const QString& color,
                     const QString& text,
                     const QString& time,
                     bool self) {
    QVariantMap m;
    m["user"]     = user;
    m["initials"] = initials;
    m["color"]    = color;
    m["text"]     = text;
    m["time"]     = time;
    m["self"]     = self;
    return m;
}

QVariantMap makeParticipant(const QString& name,
                            const QString& initials,
                            const QString& color,
                            int page,
                            int pageCount,
                            bool online,
                            bool isHost,
                            const QString& userId = QStringLiteral("")) {
    QVariantMap p;
    // BUG FIX (Issue 30): include `userId` so the EvtStudySync handler
    // can match incoming page-sync events to the right participant.
    p["userId"]    = userId;
    p["name"]      = name;
    p["initials"]  = initials;
    p["color"]     = color;
    p["page"]      = page;
    p["pageCount"] = pageCount;
    p["online"]    = online;
    p["isHost"]    = isHost;
    return p;
}

QVariantMap makeNote(const QString& author,
                     const QString& initials,
                     const QString& color,
                     const QString& text,
                     int page,
                     const QString& time) {
    QVariantMap n;
    n["author"]   = author;
    n["initials"] = initials;
    n["color"]    = color;
    n["text"]     = text;
    n["page"]     = page;
    n["time"]     = time;
    return n;
}

} // namespace

StudySessionViewModel::StudySessionViewModel(QObject* parent)
    : UserViewModelBase(parent)
{
    // Subscribe to EvtStudySync so participants' page numbers update live.
    NetworkService::instance().subscribeEvent(common::Command::EvtStudySync, this,
        [this](const common::Message& msg) {
            const QString sessionId = msg.payload().value("sessionId").toString();
            const QString userId    = msg.payload().value("userId").toString();
            const int page          = msg.payload().value("page").toInt(-1);
            if (sessionId != m_currentRoomId || page < 0) return;

            // Update the participant's page in the local cache.
            for (QVariant& v : m_participants) {
                QVariantMap p = v.toMap();
                if (p.value("userId").toString() == userId) {
                    p["page"] = page;
                    v = p;
                    emit participantsChanged();
                    return;
                }
            }
        });

    // Bug 2: refresh the room list whenever the server says it changed
    // (a new session was created or one was closed).
    NetworkService::instance().subscribeEvent(common::Command::EvtSessionListUpdated, this,
        [this](const common::Message&) {
            refresh();
        });

    // Bug 3: append chat messages pushed by the server. We skip our own
    // messages because sendMessage() already appended them locally for
    // instant feedback.
    NetworkService::instance().subscribeEvent(common::Command::EvtStudyMessage, this,
        [this](const common::Message& msg) {
            const QJsonObject p = msg.payload();
            const QString sessionId = p.value("sessionId").toString();
            if (sessionId != m_currentRoomId) return;
            const QString userId = p.value("userId").toString();
            if (userId == AuthService::instance().currentUserId()) return;

            const QString displayName = p.value("displayName").toString();
            m_chatMessages.append(makeChat(
                displayName,
                StudySessionViewModel::initialsFromName(displayName),
                QStringLiteral("#1A73E8"),
                p.value("text").toString(),
                p.value("timestamp").toString(),
                false));
            emit chatChanged();
        });

    // Bug 5: append shared notes pushed by the server. The local addNote
    // call also fires this event from the server (echo), so we skip our
    // own echoes by userId — we already added the note locally for
    // instant feedback. Notes from other participants are appended as
    // soon as they arrive.
    NetworkService::instance().subscribeEvent(common::Command::EvtStudyNote, this,
        [this](const common::Message& msg) {
            const QJsonObject p = msg.payload();
            const QString sessionId = p.value("sessionId").toString();
            if (sessionId != m_currentRoomId) return;
            const QString userId = p.value("userId").toString();
            // Skip the echo of our own note — addNote() already appended
            // it locally for instant feedback.
            if (userId == AuthService::instance().currentUserId()) return;

            const QString noteId = p.value("id").toString();
            // Defensive dedup: skip if we already have a note with this id.
            for (const QVariant& v : m_notes) {
                if (v.toMap().value("id").toString() == noteId) return;
            }

            const QString author = p.value("displayName").toString();
            QVariantMap note = makeNote(
                author,
                StudySessionViewModel::initialsFromName(author),
                QStringLiteral("#1A73E8"),
                p.value("text").toString(),
                p.value("page").toInt(),
                p.value("timestamp").toString());
            note["id"] = noteId;
            m_notes.append(note);
            emit notesChanged();
        });

    // Bug 8: live participant list updates when users join or leave.
    NetworkService::instance().subscribeEvent(common::Command::EvtStudyParticipantChanged, this,
        [this](const common::Message& msg) {
            const QJsonObject p = msg.payload();
            const QString sessionId = p.value("sessionId").toString();
            if (sessionId != m_currentRoomId) return;

            m_participants.clear();
            const QJsonArray arr = p.value("participants").toArray();
            for (const auto& v : arr) {
                const QJsonObject obj = v.toObject();
                m_participants.append(makeParticipant(
                    obj.value("displayName").toString(),
                    obj.value("initials").toString(),
                    obj.value("color").toString("#1A73E8"),
                    obj.value("page").toInt(),
                    obj.value("pageCount").toInt(),
                    obj.value("online").toBool(true),
                    obj.value("isHost").toBool(false),
                    obj.value("userId").toString()));
            }
            emit participantsChanged();
        });
}

void StudySessionViewModel::loadRoomState(const QString& roomId) {
    // No-op for the socket-backed version — participants come from
    // JoinStudySession's response. Kept for ABI compatibility with the
    // header declaration.
    Q_UNUSED(roomId)
}

// -----------------------------------------------------------------------------
//  Q_INVOKABLE surface
// -----------------------------------------------------------------------------

// Bug 2: fetch the active-session list from the server and rebuild m_rooms.
void StudySessionViewModel::refresh() {
    QJsonObject p;
    NetworkService::instance().sendRequestAsync(common::Command::GetStudySessions, p, this,
        [this](const NetworkService::Response& resp) {
            if (!resp.isSuccess()) return;
            QVariantList rooms;
            const QJsonArray arr = resp.payload.value("sessions").toArray();
            for (const auto& v : arr) {
                const QJsonObject obj = v.toObject();
                const QString bookId = obj.value("bookId").toString();
                const QString hostId = obj.value("hostUserId").toString();
                // The server summary doesn't include bookTitle / host display
                // name; fall back to ids so the UI still renders something.
                rooms.append(makeRoom(
                    obj.value("sessionId").toString(),
                    QStringLiteral("Room ") + obj.value("sessionId").toString().left(6),
                    bookId,
                    bookId,
                    hostId,
                    obj.value("participantCount").toInt(),
                    8,  // capacity — not in summary, use default
                    obj.value("currentPage").toInt(),
                    0,
                    QStringLiteral("public"),
                    obj.value("state").toInt() == static_cast<int>(common::StudySessionState::Active),
                    QStringLiteral("#1A73E8")));
            }
            m_rooms = rooms;
            emit roomsChanged();
        });
}

void StudySessionViewModel::createRoom(const QString& name,
                                       const QString& bookId,
                                       const QString& bookTitle,
                                       const QString& privacy,
                                       int capacity) {
    QJsonObject p;
    p["name"]      = name;
    p["bookId"]    = bookId;
    p["bookTitle"] = bookTitle;
    p["privacy"]   = privacy;
    p["capacity"]  = capacity;

    auto resp = NetworkService::instance().sendRequest(common::Command::CreateStudySession, p);
    if (!resp.isSuccess()) {
        // Bug 6: emit failure signal so the QML dialog can show an error
        // and stay open for retry.
        emit roomCreateFailed(resp.errorMessage.isEmpty()
                              ? QStringLiteral("Could not create the room. Please try again.")
                              : resp.errorMessage);
        return;
    }

    const QString id = resp.payload.value("sessionId").toString();
    if (id.isEmpty()) {
        emit roomCreateFailed(QStringLiteral("Server returned an empty session id."));
        return;
    }

    QVariantMap room = makeRoom(
        id, name, bookId, bookTitle,
        AuthService::instance().currentDisplayName(),
        1, capacity, 0, 0, privacy, true, "#1A73E8");
    m_rooms.prepend(room);
    emit roomsChanged();

    // Bug 6: tell the QML dialog the room was created so it can close
    // and show a success toast.
    emit roomCreated(id);

    // Auto-join the newly-created room.
    joinRoom(id);
}

void StudySessionViewModel::joinRoom(const QString& roomId) {
    QJsonObject p;
    p["sessionId"] = roomId;
    auto resp = NetworkService::instance().sendRequest(common::Command::JoinStudySession, p);
    if (!resp.isSuccess()) return;

    m_currentRoomId = roomId;
    emit currentRoomChanged();

    // Bug 7: build participants list from the enriched response payload.
    m_participants.clear();
    const QJsonArray parts = resp.payload.value("participants").toArray();
    for (const auto& v : parts) {
        const QJsonObject obj = v.toObject();
        m_participants.append(makeParticipant(
            obj.value("displayName").toString(),
            obj.value("initials").toString(),
            obj.value("color").toString("#1A73E8"),
            obj.value("page").toInt(),
            obj.value("pageCount").toInt(),
            obj.value("online").toBool(true),
            obj.value("isHost").toBool(false),
            obj.value("userId").toString()
        ));
    }
    emit participantsChanged();

    // Clear chat for the new room (server has no chat history endpoint).
    m_chatMessages.clear();
    emit chatChanged();

    // Bug 5: fetch the room's existing notes so the panel renders
    // history immediately on join.
    QJsonObject notesReq;
    notesReq["sessionId"] = roomId;
    auto notesResp = NetworkService::instance().sendRequest(common::Command::GetStudyNotes, notesReq);
    if (notesResp.isSuccess()) {
        QVariantList notes;
        const QJsonArray arr = notesResp.payload.value("notes").toArray();
        for (const auto& v : arr) {
            const QJsonObject obj = v.toObject();
            const QString author = obj.value("displayName").toString();
            QVariantMap note = makeNote(
                author,
                StudySessionViewModel::initialsFromName(author),
                QStringLiteral("#1A73E8"),
                obj.value("text").toString(),
                obj.value("page").toInt(),
                obj.value("timestamp").toString());
            note["id"] = obj.value("id").toString();
            notes.append(note);
        }
        m_notes = notes;
        emit notesChanged();
    } else {
        m_notes.clear();
        emit notesChanged();
    }
}

void StudySessionViewModel::leaveRoom() {
    if (m_currentRoomId.isEmpty()) return;
    QJsonObject p;
    p["sessionId"] = m_currentRoomId;
    NetworkService::instance().sendAsync(common::Command::LeaveStudySession, p);
    m_currentRoomId.clear();
    m_chatMessages.clear();
    m_participants.clear();
    m_notes.clear();
    emit currentRoomChanged();
    emit chatChanged();
    emit participantsChanged();
    emit notesChanged();
}

// Bug 3: send chat via the server. The server broadcasts EvtStudyMessage
// to every participant; we add the message locally for instant feedback
// and skip the echo in the EvtStudyMessage handler.
void StudySessionViewModel::sendMessage(const QString& text) {
    if (text.trimmed().isEmpty()) return;
    if (m_currentRoomId.isEmpty()) return;

    const QString displayName = AuthService::instance().currentDisplayName();
    QVariantMap msg = makeChat(
        displayName,
        StudySessionViewModel::initialsFromName(displayName),
        "#9C27B0",
        text,
        nowTimeStamp(),
        true);
    m_chatMessages.append(msg);
    emit chatChanged();

    QJsonObject p;
    p["sessionId"] = m_currentRoomId;
    p["text"]      = text;
    NetworkService::instance().sendAsync(common::Command::SendStudyMessage, p);
}

// Bug 4: send invites via the server.
void StudySessionViewModel::inviteUsers(const QStringList& usernames) {
    if (m_currentRoomId.isEmpty() || usernames.isEmpty()) return;

    QJsonArray arr;
    for (const QString& u : usernames) arr.append(u);

    QJsonObject p;
    p["sessionId"]    = m_currentRoomId;
    p["usernames"]    = arr;
    p["targetUserIds"] = arr;  // server accepts either; send both for compat
    NetworkService::instance().sendAsync(common::Command::InviteToStudySession, p);
}

// Bug 5: send notes via the server. The server broadcasts EvtStudyNote
// to every participant; we add the note locally for instant feedback and
// deduplicate by id in the EvtStudyNote handler.
void StudySessionViewModel::addNote(const QString& text, int page) {
    if (text.trimmed().isEmpty()) return;
    if (m_currentRoomId.isEmpty()) return;

    const QString displayName = AuthService::instance().currentDisplayName();
    const QString noteId = QStringLiteral("local-%1").arg(QDateTime::currentMSecsSinceEpoch());
    QVariantMap note = makeNote(
        displayName,
        StudySessionViewModel::initialsFromName(displayName),
        "#9C27B0",
        text,
        page,
        nowTimeStamp());
    note["id"] = noteId;
    m_notes.append(note);
    emit notesChanged();

    QJsonObject p;
    p["sessionId"] = m_currentRoomId;
    p["text"]      = text;
    p["page"]      = page;
    NetworkService::instance().sendAsync(common::Command::AddStudyNote, p);
}

QString StudySessionViewModel::initialsFromName(const QString& name) {
    if (name.isEmpty()) return QStringLiteral("?");
    QStringList parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return QStringLiteral("?");
    QString result;
    for (const QString& p : parts) {
        if (!p.isEmpty()) result.append(p.left(1).toUpper());
        if (result.length() >= 2) break;
    }
    return result;
}

} // namespace bookclub::client
