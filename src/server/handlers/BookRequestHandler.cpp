#include "src/server/handlers/BookRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"

namespace bookclub::server {

BookRequestHandler::BookRequestHandler(common::IBookService* bookService,
                                       common::IBookRepository* bookRepo,
                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_bookRepo(bookRepo)
{
    LOG_INFO("BookRequestHandler initialized");
}

void BookRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client) {
        LOG_ERROR("BookRequestHandler: null client");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::GetHomeSections:
            handleGetHomeSections(payload, client);
            break;
        case common::Command::SearchBooks:
            handleSearchBooks(payload, client);
            break;
        case common::Command::GetBookDetails:
            handleGetBookDetails(payload, client);
            break;
        default:
            LOG_WARNING("BookRequestHandler: Unhandled command " + common::commandToString(cmd));
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void BookRequestHandler::handleGetHomeSections(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("GetHomeSections request from client: " + client->clientId());

    QJsonObject responsePayload;
    responsePayload["featured"] = bookListToJson(m_bookService->listFeaturedBooks());
    responsePayload["newBooks"] = bookListToJson(m_bookService->listNewBooks());
    responsePayload["bestSellers"] = bookListToJson(m_bookService->listBestSellers());
    responsePayload["freeBooks"] = bookListToJson(m_bookService->listFreeBooks());

    sendSuccess(client, common::Command::GetHomeSections, responsePayload);
}

void BookRequestHandler::handleSearchBooks(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("SearchBooks request from client: " + client->clientId());

    QString keyword = payload["keyword"].toString();
    if (keyword.isEmpty()) {
        sendError(client, common::Command::SearchBooks, common::Status::BadRequest,
                  "Keyword is required");
        return;
    }

    QVector<common::Book*> results = m_bookService->searchBooks(keyword);
    QJsonObject responsePayload;
    responsePayload["results"] = bookListToJson(results);
    responsePayload["count"] = results.size();

    qDeleteAll(results);
    sendSuccess(client, common::Command::SearchBooks, responsePayload);
}

void BookRequestHandler::handleGetBookDetails(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("GetBookDetails request from client: " + client->clientId());

    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::GetBookDetails, common::Status::BadRequest,
                  "bookId is required");
        return;
    }

    common::Book* book = m_bookRepo->findById(bookId);
    if (!book) {
        sendError(client, common::Command::GetBookDetails, common::Status::NotFound,
                  "Book not found");
        return;
    }

    QJsonObject responsePayload = bookToJson(book);
    // Add reviews
    QVector<common::Review*> reviews = m_bookRepo->reviewsOf(bookId);
    QJsonArray reviewsArray;
    for (common::Review* review : reviews) {
        QJsonObject reviewObj;
        reviewObj["id"] = review->id();
        reviewObj["userId"] = review->userId();
        reviewObj["userDisplayName"] = review->userDisplayName();
        reviewObj["text"] = review->text();
        reviewObj["stars"] = review->stars();
        reviewObj["createdAt"] = review->createdAt().toString(Qt::ISODate);
        reviewsArray.append(reviewObj);
        delete review;
    }
    responsePayload["reviews"] = reviewsArray;

    delete book;
    sendSuccess(client, common::Command::GetBookDetails, responsePayload);
}

QJsonObject BookRequestHandler::bookToJson(common::Book* book) const
{
    if (!book) return {};

    QJsonObject obj;
    obj["id"] = book->id();
    obj["title"] = book->title();
    obj["authorName"] = book->authorName();
    obj["publisherId"] = book->publisherId();
    obj["genreIds"] = QJsonArray::fromStringList(book->genreIds());
    obj["description"] = book->description();
    obj["coverImagePath"] = book->coverImagePath();
    obj["pdfFilePath"] = book->pdfFilePath();
    obj["price"] = book->price();
    obj["basePrice"] = book->basePrice();
    obj["discountValue"] = book->discountValue();
    obj["averageRating"] = book->averageRating();
    obj["ratingCount"] = book->ratingCount();
    obj["totalSales"] = book->totalSales();
    obj["isFree"] = book->isFree();
    obj["isActive"] = book->isActive();
    obj["visibility"] = static_cast<int>(book->visibility());
    obj["availability"] = static_cast<int>(book->availability());
    return obj;
}

QJsonArray BookRequestHandler::bookListToJson(const QVector<common::Book*>& books) const
{
    QJsonArray array;
    for (common::Book* book : books) {
        array.append(bookToJson(book));
    }
    return array;
}

} // namespace bookclub::server
