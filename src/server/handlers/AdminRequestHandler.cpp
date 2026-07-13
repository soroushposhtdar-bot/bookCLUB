#include "src/server/handlers/AdminRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Models/Book.h"
#include "common/Models/UserAccount.h"

#include <QJsonArray>

namespace bookclub::server {

AdminRequestHandler::AdminRequestHandler(common::IUserRepository* userRepo,
                                         common::IBookRepository* bookRepo,
                                         QObject* parent)
    : RequestHandlerBase(parent)
    , m_userRepo(userRepo)
    , m_bookRepo(bookRepo)
{
    LOG_INFO("AdminRequestHandler initialized");
}

void AdminRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    // Guard against null client FIRST — previously the code passed `client`
    // into sendError() even when it was null, which would dereference null
    // inside the socket write.
    if (!client) {
        LOG_ERROR("AdminRequestHandler: null client pointer — dropping request");
        return;
    }

    if (!client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    // TODO: Check if user is admin (role check). The client->userId() should
    // be resolved to a UserAccount and its role() verified to be AdminRole.
    // Until this is implemented, any authenticated user can call admin
    // commands — a privilege-escalation risk. The mock client (AdminService)
    // doesn't go through this handler, so this only affects the real
    // socket-backed path.

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
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void AdminRequestHandler::handleGetUsersList(const QJsonObject& payload, ClientConnection* client)
{
    QVector<common::UserAccount*> users = m_userRepo->findAll();

    QJsonArray usersArray;
    for (common::UserAccount* user : users) {
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

    if (!m_userRepo->blockUser(userId)) {
        sendError(client, common::Command::BlockUser, common::Status::NotFound, "User not found");
        return;
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

    // Toggle visibility or status
    bool isActive = book->isActive();
    delete book;

    if (isActive) {
        m_bookRepo->deactivate(bookId);
        LOG_INFO("Book deactivated by admin: " + bookId);
    } else {
        m_bookRepo->activate(bookId);
        LOG_INFO("Book activated by admin: " + bookId);
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
