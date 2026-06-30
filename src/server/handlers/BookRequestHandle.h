#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IBookRepository.h"

namespace bookclub::server {

class BookRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit BookRequestHandler(common::IBookService* bookService,
                                common::IBookRepository* bookRepo,
                                QObject* parent = nullptr);
    ~BookRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetHomeSections(const QJsonObject& payload, ClientConnection* client);
    void handleSearchBooks(const QJsonObject& payload, ClientConnection* client);
    void handleGetBookDetails(const QJsonObject& payload, ClientConnection* client);

    QJsonObject bookToJson(common::Book* book) const;
    QJsonArray bookListToJson(const QVector<common::Book*>& books) const;

    common::IBookService* m_bookService;
    common::IBookRepository* m_bookRepo;
};

} // namespace bookclub::server
