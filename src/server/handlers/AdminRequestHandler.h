#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IReviewRepository.h"

namespace bookclub::server {

class NotificationDispatcher;
class ConnectionManager;

class AdminRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit AdminRequestHandler(common::IUserRepository* userRepo,
                                 common::IBookRepository* bookRepo,
                                 common::IReviewRepository* reviewRepo,
                                 ConnectionManager* connectionManager = nullptr,
                                 QObject* parent = nullptr);
    ~AdminRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetUsersList(const QJsonObject& payload, ClientConnection* client);
    void handleBlockUser(const QJsonObject& payload, ClientConnection* client);
    void handleUnblockUser(const QJsonObject& payload, ClientConnection* client);
    void handleDeleteUser(const QJsonObject& payload, ClientConnection* client);
    void handleModerateBook(const QJsonObject& payload, ClientConnection* client);
    void handleRemoveBookByAdmin(const QJsonObject& payload, ClientConnection* client);
    void handleAdminDeleteReview(const QJsonObject& payload, ClientConnection* client);
    void handleAdminApproveReview(const QJsonObject& payload, ClientConnection* client);

    QJsonObject userToJson(common::UserAccount* user) const;

    common::IUserRepository* m_userRepo;
    common::IBookRepository* m_bookRepo;
    common::IReviewRepository* m_reviewRepo;
    ConnectionManager* m_connectionManager;  // v21: for pushing EvtUserBlocked
};

} // namespace bookclub::server