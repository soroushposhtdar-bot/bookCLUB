#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IUserRepository.h"

namespace bookclub::server {

class LibraryRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit LibraryRequestHandler(common::IUserRepository* userRepo,
                                   QObject* parent = nullptr);
    ~LibraryRequestHandler() override;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetLibrary(const QJsonObject& payload, ClientConnection* client);
    void handleGetPurchasedBooks(const QJsonObject& payload, ClientConnection* client);
    void handleCreateShelf(const QJsonObject& payload, ClientConnection* client);
    void handleDeleteShelf(const QJsonObject& payload, ClientConnection* client);
    void handleAddBookToShelf(const QJsonObject& payload, ClientConnection* client);
    void handleRemoveBookFromShelf(const QJsonObject& payload, ClientConnection* client);
    void handleRenameShelf(const QJsonObject& payload, ClientConnection* client);
    // v15e: update shelf metadata (color / favorite / private / sortOrder).
    void handleUpdateShelf(const QJsonObject& payload, ClientConnection* client);
};

} // namespace bookclub::server
