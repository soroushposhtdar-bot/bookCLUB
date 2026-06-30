#include "src/server/handlers/PublisherRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Models/Discount.h"

namespace bookclub::server {

PublisherRequestHandler::PublisherRequestHandler(common::IBookService* bookService,
                                                 common::IBookRepository* bookRepo,
                                                 QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_bookRepo(bookRepo)
{
    LOG_INFO("PublisherRequestHandler initialized");
}

void PublisherRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::GetPublisherBooks:
            handleGetPublisherBooks(payload, client);
            break;
        case common::Command::PublishBook:
            handlePublishBook(payload, client);
            break;
        case common::Command::UpdateBook:
            handleUpdateBook(payload, client);
            break;
        case common::Command::DeactivateBook:
            handleDeactivateBook(payload, client);
            break;
        case common::Command::ActivateBook:
            handleActivateBook(payload, client);
            break;
        case common::Command::ApplyTimedDiscount:
            handleApplyTimedDiscount(payload, client);
            break;
        case common::Command::GetPublisherAnalytics:
            handleGetPublisherAnalytics(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void PublisherRequestHandler::handleGetPublisherBooks(const QJsonObject& payload, ClientConnection* client)
{
    QString publisherId = client->userId();
    QVector<common::Book*> books = m_bookRepo->findByPublisher(publisherId);

    QJsonArray booksArray;
    for (common::Book* book : books) {
        booksArray.append(bookToJson(book));
        delete book;
    }

    QJsonObject responsePayload;
    responsePayload["books"] = booksArray;
    responsePayload["count"] = booksArray.size();

    sendSuccess(client, common::Command::GetPublisherBooks, responsePayload);
}

void PublisherRequestHandler::handlePublishBook(const QJsonObject& payload, ClientConnection* client)
{
    common::Book* book = createBookFromPayload(payload);
    if (!book) {
        sendError(client, common::Command::PublishBook, common::Status::BadRequest, "Invalid book data");
        return;
    }

    book->setPublisherId(client->userId());
    book->setId(common::IdGenerator::generateUuid());

    if (!m_bookService->createBook(book)) {
        delete book;
        sendError(client, common::Command::PublishBook, common::Status::InternalError, "Failed to publish book");
        return;
    }

    sendSuccess(client, common::Command::PublishBook, bookToJson(book));
    delete book;
    LOG_INFO("Book published: " + book->title() + " by publisher: " + client->userId());
}

void PublisherRequestHandler::handleUpdateBook(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["id"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::UpdateBook, common::Status::BadRequest, "bookId is required");
        return;
    }

    common::Book* existingBook = m_bookRepo->findById(bookId);
    if (!existingBook) {
        sendError(client, common::Command::UpdateBook, common::Status::NotFound, "Book not found");
        return;
    }

    if (existingBook->publisherId() != client->userId()) {
        delete existingBook;
        sendError(client, common::Command::UpdateBook, common::Status::Forbidden, "You don't own this book");
        return;
    }

    common::Book* updatedBook = createBookFromPayload(payload);
    if (!updatedBook) {
        delete existingBook;
        sendError(client, common::Command::UpdateBook, common::Status::BadRequest, "Invalid book data");
        return;
    }

    updatedBook->setId(bookId);
    updatedBook->setPublisherId(client->userId());

    if (!m_bookService->updateBook(updatedBook)) {
        delete existingBook;
        delete updatedBook;
        sendError(client, common::Command::UpdateBook, common::Status::InternalError, "Failed to update book");
        return;
    }

    sendSuccess(client, common::Command::UpdateBook, bookToJson(updatedBook));
    delete existingBook;
    delete updatedBook;
    LOG_INFO("Book updated: " + bookId);
}

void PublisherRequestHandler::handleDeactivateBook(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::DeactivateBook, common::Status::BadRequest, "bookId is required");
        return;
    }

    if (!m_bookService->deactivateBook(bookId)) {
        sendError(client, common::Command::DeactivateBook, common::Status::NotFound, "Book not found");
        return;
    }

    sendSuccess(client, common::Command::DeactivateBook, {});
    LOG_INFO("Book deactivated: " + bookId);
}

void PublisherRequestHandler::handleActivateBook(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::ActivateBook, common::Status::BadRequest, "bookId is required");
        return;
    }

    if (!m_bookService->activateBook(bookId)) {
        sendError(client, common::Command::ActivateBook, common::Status::NotFound, "Book not found");
        return;
    }

    sendSuccess(client, common::Command::ActivateBook, {});
    LOG_INFO("Book activated: " + bookId);
}

void PublisherRequestHandler::handleApplyTimedDiscount(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    double discountValue = payload["discountValue"].toDouble();
    QString startDate = payload["startsAt"].toString();
    QString endDate = payload["endsAt"].toString();

    if (bookId.isEmpty() || discountValue <= 0) {
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::BadRequest,
                  "bookId and discountValue are required");
        return;
    }

    auto* discount = new common::Discount;
    discount->setId(common::IdGenerator::generateUuid());
    discount->setBookId(bookId);
    discount->setValue(discountValue);
    discount->setType(common::DiscountType::Percentage);
    discount->setActive(true);

    if (!m_bookService->applyDiscount(bookId, discount)) {
        delete discount;
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::InternalError,
                  "Failed to apply discount");
        return;
    }

    sendSuccess(client, common::Command::ApplyTimedDiscount, {});
    delete discount;
    LOG_INFO("Timed discount applied to book: " + bookId);
}

void PublisherRequestHandler::handleGetPublisherAnalytics(const QJsonObject& payload, ClientConnection* client)
{
    common::PublisherStats* stats = m_bookService->publisherStats(client->userId());
    if (!stats) {
        sendError(client, common::Command::GetPublisherAnalytics, common::Status::NotFound,
                  "No stats available");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["publisherId"] = stats->publisherId();
    responsePayload["totalBooks"] = stats->totalBooks();
    responsePayload["totalSales"] = stats->totalSales();
    responsePayload["totalRevenue"] = stats->totalRevenue();

    QJsonArray bookStatsArray;
    for (common::BookStatItem* item : stats->bookStats()) {
        QJsonObject itemObj;
        itemObj["bookId"] = item->bookId();
        itemObj["title"] = item->title();
        itemObj["salesCount"] = item->salesCount();
        itemObj["revenue"] = item->revenue();
        itemObj["averageRating"] = item->averageRating();
        bookStatsArray.append(itemObj);
    }
    responsePayload["bookStats"] = bookStatsArray;

    delete stats;
    sendSuccess(client, common::Command::GetPublisherAnalytics, responsePayload);
}

common::Book* PublisherRequestHandler::createBookFromPayload(const QJsonObject& payload)
{
    auto* book = new common::Book;

    if (payload.contains("title")) book->setTitle(payload["title"].toString());
    if (payload.contains("authorName")) book->setAuthorName(payload["authorName"].toString());

    if (payload.contains("genreIds")) {
        QStringList genres;
        for (const auto& val : payload["genreIds"].toArray()) {
            genres.append(val.toString());
        }
        book->setGenreIds(genres);
    }

    if (payload.contains("description")) book->setDescription(payload["description"].toString());
    if (payload.contains("coverImagePath")) book->setCoverImagePath(payload["coverImagePath"].toString());
    if (payload.contains("pdfFilePath")) book->setPdfFilePath(payload["pdfFilePath"].toString());
    if (payload.contains("basePrice")) book->setBasePrice(payload["basePrice"].toDouble());
    if (payload.contains("discountValue")) book->setDiscountValue(payload["discountValue"].toDouble());
    if (payload.contains("stockCount")) book->setStockCount(payload["stockCount"].toInt());

    book->setActive(true);
    book->setVisibility(common::BookVisibility::Visible);
    book->setAvailability(common::BookAvailability::Available);

    return book;
}

QJsonObject PublisherRequestHandler::bookToJson(common::Book* book) const
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
    obj["stockCount"] = book->stockCount();
    obj["isActive"] = book->isActive();
    return obj;
}

} // namespace bookclub::server
