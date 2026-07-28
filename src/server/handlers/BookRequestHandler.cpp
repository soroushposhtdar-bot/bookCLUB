#include "src/server/handlers/BookRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/NotificationDispatcher.h"
#include "src/server/BookPdfStore.h"
#include "common/Utils/Logger.h"
#include "common/Models/Book.h"
#include "common/Models/Review.h"
#include "common/Utils/DbConnection.h"

#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QByteArray>
#include <memory>

namespace bookclub::server {

BookRequestHandler::BookRequestHandler(common::IBookService* bookService,
                                       common::IBookRepository* bookRepo,
                                       common::IReviewRepository* reviewRepo,
                                       common::IRatingRepository* ratingRepo,
                                       NotificationDispatcher* dispatcher,
                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_bookRepo(bookRepo)
    , m_reviewRepo(reviewRepo)
    , m_ratingRepo(ratingRepo)
    , m_dispatcher(dispatcher)
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
        case common::Command::GetHomeSections:  handleGetHomeSections(payload, client); break;
        case common::Command::SearchBooks:      handleSearchBooks(payload, client); break;
        case common::Command::GetBookDetails:   handleGetBookDetails(payload, client); break;
        case common::Command::GetBooksByIds:    handleGetBooksByIds(payload, client); break;
        case common::Command::UploadBookPdf:    handleUploadBookPdf(payload, client); break;
        case common::Command::DownloadBookPdf:  handleDownloadBookPdf(payload, client); break;
        case common::Command::SubmitReview:     handleSubmitReview(payload, client); break;
        case common::Command::UpdateReview:     handleUpdateReview(payload, client); break;
        case common::Command::DeleteReview:     handleDeleteReview(payload, client); break;
        case common::Command::SetRating:        handleSetRating(payload, client); break;
        // Issue 7: helpful-mark + reply counters.
        case common::Command::MarkReviewHelpful: handleMarkReviewHelpful(payload, client); break;
        case common::Command::AddReviewReply:   handleAddReviewReply(payload, client); break;
        default:
            LOG_WARNING("BookRequestHandler: Unhandled command " + common::commandToString(cmd));
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void BookRequestHandler::handleGetHomeSections(const QJsonObject& /*payload*/, ClientConnection* client)
{
    LOG_DEBUG("GetHomeSections request from client: " + client->clientId());

    QJsonObject responsePayload;
    responsePayload["featured"]   = bookListToJson(m_bookService->listFeaturedBooks());
    responsePayload["newBooks"]   = bookListToJson(m_bookService->listNewBooks());
    responsePayload["bestSellers"]= bookListToJson(m_bookService->listBestSellers());
    responsePayload["freeBooks"]  = bookListToJson(m_bookService->listFreeBooks());

    sendSuccess(client, common::Command::GetHomeSections, responsePayload);
}

void BookRequestHandler::handleSearchBooks(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("SearchBooks request from client: " + client->clientId());

    // Issue 15 — empty keyword returns all active books (so the Search
    // page can show a default catalog view before the user types).
    QString keyword = payload["keyword"].toString().trimmed();
    QString field   = payload["field"].toString().trimmed().toLower();

    QVector<common::Book*> results;
    if (keyword.isEmpty()) {
        results = m_bookService->listFeaturedBooks();
    } else if (field == QStringLiteral("title")) {
        results = m_bookService->searchBooksByField(keyword, common::SearchField::Title);
    } else if (field == QStringLiteral("author")) {
        results = m_bookService->searchBooksByField(keyword, common::SearchField::Author);
    } else if (field == QStringLiteral("publisher")) {
        results = m_bookService->searchBooksByField(keyword, common::SearchField::Publisher);
    } else if (field == QStringLiteral("genre")) {
        results = m_bookService->searchBooksByField(keyword, common::SearchField::Genre);
    } else {
        // "all" or unspecified — multi-field search.
        results = m_bookService->searchBooks(keyword);
    }

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
    delete book;

    // v15: Add `purchased` field so the client knows whether to show
    // "Buy now" or "Open in reader". We check the Orders/OrderItems
    // tables for a completed order containing this book by this user.
    if (client->isAuthenticated()) {
        auto ownQ = common::DbConnection::run(
            "SELECT 1 FROM OrderItems oi "
            "JOIN Orders o ON o.id = oi.orderId "
            "WHERE o.userId = ? AND oi.bookId = ? AND o.completed = 1 LIMIT 1",
            {client->userId(), bookId}
        );
        responsePayload["purchased"] = ownQ.next();
    } else {
        responsePayload["purchased"] = false;
    }

    // Add reviews
    QVector<common::Review*> reviews = m_bookRepo->reviewsOf(bookId);
    QJsonArray reviewsArray;
    for (common::Review* review : reviews) {
        reviewsArray.append(reviewToJson(review));
        delete review;
    }
    responsePayload["reviews"] = reviewsArray;

    sendSuccess(client, common::Command::GetBookDetails, responsePayload);
}

// =============================================================================
//  PDF upload/download — used by the in-app PDF reader.
//
//  UploadBookPdf
//    Request:  { bookId, fileData (base64), fileName }
//    Response: { ok: true, pdfFilePath: "pdfs/<bookId>.pdf", fileSize: <int> }
//
//  DownloadBookPdf
//    Request:  { bookId }
//    Response: { fileData (base64), fileName, fileSize }
//
//  The publisher (or admin) is the only role allowed to upload. Any
//  authenticated user can download — they only get the bookId back if they
//  actually own the book (the reader only fires for purchased books).
// =============================================================================
void BookRequestHandler::handleUploadBookPdf(const QJsonObject& payload, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, common::Command::UploadBookPdf, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    // Only publishers or admins may upload PDFs.
    if (!requireAnyRole(client,
                        {common::AccountRole::Publisher, common::AccountRole::Admin},
                        common::Command::UploadBookPdf)) {
        return;
    }

    const QString bookId = payload["bookId"].toString().trimmed();
    const QString fileDataB64 = payload["fileData"].toString();
    const QString fileName = payload["fileName"].toString().trimmed();

    if (bookId.isEmpty() || fileDataB64.isEmpty()) {
        sendError(client, common::Command::UploadBookPdf, common::Status::BadRequest,
                  "bookId and fileData are required");
        return;
    }

    // Ownership: the publisher must own the book (admins bypass).
    common::Book* book = m_bookRepo->findById(bookId);
    if (!book) {
        sendError(client, common::Command::UploadBookPdf, common::Status::NotFound,
                  "Book not found");
        return;
    }
    const QString publisherId = book->publisherId();
    // Determine admin status via the authenticated user's role.
    auto* userAcct = getAuthenticatedUser(client);
    const bool isAdmin = userAcct && userAcct->role() == common::AccountRole::Admin;
    delete userAcct;
    if (!isAdmin && publisherId != client->userId()) {
        delete book;
        sendError(client, common::Command::UploadBookPdf, common::Status::Forbidden,
                  "You don't own this book");
        return;
    }
    delete book;

    // Decode base64 → bytes.
    const QByteArray raw = QByteArray::fromBase64(fileDataB64.toUtf8(),
                                                   QByteArray::Base64Encoding);
    if (raw.isEmpty()) {
        sendError(client, common::Command::UploadBookPdf, common::Status::BadRequest,
                  "Invalid base64 PDF data");
        return;
    }

    // Persist the bytes.
    if (!BookPdfStore::instance().savePdf(bookId, raw)) {
        sendError(client, common::Command::UploadBookPdf, common::Status::InternalError,
                  "Failed to write PDF to disk");
        return;
    }

    // Update the Books row so GetBookDetails reports the new pdfFilePath.
    const QString relPath = BookPdfStore::relativePathForBook(bookId);
    common::DbConnection::execOk(
        "UPDATE Books SET pdfFilePath = ?, updatedAt = ? WHERE id = ?",
        {relPath, QDateTime::currentDateTime(), bookId}
    );

    QJsonObject responsePayload;
    responsePayload["ok"]          = true;
    responsePayload["pdfFilePath"] = relPath;
    responsePayload["fileSize"]    = qint64(raw.size());
    responsePayload["fileName"]    = fileName.isEmpty()
                                     ? (bookId + ".pdf")
                                     : fileName;
    sendSuccess(client, common::Command::UploadBookPdf, responsePayload);
    LOG_INFO("UploadBookPdf: bookId=" + bookId +
             " size=" + QString::number(raw.size()));
}

void BookRequestHandler::handleDownloadBookPdf(const QJsonObject& payload, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, common::Command::DownloadBookPdf, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    const QString bookId = payload["bookId"].toString().trimmed();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::DownloadBookPdf, common::Status::BadRequest,
                  "bookId is required");
        return;
    }

    if (!BookPdfStore::instance().hasPdf(bookId)) {
        sendError(client, common::Command::DownloadBookPdf, common::Status::NotFound,
                  "No PDF file is attached to this book");
        return;
    }

    const QByteArray raw = BookPdfStore::instance().loadPdf(bookId);
    if (raw.isEmpty()) {
        sendError(client, common::Command::DownloadBookPdf, common::Status::InternalError,
                  "Failed to read PDF from disk");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["fileData"] = QString::fromLatin1(raw.toBase64());
    responsePayload["fileSize"] = qint64(raw.size());
    responsePayload["fileName"] = bookId + ".pdf";
    sendSuccess(client, common::Command::DownloadBookPdf, responsePayload);
    LOG_INFO("DownloadBookPdf: bookId=" + bookId +
             " size=" + QString::number(raw.size()));
}

QJsonObject BookRequestHandler::bookToJson(common::Book* book) const
{
    if (!book) return {};

    QJsonObject obj;
    obj["id"]            = book->id();
    obj["title"]         = book->title();
    obj["authorName"]    = book->authorName();
    obj["publisherId"]   = book->publisherId();
    obj["genreIds"]      = QJsonArray::fromStringList(book->genreIds());
    obj["description"]   = book->description();
    obj["coverImagePath"]= book->coverImagePath();
    obj["pdfFilePath"]   = book->pdfFilePath();
    obj["price"]         = book->price();
    obj["basePrice"]     = book->basePrice();
    obj["discountValue"] = book->discountValue();
    obj["averageRating"] = book->averageRating();
    obj["ratingCount"]   = book->ratingCount();
    obj["totalSales"]    = book->totalSales();
    obj["stockCount"]    = book->stockCount();
    obj["isFree"]        = book->isFree();
    obj["isActive"]      = book->isActive();
    obj["visibility"]    = static_cast<int>(book->visibility());
    obj["availability"]  = static_cast<int>(book->availability());
    obj["createdAt"]     = book->createdAt().toString(Qt::ISODate);

    // Join publisher display name so the client doesn't need a second round-trip.
    auto q = common::DbConnection::run(
        "SELECT displayName FROM Users WHERE id = ?",
        {book->publisherId()}
    );
    if (q.next()) {
        obj["publisherName"] = q.value(0).toString();
    }
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

QJsonObject BookRequestHandler::reviewToJson(common::Review* review) const
{
    QJsonObject obj;
    if (!review) return obj;
    obj["id"]              = review->id();
    obj["bookId"]          = review->bookId();
    obj["userId"]          = review->userId();
    obj["userDisplayName"] = review->userDisplayName();
    obj["text"]            = review->text();
    obj["stars"]           = review->stars();
    obj["isEdited"]        = review->isEdited();
    // Issue 7: surface the helpful-mark + reply counters so the
    // client's ReviewItem can render "X found this helpful" and the
    // reply count badge.
    obj["helpfulCount"]    = review->helpfulCount();
    obj["replyCount"]      = review->replyCount();
    obj["createdAt"]       = review->createdAt().toString(Qt::ISODate);
    obj["updatedAt"]       = review->updatedAt().toString(Qt::ISODate);
    return obj;
}

void BookRequestHandler::handleGetBooksByIds(const QJsonObject& payload, ClientConnection* client)
{
    const QJsonArray idsArray = payload["bookIds"].toArray();
    if (idsArray.isEmpty()) {
        sendError(client, common::Command::GetBooksByIds, common::Status::BadRequest,
                  "bookIds array is required");
        return;
    }

    QJsonArray result;
    for (const QJsonValue& v : idsArray) {
        const QString id = v.toString();
        if (id.isEmpty()) continue;
        common::Book* book = m_bookRepo->findById(id);
        if (book) {
            result.append(bookToJson(book));
            delete book;
        }
    }
    QJsonObject responsePayload;
    responsePayload["books"] = result;
    responsePayload["count"] = static_cast<int>(result.size());
    sendSuccess(client, common::Command::GetBooksByIds, responsePayload);
}

// ============================================================================
//  Reviews + Ratings
// ============================================================================

void BookRequestHandler::handleSubmitReview(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::SubmitReview, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    const QString bookId = payload["bookId"].toString();
    const int stars      = payload["stars"].toInt(0);
    const QString text   = payload["text"].toString().trimmed();

    // --- Input validation (C5) ---
    if (bookId.isEmpty()) {
        sendError(client, common::Command::SubmitReview, common::Status::ValidationError,
                  "bookId is required");
        return;
    }
    if (stars < 1 || stars > 5) {
        sendError(client, common::Command::SubmitReview, common::Status::ValidationError,
                  "stars must be between 1 and 5");
        return;
    }
    if (text.isEmpty()) {
        sendError(client, common::Command::SubmitReview, common::Status::ValidationError,
                  "Review text cannot be empty");
        return;
    }
    if (text.length() > 5000) {
        sendError(client, common::Command::SubmitReview, common::Status::ValidationError,
                  "Review text must be 5000 characters or fewer");
        return;
    }

    // Verify the book exists.
    std::unique_ptr<common::Book> book(m_bookRepo->findById(bookId));
    if (!book) {
        sendError(client, common::Command::SubmitReview, common::Status::NotFound,
                  "Book not found");
        return;
    }

    // Look up the user's display name.
    auto userQ = common::DbConnection::run(
        "SELECT displayName FROM Users WHERE id = ?",
        {client->userId()}
    );
    QString displayName = userQ.next() ? userQ.value(0).toString() : QStringLiteral("User");

    auto* review = new common::Review;
    review->setBookId(bookId);
    review->setUserId(client->userId());
    review->setUserDisplayName(displayName);
    review->setText(text);
    review->setStars(stars);

    bool ok = false;
    if (m_reviewRepo) {
        ok = m_reviewRepo->save(review);
    } else {
        ok = m_bookService->addReview(review);
    }

    if (!ok) {
        delete review;
        sendError(client, common::Command::SubmitReview, common::Status::InternalError,
                  "Failed to save review");
        return;
    }

    QJsonObject responsePayload = reviewToJson(review);
    delete review;
    sendSuccess(client, common::Command::SubmitReview, responsePayload);
    LOG_INFO("Review submitted for book " + bookId + " by user " + client->userId());

    // Real-time: notify the book's publisher that a new review was posted.
    // The dispatcher pushes an EvtNotification (type=NewReview) to the
    // publisher if they're online, and persists a copy for offline.
    if (m_dispatcher) {
        // Look up the book's publisherId.
        std::unique_ptr<common::Book> book(m_bookRepo->findById(bookId));
        if (book) {
            m_dispatcher->notifyNewReview(bookId, book->publisherId());
        }
    }
}

void BookRequestHandler::handleUpdateReview(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::UpdateReview, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    const QString reviewId = payload["id"].toString();
    const int stars        = payload["stars"].toInt(0);
    const QString text     = payload["text"].toString();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::UpdateReview, common::Status::BadRequest,
                  "id is required");
        return;
    }

    if (!m_reviewRepo) {
        sendError(client, common::Command::UpdateReview, common::Status::InternalError,
                  "Review repository not available");
        return;
    }

    std::unique_ptr<common::Review> review(m_reviewRepo->findById(reviewId));
    if (!review) {
        sendError(client, common::Command::UpdateReview, common::Status::NotFound,
                  "Review not found");
        return;
    }

    // Only the original author can edit.
    if (review->userId() != client->userId()) {
        sendError(client, common::Command::UpdateReview, common::Status::Forbidden,
                  "You can only edit your own reviews");
        return;
    }

    if (stars >= 1 && stars <= 5) review->setStars(stars);
    if (!text.trimmed().isEmpty()) review->setText(text);

    if (!m_reviewRepo->update(review.get())) {
        sendError(client, common::Command::UpdateReview, common::Status::InternalError,
                  "Failed to update review");
        return;
    }

    QJsonObject responsePayload = reviewToJson(review.get());
    sendSuccess(client, common::Command::UpdateReview, responsePayload);
}

void BookRequestHandler::handleDeleteReview(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::DeleteReview, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    const QString reviewId = payload["id"].toString();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::DeleteReview, common::Status::BadRequest,
                  "id is required");
        return;
    }

    if (!m_reviewRepo) {
        sendError(client, common::Command::DeleteReview, common::Status::InternalError,
                  "Review repository not available");
        return;
    }

    // Verify ownership.
    std::unique_ptr<common::Review> review(m_reviewRepo->findById(reviewId));
    if (!review) {
        sendError(client, common::Command::DeleteReview, common::Status::NotFound,
                  "Review not found");
        return;
    }
    if (review->userId() != client->userId()) {
        sendError(client, common::Command::DeleteReview, common::Status::Forbidden,
                  "You can only delete your own reviews");
        return;
    }

    if (!m_reviewRepo->remove(reviewId)) {
        sendError(client, common::Command::DeleteReview, common::Status::InternalError,
                  "Failed to delete review");
        return;
    }

    sendSuccess(client, common::Command::DeleteReview, {});
}

void BookRequestHandler::handleSetRating(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::SetRating, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    const QString bookId = payload["bookId"].toString();
    const int stars      = payload["stars"].toInt(0);
    if (bookId.isEmpty() || stars < 1 || stars > 5) {
        sendError(client, common::Command::SetRating, common::Status::BadRequest,
                  "bookId and stars (1-5) are required");
        return;
    }

    if (!m_ratingRepo) {
        sendError(client, common::Command::SetRating, common::Status::InternalError,
                  "Rating repository not available");
        return;
    }

    if (!m_ratingRepo->setRating(bookId, client->userId(), stars)) {
        sendError(client, common::Command::SetRating, common::Status::InternalError,
                  "Failed to save rating");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["bookId"]        = bookId;
    responsePayload["averageRating"] = m_ratingRepo->averageRating(bookId);
    responsePayload["ratingCount"]   = m_ratingRepo->count(bookId);
    sendSuccess(client, common::Command::SetRating, responsePayload);
}

// ============================================================================
//  Issue 7: Review helpful-mark + reply counters
//
//  MarkReviewHelpful
//    Request:  { reviewId }
//    Response: { reviewId, helpfulCount }
//    Increments the helpfulCount column on the matching Reviews row.
//    The userId is taken from the authenticated session — we don't
//    model a per-user reactions table yet, so this is a straight
//    counter bump (a future schema can add a ReviewReactions table
//    and make this idempotent per-user).
//
//  AddReviewReply
//    Request:  { reviewId, text }
//    Response: { reviewId, replyCount }
//    Bumps the replyCount column. The reply text itself is logged
//    server-side (see IReviewRepository::addReply) but not persisted
//    as a separate row — the schema doesn't have a Replies table.
// ============================================================================
void BookRequestHandler::handleMarkReviewHelpful(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::MarkReviewHelpful, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }
    if (!m_reviewRepo) {
        sendError(client, common::Command::MarkReviewHelpful, common::Status::InternalError,
                  "Review repository not available");
        return;
    }
    const QString reviewId = payload["reviewId"].toString();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::MarkReviewHelpful, common::Status::BadRequest,
                  "reviewId is required");
        return;
    }
    if (!m_reviewRepo->markHelpful(reviewId, client->userId())) {
        sendError(client, common::Command::MarkReviewHelpful, common::Status::InternalError,
                  "Failed to mark review as helpful");
        return;
    }
    // Read back the fresh counter so the client can update the badge
    // without refetching the whole review list.
    int helpfulCount = 0;
    std::unique_ptr<common::Review> updated(m_reviewRepo->findById(reviewId));
    if (updated) helpfulCount = updated->helpfulCount();
    QJsonObject responsePayload;
    responsePayload["reviewId"]     = reviewId;
    responsePayload["helpfulCount"] = helpfulCount;
    sendSuccess(client, common::Command::MarkReviewHelpful, responsePayload);
}

void BookRequestHandler::handleAddReviewReply(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::AddReviewReply, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }
    if (!m_reviewRepo) {
        sendError(client, common::Command::AddReviewReply, common::Status::InternalError,
                  "Review repository not available");
        return;
    }
    const QString reviewId = payload["reviewId"].toString();
    const QString text     = payload["text"].toString().trimmed();
    if (reviewId.isEmpty()) {
        sendError(client, common::Command::AddReviewReply, common::Status::BadRequest,
                  "reviewId is required");
        return;
    }
    if (text.isEmpty()) {
        sendError(client, common::Command::AddReviewReply, common::Status::ValidationError,
                  "Reply text cannot be empty");
        return;
    }
    // Look up the user's display name for the (future) Replies table.
    auto userQ = common::DbConnection::run(
        "SELECT displayName FROM Users WHERE id = ?",
        {client->userId()}
    );
    const QString displayName = userQ.next() ? userQ.value(0).toString()
                                              : QStringLiteral("User");
    if (!m_reviewRepo->addReply(reviewId, client->userId(), displayName, text)) {
        sendError(client, common::Command::AddReviewReply, common::Status::InternalError,
                  "Failed to add reply");
        return;
    }
    int replyCount = 0;
    std::unique_ptr<common::Review> updated(m_reviewRepo->findById(reviewId));
    if (updated) replyCount = updated->replyCount();
    QJsonObject responsePayload;
    responsePayload["reviewId"]   = reviewId;
    responsePayload["replyCount"] = replyCount;
    sendSuccess(client, common::Command::AddReviewReply, responsePayload);
}

} // namespace bookclub::server
