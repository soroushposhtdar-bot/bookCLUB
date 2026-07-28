#include "src/server/handlers/AdminRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/ConnectionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"
#include "common/Models/Book.h"
#include "common/Models/UserAccount.h"
#include "common/Models/Review.h"

#include <QJsonArray>
#include <QDateTime>

namespace bookclub::server {

AdminRequestHandler::AdminRequestHandler(common::IUserRepository* userRepo,
                                         common::IBookRepository* bookRepo,
                                         common::IReviewRepository* reviewRepo,
                                         ConnectionManager* connectionManager,
                                         QObject* parent)
    : RequestHandlerBase(parent)
    , m_userRepo(userRepo)
    , m_bookRepo(bookRepo)
    , m_reviewRepo(reviewRepo)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("AdminRequestHandler initialized");
}

void AdminRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    // Every Admin command requires the Admin role.
    if (!requireRole(client, common::AccountRole::Admin, request.command())) {
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::GetUsersList:
            handleGetUsersList(payload, client);
            break;
        case common::Command::BlockUser:
            handleBlockUser(payload, client);
            break;
        case common::Command::UnblockUser:
            handleUnblockUser(payload, client);
            break;
        case common::Command::DeleteUser:
            handleDeleteUser(payload, client);
            break;
        case common::Command::ModerateBook:
            handleModerateBook(payload, client);
            break;
        case common::Command::RemoveBookByAdmin:
            handleRemoveBookByAdmin(payload, client);
            break;
        case common::Command::AdminDeleteReview:
            handleAdminDeleteReview(payload, client);
            break;
        case common::Command::AdminApproveReview:
            handleAdminApproveReview(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void AdminRequestHandler::handleGetUsersList(const QJsonObject& payload, ClientConnection* client)
{
    Q_UNUSED(payload);
    QVector<common::UserAccount*> users = m_userRepo->findAll();

    QJsonArray usersArray;
    for (common::UserAccount* user : users) {
        // v21b: skip admin accounts — admins should not appear in the
        // user management list.
        if (user->role() == common::AccountRole::Admin) {
            delete user;
            continue;
        }
        usersArray.append(userToJson(user));
        delete user;
    }

    QJsonObject responsePayload;
    responsePayload["users"] = usersArray;
    responsePayload["count"] = usersArray.size();

    sendSuccess(client, common::Command::GetUsersList, responsePayload);
}

void AdminRequestHandler::handleBlockUser(const QJsonObject& payload, ClientConnection* client)
{
    QString userId = payload["userId"].toString();
    if (userId.isEmpty()) {
        sendError(client, common::Command::BlockUser, common::Status::BadRequest, "userId is required");
        return;
    }

    // v21: prevent admin from blocking other admins.
    auto* targetUser = m_userRepo->findById(userId);
    if (targetUser) {
        bool isTargetAdmin = (targetUser->role() == common::AccountRole::Admin);
        delete targetUser;
        if (isTargetAdmin) {
            sendError(client, common::Command::BlockUser, common::Status::Forbidden,
                      "Cannot block an admin account.");
            return;
        }
    }

    if (!m_userRepo->blockUser(userId)) {
        sendError(client, common::Command::BlockUser, common::Status::NotFound, "User not found");
        return;
    }

    // v21: push EvtUserBlocked to the blocked user so their client
    // immediately logs out and shows the "blocked" message.
    if (m_connectionManager) {
        QJsonObject eventPayload;
        eventPayload["userId"] = userId;
        eventPayload["reason"] = "Your account has been blocked by an administrator.";
        common::Message event(common::Command::EvtUserBlocked,
                              common::Status::Success, eventPayload);
        m_connectionManager->sendToUser(userId, event);
    }

    sendSuccess(client, common::Command::BlockUser, {});
    LOG_INFO("User blocked: " + userId + " by admin: " + client->userId());
}

void AdminRequestHandler::handleUnblockUser(const QJsonObject& payload, ClientConnection* client)
{
    QString userId = payload["userId"].toString();
    if (userId.isEmpty()) {
        sendError(client, common::Command::UnblockUser, common::Status::BadRequest, "userId is required");
        return;
    }

    if (!m_userRepo->unblockUser(userId)) {
        sendError(client, common::Command::UnblockUser, common::Status::NotFound, "User not found");
        return;
    }

    sendSuccess(client, common::Command::UnblockUser, {});
    LOG_INFO("User unblocked: " + userId + " by admin: " + client->userId());
}

void AdminRequestHandler::handleDeleteUser(const QJsonObject& payload, ClientConnection* client)
{
    QString userId = payload["userId"].toString();
    if (userId.isEmpty()) {
        sendError(client, common::Command::DeleteUser, common::Status::BadRequest, "userId is required");
        return;
    }

    // v21: prevent admin from deleting other admins.
    auto* targetUser = m_userRepo->findById(userId);
    if (targetUser) {
        bool isTargetAdmin = (targetUser->role() == common::AccountRole::Admin);
        delete targetUser;
        if (isTargetAdmin) {
            sendError(client, common::Command::DeleteUser, common::Status::Forbidden,
                      "Cannot delete an admin account.");
            return;
        }
    }

    // v21: also prevent admin from blocking other admins.
    // (The block/unblock handlers don't have this check, so we add it
    // here as a shared guard. We also check in handleBlockUser below.)

    if (!m_userRepo->remove(userId)) {
        sendError(client, common::Command::DeleteUser, common::Status::NotFound, "User not found");
        return;
    }

    sendSuccess(client, common::Command::DeleteUser, {});
    LOG_INFO("User deleted: " + userId + " by admin: " + client->userId());
}

void AdminRequestHandler::handleModerateBook(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::ModerateBook, common::Status::BadRequest, "bookId is required");
        return;
    }

    // Check if book exists
    common::Book* book = m_bookRepo->findById(bookId);
    if (!book) {
        sendError(client, common::Command::ModerateBook, common::Status::NotFound, "Book not found");
        return;
    }

    // v21b: check for an explicit "action" field. If present, set the
    // desired state directly instead of toggling.
    const QString action = payload["action"].toString();
    bool isActive = book->isActive();
    delete book;

    if (action == "active") {
        if (!isActive) m_bookRepo->activate(bookId);
        LOG_INFO("Book activated by admin: " + bookId);
    } else if (action == "inactive" || action == "removed" || action == "deactivated") {
        if (isActive) m_bookRepo->deactivate(bookId);
        LOG_INFO("Book deactivated by admin: " + bookId);
    } else {
        // No action specified — toggle (legacy behavior)
        if (isActive) {
            m_bookRepo->deactivate(bookId);
            LOG_INFO("Book deactivated by admin: " + bookId);
        } else {
            m_bookRepo->activate(bookId);
            LOG_INFO("Book activated by admin: " + bookId);
        }
    }

    sendSuccess(client, common::Command::ModerateBook, {});
}

void AdminRequestHandler::handleRemoveBookByAdmin(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::RemoveBookByAdmin, common::Status::BadRequest, "bookId is required");
        return;
    }

    if (!m_bookRepo->remove(bookId)) {
        sendError(client, common::Command::RemoveBookByAdmin, common::Status::NotFound, "Book not found");
        return;
    }

    sendSuccess(client, common::Command::RemoveBookByAdmin, {});
    LOG_INFO("Book removed by admin: " + bookId);
}

// ============================================================================
//  Admin review moderation
// ============================================================================

void AdminRequestHandler::handleAdminDeleteReview(const QJsonObject& payload, ClientConnection* client)
{
    const QString reviewId = payload["reviewId"].toString();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::AdminDeleteReview, common::Status::BadRequest,
                  "reviewId is required");
        return;
    }

    if (!m_reviewRepo) {
        sendError(client, common::Command::AdminDeleteReview, common::Status::InternalError,
                  "Review repository not available");
        return;
    }

    // Verify the review exists.
    std::unique_ptr<common::Review> review(m_reviewRepo->findById(reviewId));
    if (!review) {
        sendError(client, common::Command::AdminDeleteReview, common::Status::NotFound,
                  "Review not found");
        return;
    }

    // Admin can delete any review — no ownership check needed.
    if (!m_reviewRepo->remove(reviewId)) {
        sendError(client, common::Command::AdminDeleteReview, common::Status::InternalError,
                  "Failed to delete review");
        return;
    }

    sendSuccess(client, common::Command::AdminDeleteReview, {});
    LOG_INFO("Review deleted by admin: " + reviewId + " by admin: " + client->userId());
}

void AdminRequestHandler::handleAdminApproveReview(const QJsonObject& payload, ClientConnection* client)
{
    const QString reviewId = payload["reviewId"].toString();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::AdminApproveReview, common::Status::BadRequest,
                  "reviewId is required");
        return;
    }

    if (!m_reviewRepo) {
        sendError(client, common::Command::AdminApproveReview, common::Status::InternalError,
                  "Review repository not available");
        return;
    }

    // Verify the review exists.
    std::unique_ptr<common::Review> review(m_reviewRepo->findById(reviewId));
    if (!review) {
        sendError(client, common::Command::AdminApproveReview, common::Status::NotFound,
                  "Review not found");
        return;
    }

    // "Approve" = unflag + pin the review so it appears at the top.
    common::DbConnection::execOk(
        "UPDATE Reviews SET isFlagged = 0, isPinned = 1, updatedAt = ? WHERE id = ?",
        {QDateTime::currentDateTime(), reviewId}
    );

    QJsonObject responsePayload;
    responsePayload["reviewId"] = reviewId;
    responsePayload["isPinned"]  = true;
    responsePayload["isFlagged"] = false;

    sendSuccess(client, common::Command::AdminApproveReview, responsePayload);
    LOG_INFO("Review approved by admin: " + reviewId + " by admin: " + client->userId());
}

// ============================================================================
//  Helpers
// ============================================================================

QJsonObject AdminRequestHandler::userToJson(common::UserAccount* user) const
{
    if (!user) return {};

    QJsonObject obj;
    obj["id"] = user->id();
    obj["username"] = user->username();
    obj["displayName"] = user->displayName();
    obj["email"] = user->email();
    obj["phone"] = user->phone();
    obj["status"] = static_cast<int>(user->status());
    obj["role"] = static_cast<int>(user->role());
    obj["roleName"] = user->roleName();
    obj["createdAt"] = user->createdAt().toString(Qt::ISODate);
    return obj;
}

} // namespace bookclub::server
