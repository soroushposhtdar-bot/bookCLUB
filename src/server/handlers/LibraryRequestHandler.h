#pragma once

#include <QMap>

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Models/UserLibrary.h"

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

    common::UserLibrary* getOrCreateLibrary(const QString& userId);
    QJsonObject libraryToJson(common::UserLibrary* library) const;

    common::IUserRepository* m_userRepo;
    QMap<QString, common::UserLibrary*> m_userLibraries;
};

} // namespace bookclub::server
