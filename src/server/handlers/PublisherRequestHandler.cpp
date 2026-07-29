#include "src/server/handlers/PublisherRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/NotificationDispatcher.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include "common/Utils/IdGenerator.h"
#include "common/Models/Discount.h"
#include "common/Models/Book.h"
#include "common/Models/PublisherStats.h"

namespace bookclub::server {

PublisherRequestHandler::PublisherRequestHandler(common::IBookService* bookService,
                                                 common::IBookRepository* bookRepo,
                                                 NotificationDispatcher* dispatcher,
                                                 QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_bookRepo(bookRepo)
    , m_dispatcher(dispatcher)
{
    LOG_INFO("PublisherRequestHandler initialized");
}

void PublisherRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    // Every Publisher command requires the Publisher role (admins may also use them).
    if (!requireAnyRole(client,
                        {common::AccountRole::Publisher, common::AccountRole::Admin},
                        request.command())) {
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
    // --- Input validation (C5) ---
    const QString title     = payload["title"].toString().trimmed();
    const QString authorName= payload["authorName"].toString().trimmed();
    const double basePrice  = payload["basePrice"].toDouble(-1);
    const QString desc      = payload["description"].toString();

    if (title.isEmpty() || title.length() > 200) {
        sendError(client, common::Command::PublishBook, common::Status::ValidationError,
                  "Title is required and must be 200 characters or fewer.");
        return;
    }
    if (authorName.isEmpty() || authorName.length() > 100) {
        sendError(client, common::Command::PublishBook, common::Status::ValidationError,
                  "Author name is required and must be 100 characters or fewer.");
        return;
    }
    if (basePrice < 0) {
        sendError(client, common::Command::PublishBook, common::Status::ValidationError,
                  "Price must be 0 or greater.");
        return;
    }
    if (desc.length() > 5000) {
        sendError(client, common::Command::PublishBook, common::Status::ValidationError,
                  "Description must be 5000 characters or fewer.");
        return;
    }

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

    const QString bookTitle = book->title();
    const QString bookId = book->id();
    const QStringList genres = book->genreIds();
    sendSuccess(client, common::Command::PublishBook, bookToJson(book));
    delete book;
    LOG_INFO("Book published: " + bookTitle + " by publisher: " + client->userId());

    // Real-time: notify every user whose favourite genres match the new
    // book's genres. The dispatcher pushes an EvtNotification to each
    // matching user that's currently online, and persists a copy for
    // offline users.
    if (m_dispatcher) {
        m_dispatcher->notifyNewBook(bookId, genres);
    }
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
    double discountPercent = payload["discountValue"].toDouble();
    QString startDate = payload["startsAt"].toString();
    QString endDate = payload["endsAt"].toString();

    // v15j: discountValue = 0 means "clear the discount" (remove it).
    if (bookId.isEmpty()) {
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::BadRequest,
                  "bookId is required");
        return;
    }

    if (discountPercent == 0.0) {
        // Clear the discount on this book.
        if (m_bookService->clearDiscount(bookId)) {
            // Broadcast so clients refresh.
            if (m_dispatcher) {
                m_dispatcher->broadcastDiscountApplied(bookId, 0.0);
            }
            sendSuccess(client, common::Command::ApplyTimedDiscount, {});
            LOG_INFO("Discount cleared for book: " + bookId);
            return;
        } else {
            sendError(client, common::Command::ApplyTimedDiscount, common::Status::InternalError,
                      "Failed to clear discount");
            return;
        }
    }

    if (discountPercent <= 0) {
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::BadRequest,
                  "discountValue must be positive (or 0 to clear)");
        return;
    }

    // v15i: validate discount range (1-100%)
    if (discountPercent > 100) {
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::ValidationError,
                  "Discount percentage must be between 1 and 100.");
        return;
    }

    // v15i: validate date range — end must be after start
    if (!startDate.isEmpty() && !endDate.isEmpty()) {
        QDateTime start = QDateTime::fromString(startDate, Qt::ISODate);
        QDateTime end = QDateTime::fromString(endDate, Qt::ISODate);
        if (start.isValid() && end.isValid() && end <= start) {
            sendError(client, common::Command::ApplyTimedDiscount, common::Status::ValidationError,
                      "End date must be after start date.");
            return;
        }
    }

    // v15i: check if the book already has an active discount. Reject
    // if so — each book can only have ONE discount at a time.
    auto* existingBook = m_bookRepo->findById(bookId);
    if (existingBook) {
        if (existingBook->discountValue() > 0.0) {
            delete existingBook;
            sendError(client, common::Command::ApplyTimedDiscount, common::Status::Conflict,
                      "This book already has an active discount. Remove it first before applying a new one.");
            return;
        }
        delete existingBook;
    }

    // v15i: compute the ABSOLUTE discount amount from the percentage.
    // The Books table stores discountValue as an absolute amount
    // (e.g. $15 off a $60 book), not a percentage. The client sends
    // a percentage (e.g. 25 for 25%). We need to convert:
    //   absoluteDiscount = basePrice * (percent / 100)
    auto* book = m_bookRepo->findById(bookId);
    if (!book) {
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::NotFound,
                  "Book not found");
        return;
    }
    double basePrice = book->basePrice();
    double absoluteDiscount = basePrice * (discountPercent / 100.0);
    delete book;

    auto* discount = new common::Discount;
    discount->setId(common::IdGenerator::generateUuid());
    discount->setBookId(bookId);
    discount->setValue(absoluteDiscount);  // store ABSOLUTE amount
    discount->setType(common::DiscountType::Percentage);
    discount->setActive(true);

    if (!m_bookService->applyDiscount(bookId, discount)) {
        delete discount;
        sendError(client, common::Command::ApplyTimedDiscount, common::Status::InternalError,
                  "Failed to apply discount");
        return;
    }

    // v15i: return the updated book JSON so the client can refresh
    // its cache immediately.
    auto* updatedBook = m_bookRepo->findById(bookId);
    QJsonObject responsePayload;
    if (updatedBook) {
        responsePayload["book"] = bookToJson(updatedBook);
        delete updatedBook;
    }
    responsePayload["discountPercent"] = discountPercent;
    responsePayload["discountValue"] = absoluteDiscount;
    sendSuccess(client, common::Command::ApplyTimedDiscount, responsePayload);
    delete discount;
    LOG_INFO("Timed discount applied to book: " + bookId +
             " (" + QString::number(discountPercent) + "% = " +
             QString::number(absoluteDiscount) + " off)");

    // Real-time: notify users about the discount
    if (m_dispatcher) {
        // v15i: broadcast EvtDiscountApplied to ALL connected clients so
        // they invalidate their book cache and show the discounted price.
        m_dispatcher->broadcastDiscountApplied(bookId, absoluteDiscount);
        // Also send a notification to users who have this book in their
        // wishlist (if any).
        QStringList empty;
        m_dispatcher->notifyDiscountOnBook(bookId, empty);
    }
}

void PublisherRequestHandler::handleGetPublisherAnalytics(const QJsonObject& payload, ClientConnection* client)
{
    common::PublisherStats* stats = m_bookService->publisherStats(client->userId());
    if (!stats) {
        sendError(client, common::Command::GetPublisherAnalytics, common::Status::NotFound,
                  "No stats available");
        return;
    }

    const QString publisherId = stats->publisherId();

    QJsonObject responsePayload;
    responsePayload["publisherId"] = publisherId;
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

    // ========================================================================
    // v22 (Issues 1 & 4): emit REAL chart data pulled directly from the
    // Orders / OrderItems / Books / Users / BookGenres / Genres tables so
    // the publisher dashboard's charts render historical truth instead of
    // deterministic PRNG noise synthesised client-side.
    //
    // Each block below is a self-contained SQL query; failures are non-fatal
    // and simply result in an empty array (the client already handles empty
    // series gracefully).
    // ========================================================================

    // ---- salesSeries: 30 daily points of units sold ----------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT date(o.createdAt) AS d, COUNT(*) AS cnt "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "GROUP BY d ORDER BY d DESC LIMIT 30");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            pt["label"] = q.value(0).toString();
            pt["value"] = q.value(1).toInt();
            arr.prepend(pt);   // we ordered DESC so prepend to chronological
        }
        responsePayload["salesSeries"] = arr;
    }

    // ---- revenueSeries: 30 daily points of revenue -----------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT date(o.createdAt) AS d, SUM(oi.lineTotal) AS rev "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "GROUP BY d ORDER BY d DESC LIMIT 30");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            pt["label"] = q.value(0).toString();
            pt["value"] = q.value(1).toDouble();
            arr.prepend(pt);
        }
        responsePayload["revenueSeries"] = arr;
    }

    // ---- monthlyRevenue: 12 monthly points -------------------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT strftime('%Y-%m', o.createdAt) AS m, SUM(oi.lineTotal) AS rev "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "GROUP BY m ORDER BY m DESC LIMIT 12");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            pt["label"] = q.value(0).toString();
            pt["value"] = q.value(1).toDouble();
            arr.prepend(pt);
        }
        responsePayload["monthlyRevenue"] = arr;
    }

    // ---- recentOrders: last 10 completed orders --------------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT o.id, u.displayName, b.title, b.id, oi.unitPrice, o.createdAt "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Users  u ON u.id = o.userId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "ORDER BY o.createdAt DESC LIMIT 10");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            pt["id"]        = q.value(0).toString();
            pt["customer"]  = q.value(1).toString();
            pt["bookTitle"] = q.value(2).toString();
            pt["bookId"]    = q.value(3).toString();
            pt["total"]     = q.value(4).toDouble();
            pt["time"]      = q.value(5).toDateTime().toString(Qt::ISODate);
            pt["status"]    = QStringLiteral("completed");
            arr.append(pt);
        }
        responsePayload["recentOrders"] = arr;
    }

    // ---- topBuyers: top 5 by total spent ---------------------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT u.id, u.displayName, SUM(oi.lineTotal) AS spent, "
                "       COUNT(DISTINCT oi.bookId) AS books, MAX(o.createdAt) AS last "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Users  u ON u.id = o.userId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "GROUP BY u.id ORDER BY spent DESC LIMIT 5");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            const QString name = q.value(1).toString();
            pt["userId"]      = q.value(0).toString();
            pt["displayName"] = name;
            // Initials: first letter of first + last word.
            const QStringList parts = name.split(' ', Qt::SkipEmptyParts);
            QString initials;
            if (!parts.isEmpty()) initials += parts.first().at(0);
            if (parts.size() > 1)  initials += parts.last().at(0);
            pt["initials"]    = initials;
            pt["books"]       = q.value(3).toInt();
            pt["lastOrder"]   = q.value(4).toDateTime().toString(Qt::ISODate);
            pt["totalSpent"]  = q.value(2).toDouble();
            arr.append(pt);
        }
        responsePayload["topBuyers"] = arr;
    }

    // ---- genreBreakdown: aggregate units sold by genre -------------------
    {
        QJsonArray arr;
        const QString sql =
            QStringLiteral(
                "SELECT g.name AS genre, SUM(oi.quantity) AS units "
                "FROM OrderItems oi "
                "JOIN Books       b  ON b.id = oi.bookId "
                "JOIN BookGenres  bg ON bg.bookId = b.id "
                "JOIN Genres      g  ON g.id = bg.genreId "
                "JOIN Orders      o  ON o.id = oi.orderId "
                "WHERE b.publisherId = ? AND o.completed = 1 "
                "GROUP BY g.name ORDER BY units DESC");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        while (q.next()) {
            QJsonObject pt;
            pt["name"]  = q.value(0).toString();
            pt["value"] = q.value(1).toInt();
            arr.append(pt);
        }
        responsePayload["genreBreakdown"] = arr;
    }

    // ---- averageRating (weighted) ----------------------------------------
    // SUM(b.averageRating * b.ratingCount) / NULLIF(SUM(b.ratingCount), 0)
    {
        const QString sql =
            QStringLiteral(
                "SELECT SUM(b.averageRating * b.ratingCount) / "
                "       NULLIF(SUM(b.ratingCount), 0) "
                "FROM Books b WHERE b.publisherId = ? AND b.isActive = 1");
        QSqlQuery q = common::DbConnection::run(sql, {publisherId});
        if (q.next()) {
            responsePayload["averageRating"] = q.value(0).toDouble();
        } else {
            responsePayload["averageRating"] = 0.0;
        }
    }

    // ---- repeatBuyerRate: % of buyers with >1 distinct book --------------
    // Of all distinct buyers, count those who bought >1 distinct book;
    // ratio is repeat_buyers / total_buyers * 100.
    {
        const QString sqlBuyers =
            QStringLiteral(
                "SELECT COUNT(DISTINCT o.userId) "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1");
        const QString sqlRepeat =
            QStringLiteral(
                "SELECT COUNT(*) FROM ("
                "  SELECT o.userId "
                "  FROM OrderItems oi "
                "  JOIN Orders o ON o.id = oi.orderId "
                "  JOIN Books  b ON b.id = oi.bookId "
                "  WHERE b.publisherId = ? AND o.completed = 1 "
                "  GROUP BY o.userId "
                "  HAVING COUNT(DISTINCT oi.bookId) > 1"
                ")");
        QSqlQuery qb = common::DbConnection::run(sqlBuyers, {publisherId});
        QSqlQuery qr = common::DbConnection::run(sqlRepeat, {publisherId});
        int totalBuyers = qb.next() ? qb.value(0).toInt() : 0;
        int repeatBuyers = qr.next() ? qr.value(0).toInt() : 0;
        int rate = (totalBuyers > 0)
                   ? static_cast<int>(repeatBuyers * 100.0 / totalBuyers)
                   : 0;
        responsePayload["repeatBuyerRate"] = rate;
    }

    // ---- revenueTrend + unitsSoldTrend -----------------------------------
    // Compare last 7 days vs previous 7 days for both revenue and units.
    {
        const QString sqlRev =
            QStringLiteral(
                "SELECT "
                "  COALESCE(SUM(CASE WHEN o.createdAt >= date('now','-7 days') THEN oi.lineTotal END),0) AS recent, "
                "  COALESCE(SUM(CASE WHEN o.createdAt <  date('now','-7 days') "
                "                    AND o.createdAt >= date('now','-14 days') THEN oi.lineTotal END),0) AS prev "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1");
        QSqlQuery q = common::DbConnection::run(sqlRev, {publisherId});
        if (q.next()) {
            const double recent = q.value(0).toDouble();
            const double prev   = q.value(1).toDouble();
            double trend = 0.0;
            if (prev > 0) trend = (recent - prev) / prev * 100.0;
            else if (recent > 0) trend = 100.0;
            const char sign = trend >= 0 ? '+' : '-';
            responsePayload["revenueTrend"] =
                QString("%1%2%").arg(sign).arg(std::abs(trend), 0, 'f', 1);
        } else {
            responsePayload["revenueTrend"] = QStringLiteral("+0.0%");
        }

        const QString sqlUnits =
            QStringLiteral(
                "SELECT "
                "  COALESCE(SUM(CASE WHEN o.createdAt >= date('now','-7 days') THEN oi.quantity END),0) AS recent, "
                "  COALESCE(SUM(CASE WHEN o.createdAt <  date('now','-7 days') "
                "                    AND o.createdAt >= date('now','-14 days') THEN oi.quantity END),0) AS prev "
                "FROM OrderItems oi "
                "JOIN Orders o ON o.id = oi.orderId "
                "JOIN Books  b ON b.id = oi.bookId "
                "WHERE b.publisherId = ? AND o.completed = 1");
        QSqlQuery qu = common::DbConnection::run(sqlUnits, {publisherId});
        if (qu.next()) {
            const double recent = qu.value(0).toDouble();
            const double prev   = qu.value(1).toDouble();
            double trend = 0.0;
            if (prev > 0) trend = (recent - prev) / prev * 100.0;
            else if (recent > 0) trend = 100.0;
            const char sign = trend >= 0 ? '+' : '-';
            responsePayload["unitsSoldTrend"] =
                QString("%1%2%").arg(sign).arg(std::abs(trend), 0, 'f', 1);
        } else {
            responsePayload["unitsSoldTrend"] = QStringLiteral("+0.0%");
        }
    }

    // ---- geographicBreakdown ---------------------------------------------
    // We don't collect geographic data, so emit an empty array (the QML
    // delegate renders an EmptyState when this is empty — better than the
    // previous fake Iranian-province distribution).
    responsePayload["geographicBreakdown"] = QJsonArray();

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
    // v22 (Issue 2): include rating + sales fields so the publisher catalog
    // table cards (which bind to model.totalSales / model.averageRating /
    // model.ratingCount) render real values instead of always 0.
    obj["averageRating"] = book->averageRating();
    obj["ratingCount"]   = book->ratingCount();
    obj["totalSales"]    = book->totalSales();
    obj["isActive"] = book->isActive();
    return obj;
}

} // namespace bookclub::server
