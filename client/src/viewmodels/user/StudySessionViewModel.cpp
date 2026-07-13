// =============================================================================
//  StudySessionViewModel.cpp
// =============================================================================
//  All data is in-memory mock. The shape of each QVariantMap mirrors what
//  GroupReadingPage.qml's ListElements expose today, so the QML can swap its
//  ListModel bindings for viewModel properties with minimal churn.
// =============================================================================
#include "viewmodels/user/StudySessionViewModel.h"

#include <QDateTime>
#include <QStringList>

namespace bookclub::client {

namespace {

// Generate a "HH:mm" timestamp for "now" (used by sendMessage).
QString nowTimeStamp() {
    return QDateTime::currentDateTime().toString("HH:mm");
}

// Build a room QVariantMap with sensible defaults for the fields the UI reads.
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
                            bool isHost) {
    QVariantMap p;
    p["name"]     = name;
    p["initials"] = initials;
    p["color"]    = color;
    p["page"]     = page;
    p["pageCount"]= pageCount;
    p["online"]   = online;
    p["isHost"]   = isHost;
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
    // ----- Seed 5 mock rooms (same data GroupReadingPage had as ListElements) -----
    m_rooms = {
        makeRoom("room-1", "Midnight Library Club",   "demo1", "The Midnight Library",     "Alice R.", 6, 12,  42, 304, "public",  true,  "#1A73E8"),
        makeRoom("room-2", "Sci-Fi Saturdays",        "demo2", "Project Hail Mary",        "Bob B.",   4,  8, 118, 476, "public",  true,  "#1E8E3E"),
        makeRoom("room-3", "Quiet Reads",             "demo3", "Atomic Habits",            "Carol M.", 3,  5,  24, 320, "private", false, "#F29900"),
        makeRoom("room-4", "Mystery Mondays",         "demo4", "The Silent Patient",       "David K.", 8, 10,  88, 336, "public",  true,  "#9C27B0"),
        makeRoom("room-5", "Page Turners",            "demo5", "Where the Crawdads Sing",  "Eve L.",   5,  8, 156, 384, "public",  false, "#D93025"),
    };

    // ----- Seed mock chat + participants + notes for the first room -----
    // (GroupReadingPage shows these immediately, so the VM must too.)
    loadRoomState("room-1");
}

void StudySessionViewModel::loadRoomState(const QString& roomId) {
    // ----- Chat (mocked — same lines the QML ListModel had) -----
    m_chatMessages = {
        makeChat("Alice R.", "A", "#1A73E8", "Just finished chapter 3 — that ending!", "14:32", false),
        makeChat("Bob B.",   "B", "#1E8E3E", "Right? I had to re-read it twice.",       "14:34", false),
        makeChat("Carol M.", "C", "#F29900", "Wait until you see chapter 5.",           "14:36", false),
        makeChat("You",      "Y", "#9C27B0", "Turning to page 42 now.",                 "14:38", true ),
        makeChat("Alice R.", "A", "#1A73E8", "We're all on page 42 — perfect sync.",    "14:39", false),
        makeChat("Bob B.",   "B", "#1E8E3E", "Let's pause here and discuss on Friday.", "14:42", false),
    };

    // ----- Participants (mocked — derived from the chat cast + a couple extras) -----
    m_participants = {
        makeParticipant("Alice R.",  "A", "#1A73E8",  42, 304, true,  true ),
        makeParticipant("Bob B.",    "B", "#1E8E3E",  42, 304, true,  false),
        makeParticipant("Carol M.",  "C", "#F29900",  40, 304, true,  false),
        makeParticipant("David K.",  "D", "#9C27B0",  38, 304, false, false),
        makeParticipant("Eve L.",    "E", "#D93025",  42, 304, true,  false),
        makeParticipant("Frank P.",  "F", "#7B1FA2",  35, 304, false, false),
    };

    // ----- Shared notes (mocked) -----
    m_notes = {
        makeNote("Alice R.", "A", "#1A73E8", "Nora's regret motif starts on page 12 — keep an eye on it.", 12, "14:30"),
        makeNote("Bob B.",   "B", "#1E8E3E", "The library = possibility space. Each book = a life unlived.", 42, "14:35"),
        makeNote("Carol M.", "C", "#F29900", "Chapter 5 cliffhanger — discuss Friday.", 88, "14:40"),
    };

    // Silence "unused roomId" — the mock always returns the same chat set.
    // (In a real impl, this method would look up per-room state by ID.)
    Q_UNUSED(roomId)
}

// -----------------------------------------------------------------------------
//  Q_INVOKABLE surface
// -----------------------------------------------------------------------------

void StudySessionViewModel::refresh() {
    // Re-emit every signal so QML re-reads all properties. Useful after a
    // service reconnect or a language switch.
    emit roomsChanged();
    emit chatChanged();
    emit participantsChanged();
    emit currentRoomChanged();
    emit notesChanged();
}

void StudySessionViewModel::createRoom(const QString& name,
                                       const QString& bookId,
                                       const QString& bookTitle,
                                       const QString& privacy,
                                       int capacity) {
    const QString id = "room-" + QString::number(m_rooms.size() + 1);
    QVariantMap room = makeRoom(
        id,
        name,
        bookId,
        bookTitle,
        "You",            // host = the current user
        1,                // just the host for now
        capacity > 0 ? capacity : 8,
        1,                // start on page 1
        300,              // placeholder page count
        privacy.isEmpty() ? QStringLiteral("public") : privacy,
        true,             // newly created rooms are live
        "#1A73E8");       // default accent
    m_rooms.append(room);

    // Auto-join the freshly-created room.
    m_currentRoomId = id;
    loadRoomState(id);
    // Reset chat/participants for a brand-new room — host is alone.
    m_chatMessages.clear();
    m_participants = {
        makeParticipant("You", "Y", "#9C27B0", 1, 300, true, true),
    };
    m_notes.clear();

    emit roomsChanged();
    emit currentRoomChanged();
    emit chatChanged();
    emit participantsChanged();
    emit notesChanged();
}

void StudySessionViewModel::joinRoom(const QString& roomId) {
    if (m_currentRoomId == roomId) return;
    m_currentRoomId = roomId;
    loadRoomState(roomId);

    emit currentRoomChanged();
    emit chatChanged();
    emit participantsChanged();
    emit notesChanged();
}

void StudySessionViewModel::leaveRoom() {
    if (m_currentRoomId.isEmpty()) return;
    m_currentRoomId.clear();
    m_chatMessages.clear();
    m_participants.clear();
    m_notes.clear();

    emit currentRoomChanged();
    emit chatChanged();
    emit participantsChanged();
    emit notesChanged();
}

void StudySessionViewModel::sendMessage(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;
    m_chatMessages.append(makeChat("You", "Y", "#9C27B0", trimmed, nowTimeStamp(), true));
    emit chatChanged();
}

void StudySessionViewModel::inviteUsers(const QStringList& usernames) {
    // Mock — a real impl would POST to /study/invite and the backend would
    // push a join notification to each invitee. Here we just no-op (the UI
    // shows a toast on its own; nothing in the VM state changes).
    Q_UNUSED(usernames)
}

void StudySessionViewModel::addNote(const QString& text, int page) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return;
    m_notes.prepend(makeNote("You", "Y", "#9C27B0", trimmed, page, nowTimeStamp()));
    emit notesChanged();
}

QVariantList StudySessionViewModel::notes() const {
    return m_notes;
}

} // namespace bookclub::client
