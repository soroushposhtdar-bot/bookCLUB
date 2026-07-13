#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookRepository.h"

namespace bookclub::server {

class AdminRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit AdminRequestHandler(common::IUserRepository* userRepo,
                                 common::IBookRepository* bookRepo,
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

    QJsonObject userToJson(common::UserAccount* user) const;

    common::IUserRepository* m_userRepo;
    common::IBookRepository* m_bookRepo;
};

} // namespace bookclub::server
