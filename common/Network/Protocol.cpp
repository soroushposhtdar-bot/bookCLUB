// common/Network/Protocol.cpp
#include "common/Network/Protocol.h"

namespace bookclub::common {

QString commandToString(Command cmd) {
    switch (cmd) {
        // Auth
        case Command::Login:            return "Login";
        case Command::Register:         return "Register";
        case Command::ResetPassword:    return "ResetPassword";
        case Command::ChangePassword:   return "ChangePassword";
        case Command::Logout:           return "Logout";

        // Books
        case Command::GetHomeSections:  return "GetHomeSections";
        case Command::SearchBooks:      return "SearchBooks";
        case Command::GetBookDetails:   return "GetBookDetails";
        case Command::GetBooksByIds:    return "GetBooksByIds";
        case Command::UploadBookPdf:    return "UploadBookPdf";
        case Command::DownloadBookPdf:  return "DownloadBookPdf";
        case Command::UpdateShelf:      return "UpdateShelf";

        // Cart
        case Command::AddToCart:        return "AddToCart";
        case Command::RemoveFromCart:   return "RemoveFromCart";
        case Command::GetCart:          return "GetCart";
        case Command::Checkout:         return "Checkout";
        case Command::ApplyDiscount:    return "ApplyDiscount";
        case Command::ClearCart:        return "ClearCart";

        // Library
        case Command::GetLibrary:           return "GetLibrary";
        case Command::GetPurchasedBooks:    return "GetPurchasedBooks";
        case Command::CreateShelf:          return "CreateShelf";
        case Command::DeleteShelf:          return "DeleteShelf";
        case Command::AddBookToShelf:       return "AddBookToShelf";
        case Command::RemoveBookFromShelf:  return "RemoveBookFromShelf";
        case Command::RenameShelf:          return "RenameShelf";

        // Reviews + ratings
        case Command::SubmitReview:       return "SubmitReview";
        case Command::UpdateReview:       return "UpdateReview";
        case Command::DeleteReview:       return "DeleteReview";
        case Command::SetRating:          return "SetRating";
        // Issue 7: helpful-mark + reply counters.
        case Command::MarkReviewHelpful:  return "MarkReviewHelpful";
        case Command::AddReviewReply:     return "AddReviewReply";

        // Wishlist
        case Command::ToggleWishlist:   return "ToggleWishlist";
        case Command::GetWishlist:      return "GetWishlist";

        // Publisher
        case Command::GetPublisherBooks:     return "GetPublisherBooks";
        case Command::PublishBook:           return "PublishBook";
        case Command::UpdateBook:            return "UpdateBook";
        case Command::DeactivateBook:        return "DeactivateBook";
        case Command::ActivateBook:          return "ActivateBook";
        case Command::ApplyTimedDiscount:    return "ApplyTimedDiscount";
        case Command::GetPublisherAnalytics: return "GetPublisherAnalytics";

        // Admin
        case Command::GetUsersList:       return "GetUsersList";
        case Command::BlockUser:          return "BlockUser";
        case Command::UnblockUser:        return "UnblockUser";
        case Command::DeleteUser:         return "DeleteUser";
        case Command::ModerateBook:       return "ModerateBook";
        case Command::RemoveBookByAdmin:  return "RemoveBookByAdmin";
        case Command::AdminDeleteReview:  return "AdminDeleteReview";
        case Command::AdminApproveReview: return "AdminApproveReview";

        // Notifications
        case Command::GetNotifications:         return "GetNotifications";
        case Command::MarkNotificationRead:     return "MarkNotificationRead";
        case Command::MarkAllNotificationsRead: return "MarkAllNotificationsRead";

        // Study sessions
        case Command::CreateStudySession:     return "CreateStudySession";
        case Command::JoinStudySession:       return "JoinStudySession";
        case Command::LeaveStudySession:      return "LeaveStudySession";
        case Command::SyncStudyPage:          return "SyncStudyPage";
        case Command::GetStudySessions:       return "GetStudySessions";
        case Command::SendStudyMessage:       return "SendStudyMessage";
        case Command::InviteToStudySession:   return "InviteToStudySession";
        case Command::AddStudyNote:           return "AddStudyNote";
        case Command::GetStudyNotes:          return "GetStudyNotes";

        // Server / operator
        case Command::GetServerHealth:    return "GetServerHealth";
        case Command::GetServerLogs:      return "GetServerLogs";
        case Command::GetServerClients:   return "GetServerClients";

        // User profile
        case Command::GetCurrentUser:     return "GetCurrentUser";
        case Command::UpdateProfile:      return "UpdateProfile";
        case Command::SaveFavoriteGenres: return "SaveFavoriteGenres";

        // Genres / Authors
        case Command::GetGenres:          return "GetGenres";
        case Command::GetAuthors:         return "GetAuthors";

        // Server-pushed events
        case Command::EvtNotification:            return "EvtNotification";
        case Command::EvtReviewUpdated:           return "EvtReviewUpdated";
        case Command::EvtStudySync:               return "EvtStudySync";
        case Command::EvtBookAdded:               return "EvtBookAdded";
        case Command::EvtDiscountApplied:         return "EvtDiscountApplied";
        case Command::EvtUserBlocked:             return "EvtUserBlocked";
        case Command::EvtServerShutdown:          return "EvtServerShutdown";
        case Command::EvtSessionListUpdated:      return "EvtSessionListUpdated";
        case Command::EvtStudyMessage:            return "EvtStudyMessage";
        case Command::EvtStudyNote:               return "EvtStudyNote";
        case Command::EvtStudyParticipantChanged: return "EvtStudyParticipantChanged";

        case Command::Invalid: return "Invalid";
        default:               return "Unknown";
    }
}

QString statusToString(Status status) {
    switch (status) {
        case Status::Success:            return "Success";
        case Status::BadRequest:         return "BadRequest";
        case Status::Unauthorized:       return "Unauthorized";
        case Status::Forbidden:          return "Forbidden";
        case Status::NotFound:           return "NotFound";
        case Status::Conflict:           return "Conflict";
        case Status::ValidationError:    return "ValidationError";
        case Status::InternalError:      return "InternalError";
        case Status::ServiceUnavailable: return "ServiceUnavailable";
        default:                         return "Unknown";
    }
}

} // namespace bookclub::common
