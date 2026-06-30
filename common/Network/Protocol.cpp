// common/Network/Protocol.cpp
#include "common/Network/Protocol.h"

namespace bookclub::common {

QString commandToString(Command cmd) {
    switch (cmd) {
        case Command::Login: return "Login";
        case Command::Register: return "Register";
        case Command::ResetPassword: return "ResetPassword";
        case Command::Logout: return "Logout";
        case Command::GetHomeSections: return "GetHomeSections";
        case Command::SearchBooks: return "SearchBooks";
        case Command::GetBookDetails: return "GetBookDetails";
        case Command::AddToCart: return "AddToCart";
        case Command::RemoveFromCart: return "RemoveFromCart";
        case Command::GetCart: return "GetCart";
        case Command::Checkout: return "Checkout";
        case Command::ApplyDiscount: return "ApplyDiscount";
        case Command::GetLibrary: return "GetLibrary";
        case Command::GetPurchasedBooks: return "GetPurchasedBooks";
        case Command::CreateShelf: return "CreateShelf";
        case Command::DeleteShelf: return "DeleteShelf";
        case Command::AddBookToShelf: return "AddBookToShelf";
        case Command::RemoveBookFromShelf: return "RemoveBookFromShelf";
        case Command::GetPublisherBooks: return "GetPublisherBooks";
        case Command::PublishBook: return "PublishBook";
        case Command::UpdateBook: return "UpdateBook";
        case Command::DeactivateBook: return "DeactivateBook";
        case Command::ActivateBook: return "ActivateBook";
        case Command::ApplyTimedDiscount: return "ApplyTimedDiscount";
        case Command::GetPublisherAnalytics: return "GetPublisherAnalytics";
        case Command::GetUsersList: return "GetUsersList";
        case Command::BlockUser: return "BlockUser";
        case Command::UnblockUser: return "UnblockUser";
        case Command::DeleteUser: return "DeleteUser";
        case Command::ModerateBook: return "ModerateBook";
        case Command::RemoveBookByAdmin: return "RemoveBookByAdmin";
        case Command::GetNotifications: return "GetNotifications";
        case Command::MarkNotificationRead: return "MarkNotificationRead";
        case Command::MarkAllNotificationsRead: return "MarkAllNotificationsRead";
        case Command::CreateStudySession: return "CreateStudySession";
        case Command::JoinStudySession: return "JoinStudySession";
        case Command::LeaveStudySession: return "LeaveStudySession";
        case Command::SyncStudyPage: return "SyncStudyPage";
        case Command::GetServerHealth: return "GetServerHealth";
        default: return "Unknown";
    }
}

QString statusToString(Status status) {
    switch (status) {
        case Status::Success: return "Success";
        case Status::BadRequest: return "BadRequest";
        case Status::Unauthorized: return "Unauthorized";
        case Status::Forbidden: return "Forbidden";
        case Status::NotFound: return "NotFound";
        case Status::Conflict: return "Conflict";
        case Status::ValidationError: return "ValidationError";
        case Status::InternalError: return "InternalError";
        case Status::ServiceUnavailable: return "ServiceUnavailable";
        default: return "Unknown";
    }
}

} // namespace bookclub::common
