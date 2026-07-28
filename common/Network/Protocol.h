// common/Network/Protocol.h
//
// Wire protocol vocabulary for BookCLUB.
//
// Conventions:
//   - Commands in the range [1, 999]  are client→server REQUESTS.
//     The server replies with the SAME command value and a Status code.
//   - Commands in the range [1000+]   are server→client PUSHED EVENTS.
//     These are unsolicited; the client should register a handler for each
//     event type it cares about. The server never expects a reply.
//   - Command::Invalid (0) is reserved as a sentinel.
#pragma once

#include <QString>
#include <QMetaType>

namespace bookclub::common {

enum class Command : quint16 {
    Invalid = 0,

    // ===== Auth (1-9) =====
    Login = 1,
    Register,
    ResetPassword,
    ChangePassword,
    Logout,

    // ===== Book Catalog (10-19) =====
    GetHomeSections,
    SearchBooks,
    GetBookDetails,
    GetBooksByIds,            // batch fetch by list of IDs (perf)

    // ===== Cart (20-29) =====
    AddToCart,
    RemoveFromCart,
    GetCart,
    Checkout,
    ApplyDiscount,
    ClearCart,

    // ===== Library / Shelves (30-39) =====
    GetLibrary,
    GetPurchasedBooks,
    CreateShelf,
    DeleteShelf,
    AddBookToShelf,
    RemoveBookFromShelf,
    RenameShelf,

    // ===== Reviews + Ratings (40-49) =====
    SubmitReview,
    UpdateReview,
    DeleteReview,
    SetRating,
    // Issue 7: helpful-mark + reply counters. The client fires these
    // from the BookDetailViewModel; the server increments the matching
    // column on the Reviews row and replies with Success.
    MarkReviewHelpful,
    AddReviewReply,

    // ===== Wishlist (50-59) =====
    ToggleWishlist,
    GetWishlist,

    // ===== Publisher (60-79) =====
    GetPublisherBooks,
    PublishBook,
    UpdateBook,
    DeactivateBook,
    ActivateBook,
    ApplyTimedDiscount,
    GetPublisherAnalytics,

    // ===== Admin (80-99) =====
    GetUsersList,
    BlockUser,
    UnblockUser,
    DeleteUser,
    ModerateBook,
    RemoveBookByAdmin,
    AdminDeleteReview,       // 86 — admin-force-delete any review
    AdminApproveReview,      // 87 — admin pin/approve a review (unflag)

    // ===== Notifications (100-109) =====
    GetNotifications,
    MarkNotificationRead,
    MarkAllNotificationsRead,

    // ===== Study Sessions (110-119) =====
    CreateStudySession,
    JoinStudySession,
    LeaveStudySession,
    SyncStudyPage,
    // Explicit values so future additions in this block don't shift older
    // codes — the e2e test, the QML client, and any persisted sessions all
    // depend on these staying stable.
    GetStudySessions      = 114,   // client→server: fetch the active-session list
    SendStudyMessage      = 115,   // client→server: in-room chat message
    InviteToStudySession  = 116,   // client→server: invite users to a session
    AddStudyNote          = 117,   // client→server: append a shared note
    GetStudyNotes         = 118,   // client→server: fetch shared notes for a session

    // ===== Server / Operator (120-129) =====
    GetServerHealth,
    GetServerLogs,
    GetServerClients,

    // ===== User profile (130-139) =====
    GetCurrentUser,
    UpdateProfile,
    SaveFavoriteGenres,

    // ===== Genres (140-149) =====
    GetGenres,

    // ===== Authors (150-159) =====
    GetAuthors,

    // ===== PDF file transfer (160-169) =====
    // v14: in-app PDF reader. The publisher uploads a book's PDF bytes
    // (base64-encoded) so the server can store them; clients download
    // the bytes when they open the book in the reader.
    UploadBookPdf,            // publisher → server: base64 PDF bytes for a book
    DownloadBookPdf,          // client → server: fetch base64 PDF bytes for a book

    // ===== Shelf metadata update (170-179) =====
    // v15e: update a shelf's color / favorite / private / sortOrder.
    // The payload includes shelfId + the fields to update.
    UpdateShelf,

    // ===== Server-pushed events (1000+) =====
    // These are fired by the server without a preceding client request.
    // The client registers a handler via ClientNetworkManager::registerRequestHandler
    // for the matching EVT_* command and reacts in real time.
    EvtNotification = 1000,       // payload: Notification JSON
    EvtReviewUpdated,             // payload: { bookId, review }
    EvtStudySync,                 // payload: { sessionId, page, userId }
    EvtBookAdded,                 // payload: Book JSON (new book in favourite genre)
    EvtDiscountApplied,           // payload: { bookId, discountValue, endsAt }
    EvtUserBlocked,               // payload: { userId }   (admin pushed to user)
    EvtServerShutdown,            // payload: { reason }
    // Study-session events. Explicit values keep the protocol stable across
    // future Protocol.h edits — the client subscribes by enum value, not by
    // name, so reordering must never renumber these.
    EvtSessionListUpdated      = 1007,   // payload: { sessions: [...] } — broadcast on create/close
    EvtStudyMessage            = 1008,   // payload: { sessionId, userId, displayName, text, timestamp }
    EvtStudyNote               = 1009,   // payload: { sessionId, id, userId, displayName, text, page, timestamp }
    EvtStudyParticipantChanged = 1010,   // payload: { sessionId, event, participants: [...] }
};

// HTTP-like status codes used in responses.
enum class Status : quint16 {
    Success = 200,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    Conflict = 409,
    ValidationError = 422,
    InternalError = 500,
    ServiceUnavailable = 503
};

// True for any server-pushed event command.
inline bool isEventCommand(Command cmd) {
    return static_cast<quint16>(cmd) >= 1000;
}

QString commandToString(Command cmd);
QString statusToString(Status status);

} // namespace bookclub::common

Q_DECLARE_METATYPE(bookclub::common::Command)
Q_DECLARE_METATYPE(bookclub::common::Status)
