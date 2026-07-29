// src/server/handlers/CartRequestHandler.cpp
//
// Cart handler now uses ICartRepository for persistent storage.
// The Checkout response includes the list of purchased book IDs so the
// client's LibraryService can refresh without an extra round-trip.
#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/NotificationDispatcher.h"
#include "src/server/DatabaseManager.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/ICartRepository.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Models/Order.h"
#include "common/Models/Cart.h"
#include "common/Models/CartItem.h"
#include "common/Utils/DbConnection.h"

#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <memory>
#include "common/Models/Book.h"

namespace bookclub::server {

CartRequestHandler::CartRequestHandler(common::IBookService* bookService,
                                       common::IOrderRepository* orderRepo,
                                       NotificationDispatcher* dispatcher,
                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_orderRepo(orderRepo)
    , m_dispatcher(dispatcher)
{
    LOG_INFO("CartRequestHandler initialized");
}

CartRequestHandler::~CartRequestHandler()
{
    // No owned state — carts are now persistent via ICartRepository.
}

void CartRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::AddToCart:        handleAddToCart(payload, client); break;
        case common::Command::RemoveFromCart:   handleRemoveFromCart(payload, client); break;
        case common::Command::GetCart:          handleGetCart(payload, client); break;
        case common::Command::Checkout:         handleCheckout(payload, client); break;
        case common::Command::ApplyDiscount:    handleApplyDiscount(payload, client); break;
        case common::Command::ClearCart:        handleClearCart(payload, client); break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

// Helper: serialise a Cart to JSON.
// Issue 12 — enrich each cart item with book metadata (author, publisher,
// cover colors, base price, rating) so the client's CartPage can render
// covers + authors + discount badges without an extra round-trip per item.
static QJsonObject cartToJson(common::Cart* cart)
{
    if (!cart) return {};
    QJsonObject obj;
    obj["subtotal"]      = cart->subtotal();
    obj["discountTotal"] = cart->discountTotal();
    obj["total"]         = cart->total();
    obj["itemCount"]     = cart->itemCount();

    // v15e: compute the per-item discount total (sum of per-book
    // discounts) so the client can display "Purchase profit" without
    // re-computing it. This is distinct from discountTotal (promo code).
    double perItemDiscount = 0.0;
    for (common::CartItem* item : cart->items()) {
        const double base = item->unitPrice();
        const double discounted = item->discountedUnitPrice();
        if (discounted > 0.0 && discounted < base) {
            perItemDiscount += (base - discounted) * item->quantity();
        }
    }
    obj["perItemDiscountTotal"] = perItemDiscount;

    // Pre-fetch all book IDs in one go so we can batch a single SQL query
    // instead of N queries (one per cart item).
    QStringList bookIds;
    for (common::CartItem* item : cart->items()) {
        bookIds.append(item->bookId());
    }

    // Build a map of bookId → book JSON for O(1) lookup below.
    QHash<QString, QJsonObject> bookMap;
    if (!bookIds.isEmpty()) {
        QString placeholders;
        for (int i = 0; i < bookIds.size(); ++i) {
            if (i > 0) placeholders += QStringLiteral(",");
            placeholders += QStringLiteral("?");
        }
        QVariantList args;
        for (const QString& id : bookIds) args.append(id);
        auto q = common::DbConnection::run(
            QStringLiteral("SELECT id, title, authorName, description, coverColor, "
                           "coverAccent, basePrice, discountValue, averageRating, "
                           "ratingCount, publisherId FROM Books WHERE id IN (") + placeholders + QStringLiteral(")"),
            args
        );
        while (q.next()) {
            QJsonObject b;
            b["bookId"]        = q.value(0).toString();
            b["bookTitle"]     = q.value(1).toString();
            b["authorName"]    = q.value(2).toString();
            b["description"]   = q.value(3).toString();
            b["coverColor"]    = q.value(4).toString();
            b["coverAccent"]   = q.value(5).toString();
            b["basePrice"]     = q.value(6).toDouble();
            b["discountValue"] = q.value(7).toDouble();
            b["averageRating"] = q.value(8).toDouble();
            b["ratingCount"]   = q.value(9).toInt();
            b["publisherId"]   = q.value(10).toString();
            bookMap.insert(q.value(0).toString(), b);
        }
        // Resolve publisher names in one more batch query.
        QStringList publisherIds;
        for (auto it = bookMap.begin(); it != bookMap.end(); ++it) {
            const QString pid = it.value().value("publisherId").toString();
            if (!pid.isEmpty() && !publisherIds.contains(pid)) publisherIds.append(pid);
        }
        QHash<QString, QString> publisherNames;
        if (!publisherIds.isEmpty()) {
            QString pPlaceholders;
            for (int i = 0; i < publisherIds.size(); ++i) {
                if (i > 0) pPlaceholders += QStringLiteral(",");
                pPlaceholders += QStringLiteral("?");
            }
            QVariantList pArgs;
            for (const QString& id : publisherIds) pArgs.append(id);
            auto pq = common::DbConnection::run(
                QStringLiteral("SELECT id, displayName FROM Users WHERE id IN (") + pPlaceholders + QStringLiteral(")"),
                pArgs
            );
            while (pq.next()) {
                publisherNames.insert(pq.value(0).toString(), pq.value(1).toString());
            }
        }
        for (auto it = bookMap.begin(); it != bookMap.end(); ++it) {
            const QString pid = it.value().value("publisherId").toString();
            it.value()["publisherName"] = publisherNames.value(pid, QString());
        }
    }

    QJsonArray itemsArray;
    for (common::CartItem* item : cart->items()) {
        QJsonObject itemObj;
        itemObj["bookId"]              = item->bookId();
        itemObj["bookTitle"]           = item->bookTitle();
        itemObj["unitPrice"]           = item->unitPrice();
        itemObj["discountedUnitPrice"] = item->discountedUnitPrice();
        itemObj["quantity"]            = item->quantity();
        itemObj["lineTotal"]           = item->lineTotal();

        // Merge in the book metadata we fetched above.
        const QJsonObject book = bookMap.value(item->bookId());
        if (!book.isEmpty()) {
            itemObj["authorName"]    = book.value("authorName");
            itemObj["publisherName"] = book.value("publisherName");
            itemObj["coverColor"]    = book.value("coverColor");
            itemObj["coverAccent"]   = book.value("coverAccent");
            itemObj["description"]   = book.value("description");
            itemObj["basePrice"]     = book.value("basePrice");
            itemObj["discountValue"] = book.value("discountValue");
            itemObj["averageRating"] = book.value("averageRating");
            itemObj["ratingCount"]   = book.value("ratingCount");
        }
        itemsArray.append(itemObj);
    }
    obj["items"] = itemsArray;
    return obj;
}

// Helper: check if a user has already purchased a book (completed order).
static bool hasUserPurchased(const QString& userId, const QString& bookId)
{
    auto q = common::DbConnection::run(
        "SELECT 1 FROM OrderItems oi "
        "JOIN Orders o ON o.id = oi.orderId "
        "WHERE o.userId = ? AND oi.bookId = ? AND o.completed = 1 LIMIT 1",
        {userId, bookId}
    );
    return q.next();
}

void CartRequestHandler::handleAddToCart(const QJsonObject& payload, ClientConnection* client)
{
    const QString bookId = payload["bookId"].toString();
    const int quantity   = payload["quantity"].toInt(1);
    if (bookId.isEmpty()) {
        sendError(client, common::Command::AddToCart, common::Status::BadRequest, "bookId is required");
        return;
    }

    // Verify the book exists and is active.
    auto* bookRepo = common::createBookRepository();
    std::unique_ptr<common::Book> book(bookRepo->findById(bookId));
    if (!book) {
        sendError(client, common::Command::AddToCart, common::Status::NotFound, "Book not found");
        return;
    }
    if (!book->isActive()) {
        sendError(client, common::Command::AddToCart, common::Status::BadRequest, "Book is not active");
        return;
    }

    // v15: Reject if the user has already purchased this book. Each book
    // can only be bought once — no duplicate purchases.
    if (hasUserPurchased(client->userId(), bookId)) {
        sendError(client, common::Command::AddToCart, common::Status::Conflict,
                  "You already own this book.");
        return;
    }

    auto* cartRepo = common::createCartRepository();
    // v15: always add with quantity = 1. The repo's addItem now SETs
    // (not bumps) the quantity, so even if the book is already in the
    // cart, it stays at 1.
    cartRepo->addItem(client->userId(), bookId, 1);

    // Return the fresh cart.
    std::unique_ptr<common::Cart> cart(cartRepo->getOrCreateForUser(client->userId()));
    sendSuccess(client, common::Command::AddToCart, cartToJson(cart.get()));
    LOG_INFO("Book added to cart: " + bookId + " for user: " + client->userId());
}

void CartRequestHandler::handleRemoveFromCart(const QJsonObject& payload, ClientConnection* client)
{
    const QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::RemoveFromCart, common::Status::BadRequest, "bookId is required");
        return;
    }

    auto* cartRepo = common::createCartRepository();
    cartRepo->removeItem(client->userId(), bookId);
    std::unique_ptr<common::Cart> cart(cartRepo->getOrCreateForUser(client->userId()));
    sendSuccess(client, common::Command::RemoveFromCart, cartToJson(cart.get()));
}

void CartRequestHandler::handleGetCart(const QJsonObject& /*payload*/, ClientConnection* client)
{
    auto* cartRepo = common::createCartRepository();
    std::unique_ptr<common::Cart> cart(cartRepo->getOrCreateForUser(client->userId()));
    sendSuccess(client, common::Command::GetCart, cartToJson(cart.get()));
}

void CartRequestHandler::handleClearCart(const QJsonObject& /*payload*/, ClientConnection* client)
{
    auto* cartRepo = common::createCartRepository();
    cartRepo->clear(client->userId());
    sendSuccess(client, common::Command::ClearCart, {});
}

void CartRequestHandler::handleCheckout(const QJsonObject& /*payload*/, ClientConnection* client)
{
    auto* cartRepo = common::createCartRepository();
    std::unique_ptr<common::Cart> cart(cartRepo->getOrCreateForUser(client->userId()));

    if (!cart || cart->isEmpty()) {
        sendError(client, common::Command::Checkout, common::Status::BadRequest, "Cart is empty");
        return;
    }

    common::Order* order = nullptr;
    if (!m_bookService->purchaseCart(cart.get(), &order) || !order) {
        sendError(client, common::Command::Checkout, common::Status::InternalError, "Checkout failed");
        return;
    }

    order->setPaid(true);
    order->setCompleted(true);

    // Wrap the order persistence + book-sales bump in a transaction so a
    // crash mid-way doesn't leave the data inconsistent (order saved but
    // sales not bumped, or vice versa).
    auto& db = DatabaseManager::instance();
    const bool txStarted = db.beginTransaction();

    if (!m_orderRepo->save(order)) {
        delete order;
        if (txStarted) db.rollbackTransaction();
        sendError(client, common::Command::Checkout, common::Status::InternalError, "Failed to persist order");
        return;
    }

    // Bump Books.totalSales for every item purchased.
    for (const common::OrderItem* item : order->items()) {
        common::DbConnection::execOk(
            "UPDATE Books SET totalSales = totalSales + ?, updatedAt = ? WHERE id = ?",
            {item->quantity(), QDateTime::currentDateTime(), item->bookId()}
        );
    }

    if (txStarted) {
        if (!db.commitTransaction()) {
            db.rollbackTransaction();
            delete order;
            sendError(client, common::Command::Checkout, common::Status::InternalError,
                      "Failed to commit checkout transaction");
            return;
        }
    }

    // Clear the persistent cart now that checkout succeeded.
    cartRepo->clear(client->userId());

    // Build the response payload — include the purchased book IDs so the
    // client can refresh the library without an extra round-trip.
    QJsonArray purchasedIds;
    QStringList purchasedBookIds;  // for notification
    for (const common::OrderItem* item : order->items()) {
        purchasedIds.append(item->bookId());
        purchasedBookIds.append(item->bookId());
    }
    QJsonObject responsePayload;
    responsePayload["orderId"]         = order->id();
    responsePayload["finalTotal"]      = order->finalTotal();
    responsePayload["purchasedBookIds"] = purchasedIds;

    delete order;
    sendSuccess(client, common::Command::Checkout, responsePayload);
    LOG_INFO("Checkout completed for user: " + client->userId());

    // Real-time: notify each purchased book's publisher about the sale.
    // The dispatcher pushes an EvtNotification (type=SaleRegistered) to
    // the publisher if they're online, and persists a copy for offline.
    if (m_dispatcher) {
        auto* bookRepo = common::createBookRepository();
        for (const QString& bookId : purchasedBookIds) {
            std::unique_ptr<common::Book> book(bookRepo->findById(bookId));
            if (book) {
                m_dispatcher->notifyNewSale(bookId, book->publisherId());
            }
        }
    }
}

void CartRequestHandler::handleApplyDiscount(const QJsonObject& payload, ClientConnection* client)
{
    const QString discountCode = payload["discountCode"].toString();
    if (discountCode.isEmpty()) {
        sendError(client, common::Command::ApplyDiscount, common::Status::BadRequest,
                  "discountCode is required");
        return;
    }

    // 1. Load the current cart.
    auto* cartRepo = common::createCartRepository();
    std::unique_ptr<common::Cart> cart(cartRepo->getOrCreateForUser(client->userId()));
    if (!cart || cart->isEmpty()) {
        delete cartRepo;
        sendError(client, common::Command::ApplyDiscount, common::Status::BadRequest,
                  "Cart is empty");
        return;
    }

    // 2. Look up the discount code.
    const QDateTime now = QDateTime::currentDateTime();
    auto codeQuery = common::DbConnection::run(
        "SELECT id, type, value, minCartTotal, maxUses, usedCount "
        "FROM DiscountCodes "
        "WHERE code = ? AND isActive = 1 AND startsAt <= ? AND endsAt > ?",
        {discountCode, now, now}
    );

    if (!codeQuery.next()) {
        delete cartRepo;
        sendError(client, common::Command::ApplyDiscount, common::Status::NotFound,
                  "Invalid, expired, or inactive discount code");
        return;
    }

    const QString codeId       = codeQuery.value("id").toString();
    const int     codeType     = codeQuery.value("type").toInt();      // 0=Percentage, 1=Fixed
    const double  codeValue    = codeQuery.value("value").toDouble();
    const double  minCartTotal = codeQuery.value("minCartTotal").toDouble();
    const int     maxUses      = codeQuery.value("maxUses").toInt();
    const int     usedCount    = codeQuery.value("usedCount").toInt();

    // 3. Validate usage limits.
    if (maxUses > 0 && usedCount >= maxUses) {
        delete cartRepo;
        sendError(client, common::Command::ApplyDiscount, common::Status::BadRequest,
                  "Discount code has reached its usage limit");
        return;
    }

    // 4. Validate minimum cart total.
    if (cart->subtotal() < minCartTotal) {
        delete cartRepo;
        sendError(client, common::Command::ApplyDiscount, common::Status::BadRequest,
                  "Cart total does not meet the minimum required for this code (min: "
                  + QString::number(minCartTotal, 'f', 2) + ")");
        return;
    }

    // 5. Calculate the discount amount.
    double discountAmount = 0.0;
    if (codeType == 0) {
        // Percentage
        discountAmount = cart->subtotal() * (codeValue / 100.0);
    } else {
        // Fixed amount
        discountAmount = codeValue;
    }

    // Clamp: discount cannot exceed the cart subtotal.
    discountAmount = qMin(discountAmount, cart->subtotal());
    discountAmount = qMax(discountAmount, 0.0);

    // 6. Update the cart's discount total.
    cart->setDiscountTotal(discountAmount);
    cart->recalculateTotal();

    // 7. Increment the usage counter for the code.
    common::DbConnection::execOk(
        "UPDATE DiscountCodes SET usedCount = usedCount + 1 WHERE id = ?",
        {codeId}
    );

    // 8. Build response with discount info.
    QJsonObject cartResponse = cartToJson(cart.get());
    cartResponse["discountCode"]    = discountCode;
    cartResponse["discountAmount"] = discountAmount;
    cartResponse["discountType"]   = codeType;

    delete cartRepo;
    sendSuccess(client, common::Command::ApplyDiscount, cartResponse);
    LOG_INFO("Discount code applied: " + discountCode + " amount: "
             + QString::number(discountAmount, 'f', 2)
             + " for user: " + client->userId());
}

} // namespace bookclub::server
