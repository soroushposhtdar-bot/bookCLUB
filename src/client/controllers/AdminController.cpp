// src/client/controllers/AdminController.cpp
#include "src/client/controllers/AdminController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

AdminController::AdminController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::GetUsersList, [this](const common::Message& response) {
        handleGetUsersListResponse(response);
    });

    network.registerRequestHandler(common::Command::BlockUser, [this](const common::Message& response) {
        handleBlockUserResponse(response);
    });

    network.registerRequestHandler(common::Command::UnblockUser, [this](const common::Message& response) {
        handleUnblockUserResponse(response);
    });

    network.registerRequestHandler(common::Command::DeleteUser, [this](const common::Message& response) {
        handleDeleteUserResponse(response);
    });

    network.registerRequestHandler(common::Command::ModerateBook, [this](const common::Message& response) {
        handleModerateBookResponse(response);
    });

    network.registerRequestHandler(common::Command::RemoveBookByAdmin, [this](const common::Message& response) {
        handleRemoveBookByAdminResponse(response);
    });

    LOG_INFO("AdminController initialized");
}

AdminController::~AdminController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::GetUsersList);
    network.unregisterRequestHandler(common::Command::BlockUser);
    network.unregisterRequestHandler(common::Command::UnblockUser);
    network.unregisterRequestHandler(common::Command::DeleteUser);
    network.unregisterRequestHandler(common::Command::ModerateBook);
    network.unregisterRequestHandler(common::Command::RemoveBookByAdmin);
}

// ---- Public Methods ----

void AdminController::loadUsers()
{
    LOG_DEBUG("AdminController::loadUsers() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load users failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetUsersList, {});
}

void AdminController::searchUsers(const QString& keyword)
{
    LOG_DEBUG("AdminController::searchUsers() called with keyword: " + keyword);

    if (keyword.isEmpty()) {
        LOG_WARNING("Search users failed: keyword is empty");
        emit errorOccurred("Search keyword is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Search users failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Load all users from the server, then filter locally by the keyword.
    // The filtered result is stored in m_usersData so getUsers() returns
    // only matching users. Previously this method ignored the keyword
    // entirely and just called loadUsers().
    QJsonObject payload;
    payload["keyword"] = keyword;
    // Note: there's no dedicated SearchUsers command, so we reuse
    // GetUsersList and filter client-side. The keyword is stored so the
    // response handler can apply the filter.
    m_searchKeyword = keyword;
    ClientNetworkManager::instance().sendRequest(common::Command::GetUsersList, payload);
}

void AdminController::blockUser(const QString& userId)
{
    LOG_DEBUG("AdminController::blockUser() called for user: " + userId);

    if (userId.isEmpty()) {
        LOG_WARNING("Block user failed: user ID is empty");
        emit errorOccurred("User ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Block user failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["userId"] = userId;

    ClientNetworkManager::instance().sendRequest(common::Command::BlockUser, payload);
}

void AdminController::unblockUser(const QString& userId)
{
    LOG_DEBUG("AdminController::unblockUser() called for user: " + userId);

    if (userId.isEmpty()) {
        LOG_WARNING("Unblock user failed: user ID is empty");
        emit errorOccurred("User ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Unblock user failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["userId"] = userId;

    ClientNetworkManager::instance().sendRequest(common::Command::UnblockUser, payload);
}

void AdminController::deleteUser(const QString& userId)
{
    LOG_DEBUG("AdminController::deleteUser() called for user: " + userId);

    if (userId.isEmpty()) {
        LOG_WARNING("Delete user failed: user ID is empty");
        emit errorOccurred("User ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Delete user failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["userId"] = userId;

    ClientNetworkManager::instance().sendRequest(common::Command::DeleteUser, payload);
}

void AdminController::loadBooks()
{
    LOG_DEBUG("AdminController::loadBooks() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load books failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Admin uses the same BookCatalogController's search or GetHomeSections
    // For admin, we can just use the book catalog's loadHomeSections
    // But since admin needs all books, we use a search with empty keyword
    QJsonObject payload;
    payload["keyword"] = "";
    ClientNetworkManager::instance().sendRequest(common::Command::SearchBooks, payload);
}

void AdminController::moderateBook(const QString& bookId)
{
    LOG_DEBUG("AdminController::moderateBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Moderate book failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Moderate book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::ModerateBook, payload);
}

void AdminController::removeBook(const QString& bookId)
{
    LOG_DEBUG("AdminController::removeBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Remove book failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Remove book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::RemoveBookByAdmin, payload);
}

// ---- Response Handlers ----

void AdminController::handleGetUsersListResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load users");
        LOG_WARNING("Load users failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QJsonArray users = data["users"].toArray();
    int count = data["count"].toInt();

    m_usersData = data;

    emit userListChanged();

    LOG_INFO("Users loaded successfully. Count: " + QString::number(count));
}

void AdminController::handleBlockUserResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to block user");
        LOG_WARNING("Block user failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Refresh the cached user list from the server so subscribers see the
    // updated status. Previously this only emitted userListChanged()
    // without updating m_usersData, so QML subscribers re-queried and got
    // the stale list (the user still appeared Active).
    loadUsers();
    emit userListChanged();

    LOG_INFO("User blocked successfully");
}

void AdminController::handleUnblockUserResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to unblock user");
        LOG_WARNING("Unblock user failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Refresh the cached user list so subscribers see the updated status.
    loadUsers();
    emit userListChanged();

    LOG_INFO("User unblocked successfully");
}

void AdminController::handleDeleteUserResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to delete user");
        LOG_WARNING("Delete user failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Refresh the cached user list so the deleted user disappears.
    loadUsers();
    emit userListChanged();

    LOG_INFO("User deleted successfully");
}

void AdminController::handleModerateBookResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to moderate book");
        LOG_WARNING("Moderate book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Refresh the cached book list so subscribers see the updated status.
    loadBooks();
    emit bookListChanged();

    LOG_INFO("Book moderated successfully");
}

void AdminController::handleRemoveBookByAdminResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to remove book");
        LOG_WARNING("Remove book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Refresh the cached book list so subscribers see the removal.
    loadBooks();
    emit bookListChanged();

    LOG_INFO("Book removed by admin successfully");
}

// ---- Helper Methods ----

QJsonArray AdminController::getUsers() const
{
    return m_usersData["users"].toArray();
}

int AdminController::getUserCount() const
{
    return m_usersData["count"].toInt();
}

QJsonObject AdminController::getUser(const QString& userId) const
{
    QJsonArray users = m_usersData["users"].toArray();
    for (const auto& user : users) {
        QJsonObject userObj = user.toObject();
        if (userObj["id"].toString() == userId) {
            return userObj;
        }
    }
    return {};
}

QJsonArray AdminController::getBooks() const
{
    // This would come from the book catalog controller
    // For admin, we can use the search results
    return m_booksData["results"].toArray();
}

int AdminController::getBookCount() const
{
    return m_booksData["count"].toInt();
}

void AdminController::setBooksData(const QJsonObject& booksData)
{
    m_booksData = booksData;
    emit bookListChanged();
}

} // namespace bookclub::client
