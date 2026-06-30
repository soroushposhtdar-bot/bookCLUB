#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IBookRepository.h"

namespace bookclub::server {

class PublisherRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit PublisherRequestHandler(common::IBookService* bookService,
                                     common::IBookRepository* bookRepo,
                                     QObject* parent = nullptr);
    ~PublisherRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetPublisherBooks(const QJsonObject& payload, ClientConnection* client);
    void handlePublishBook(const QJsonObject& payload, ClientConnection* client);
    void handleUpdateBook(const QJsonObject& payload, ClientConnection* client);
    void handleDeactivateBook(const QJsonObject& payload, ClientConnection* client);
    void handleActivateBook(const QJsonObject& payload, ClientConnection* client);
    void handleApplyTimedDiscount(const QJsonObject& payload, ClientConnection* client);
    void handleGetPublisherAnalytics(const QJsonObject& payload, ClientConnection* client);

    common::Book* createBookFromPayload(const QJsonObject& payload);
    QJsonObject bookToJson(common::Book* book) const;

    common::IBookService* m_bookService;
    common::IBookRepository* m_bookRepo;
};

} // namespace bookclub::server
