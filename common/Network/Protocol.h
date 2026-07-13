// TODO: common/Network/Protocol.h
// common/Network/Protocol.h
#pragma once

#include <QString>
#include <QMetaType>

namespace bookclub::common {

// دستورات ارسالی از سمت کلاینت به سرور
enum class Command : quint16 {
    Invalid = 0,

    // Auth
    Login,
    Register,
    ResetPassword,
    ChangePassword,
    Logout,

    // Book Catalog
    GetHomeSections,
    SearchBooks,
    GetBookDetails,

    // Cart
    AddToCart,
    RemoveFromCart,
    GetCart,
    Checkout,
    ApplyDiscount,

    // Library
    GetLibrary,
    GetPurchasedBooks,
    CreateShelf,
    DeleteShelf,
    AddBookToShelf,
    RemoveBookFromShelf,

    // Publisher
    GetPublisherBooks,
    PublishBook,
    UpdateBook,
    DeactivateBook,
    ActivateBook,
    ApplyTimedDiscount,
    GetPublisherAnalytics,

    // Admin
    GetUsersList,
    BlockUser,
    UnblockUser,
    DeleteUser,
    ModerateBook,
    RemoveBookByAdmin,

    // Notifications
    GetNotifications,
    MarkNotificationRead,
    MarkAllNotificationsRead,

    // Study Session (Bonus)
    CreateStudySession,
    JoinStudySession,
    LeaveStudySession,
    SyncStudyPage,

    // Server
    GetServerHealth
};

// وضعیت پاسخ سرور
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

// تبدیل Enum به QString برای دیباگ و لاگ
QString commandToString(Command cmd);
QString statusToString(Status status);

} // namespace bookclub::common

Q_DECLARE_METATYPE(bookclub::common::Command)
Q_DECLARE_METATYPE(bookclub::common::Status)
