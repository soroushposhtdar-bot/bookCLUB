#pragma once

#include <QString>
#include <QFlags>

namespace bookclub::common {

enum class AccountRole {
    User,
    Publisher,
    Admin
};

enum class AccountStatus {
    Pending,
    Active,
    Blocked,
    Disabled,
    Deleted
};

enum class BookVisibility {
    Visible,
    Hidden,
    Archived
};

enum class BookAvailability {
    Available,
    Unavailable,
    Removed
};

enum class NotificationType {
    Generic,
    NewBookInFavoriteGenre,
    DiscountOnSavedBook,
    SaleRegistered,
    NewReview,
    NewRating,
    SystemAlert
};

enum class NotificationState {
    Unread,
    Read,
    Archived
};

enum class SearchField {
    Title,
    Author,
    Publisher,
    Genre,
    All
};

enum class DiscountType {
    Percentage,
    FixedAmount
};

enum class StudySessionState {
    Created,
    Active,
    Paused,
    Closed
};

enum class SyncEventType {
    OpenBook,
    TurnPage,
    JumpToPage,
    ZoomChanged,
    LastPageSaved
};

enum class SortOrder {
    Ascending,
    Descending
};

} // namespace bookclub::common
