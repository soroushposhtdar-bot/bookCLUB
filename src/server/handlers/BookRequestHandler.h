#pragma once

#include <QVector>

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IReviewRepository.h"
#include "common/Interfaces/IRatingRepository.h"

namespace bookclub::server {

class NotificationDispatcher;

class BookRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit BookRequestHandler(common::IBookService* bookService,
                                common::IBookRepository* bookRepo,
                                common::IReviewRepository* reviewRepo = nullptr,
                                common::IRatingRepository* ratingRepo = nullptr,
                                NotificationDispatcher* dispatcher = nullptr,
                                QObject* parent = nullptr);
    ~BookRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    // Catalog
    void handleGetHomeSections(const QJsonObject& payload, ClientConnection* client);
    void handleSearchBooks(const QJsonObject& payload, ClientConnection* client);
    void handleGetBookDetails(const QJsonObject& payload, ClientConnection* client);
    void handleGetBooksByIds(const QJsonObject& payload, ClientConnection* client);

    // PDF file transfer
    void handleUploadBookPdf(const QJsonObject& payload, ClientConnection* client);
    void handleDownloadBookPdf(const QJsonObject& payload, ClientConnection* client);

    // Reviews + ratings
    void handleSubmitReview(const QJsonObject& payload, ClientConnection* client);
    void handleUpdateReview(const QJsonObject& payload, ClientConnection* client);
    void handleDeleteReview(const QJsonObject& payload, ClientConnection* client);
    void handleSetRating(const QJsonObject& payload, ClientConnection* client);
    // Issue 7: helpful-mark + reply counters.
    void handleMarkReviewHelpful(const QJsonObject& payload, ClientConnection* client);
    void handleAddReviewReply(const QJsonObject& payload, ClientConnection* client);

    QJsonObject bookToJson(common::Book* book) const;
    QJsonArray bookListToJson(const QVector<common::Book*>& books) const;
    QJsonObject reviewToJson(common::Review* review) const;

    common::IBookService* m_bookService;
    common::IBookRepository* m_bookRepo;
    common::IReviewRepository* m_reviewRepo;
    common::IRatingRepository* m_ratingRepo;
    NotificationDispatcher* m_dispatcher;
};

} // namespace bookclub::server
