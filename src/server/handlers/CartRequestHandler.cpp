#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Utils/Logger.h"

#include <QJsonArray>
#include "common/Utils/IdGenerator.h"
#include "common/Models/Order.h"
#include "common/Models/Cart.h"
#include "common/Models/CartItem.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>

namespace bookclub::server {

CartRequestHandler::CartRequestHandler(common::IBookService* bookService,
                                       common::IOrderRepository* orderRepo,
                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_bookService(bookService)
    , m_orderRepo(orderRepo)
{
    LOG_INFO("CartRequestHandler initialized");
}

CartRequestHandler::~CartRequestHandler()
{
    qDeleteAll(m_userCarts);
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
        case common::Command::AddToCart:
            handleAddToCart(payload, client);
            break;
        case common::Command::RemoveFromCart:
            handleRemoveFromCart(payload, client);
            break;
        case common::Command::GetCart:
            handleGetCart(payload, client);
            break;
        case common::Command::Checkout:
            handleCheckout(payload, client);
            break;
        case common::Command::ApplyDiscount:
            handleApplyDiscount(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void CartRequestHandler::handleAddToCart(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    int quantity = payload["quantity"].toInt(1);

    if (bookId.isEmpty()) {
        sendError(client, common::Command::AddToCart, common::Status::BadRequest, "bookId is required");
        return;
    }

    common::Cart* cart = getOrCreateCart(client->userId());

    // Load book info from the database so the cart item has the right
    // title, unit price, and stock availability. The previous version
    // left these as defaults (empty title, 0 price), which made the cart
    // total 0 and broke checkout.
    QSqlDatabase db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (!db.isValid()) {
        db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
        db.setDatabaseName("bookclub.db");
        db.open();
    }
    if (!db.isOpen()) db.open();

    QSqlQuery q(db);
    q.prepare("SELECT title, basePrice, discountValue, isActive, stockCount FROM Books WHERE id = ?");
    q.addBindValue(bookId);
    if (!q.exec() || !q.next()) {
        sendError(client, common::Command::AddToCart, common::Status::NotFound, "Book not found");
        return;
    }

    if (q.value(3).toInt() != 1) {
        sendError(client, common::Command::AddToCart, common::Status::BadRequest, "Book is not active");
        return;
    }

    auto* item = new common::CartItem(bookId);
    item->setBookTitle(q.value(0).toString());
    const double base = q.value(1).toDouble();
    const double discount = q.value(2).toDouble();
    item->setUnitPrice(base);
    item->setDiscountedUnitPrice(discount);
    item->setQuantity(quantity);
    cart->addItem(item);

    sendSuccess(client, common::Command::AddToCart, cartToJson(cart));
    LOG_INFO("Book added to cart: " + bookId + " for user: " + client->userId());
}

void CartRequestHandler::handleRemoveFromCart(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::RemoveFromCart, common::Status::BadRequest, "bookId is required");
        return;
    }

    common::Cart* cart = getOrCreateCart(client->userId());
    cart->removeItem(bookId);

    sendSuccess(client, common::Command::RemoveFromCart, cartToJson(cart));
}

void CartRequestHandler::handleGetCart(const QJsonObject& payload, ClientConnection* client)
{
    common::Cart* cart = getOrCreateCart(client->userId());
    sendSuccess(client, common::Command::GetCart, cartToJson(cart));
}

void CartRequestHandler::handleCheckout(const QJsonObject& /*payload*/, ClientConnection* client)
{
    common::Cart* cart = getOrCreateCart(client->userId());
    if (cart->isEmpty()) {
        sendError(client, common::Command::Checkout, common::Status::BadRequest, "Cart is empty");
        return;
    }

    common::Order* order = nullptr;
    if (!m_bookService->purchaseCart(cart, &order)) {
        sendError(client, common::Command::Checkout, common::Status::InternalError, "Checkout failed");
        return;
    }

    // Persist the order so it survives a server restart, and mark it paid+completed
    // (we treat checkout as a synchronous payment).
    order->setPaid(true);
    order->setCompleted(true);
    if (!m_orderRepo->save(order)) {
        delete order;
        sendError(client, common::Command::Checkout, common::Status::InternalError, "Failed to persist order");
        return;
    }

    // Bump Books.totalSales for every item purchased.
    {
        QSqlDatabase db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
        if (!db.isValid()) {
            db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        if (!db.isOpen()) db.open();
        for (const common::OrderItem* item : order->items()) {
            QSqlQuery q(db);
            q.prepare("UPDATE Books SET totalSales = totalSales + ?, updatedAt = ? WHERE id = ?");
            q.addBindValue(item->quantity());
            q.addBindValue(QDateTime::currentDateTime());
            q.addBindValue(item->bookId());
            q.exec();
        }
    }

    // Clear cart after successful checkout
    cart->clear();

    QJsonObject responsePayload;
    responsePayload["orderId"] = order->id();
    responsePayload["finalTotal"] = order->finalTotal();

    delete order;
    sendSuccess(client, common::Command::Checkout, responsePayload);
    LOG_INFO("Checkout completed for user: " + client->userId());
}

void CartRequestHandler::handleApplyDiscount(const QJsonObject& payload, ClientConnection* client)
{
    QString discountCode = payload["discountCode"].toString();
    if (discountCode.isEmpty()) {
        sendError(client, common::Command::ApplyDiscount, common::Status::BadRequest, "discountCode is required");
        return;
    }

    common::Cart* cart = getOrCreateCart(client->userId());
    // TODO: Validate discount code and apply
    cart->setDiscountTotal(10.0); // Example

    sendSuccess(client, common::Command::ApplyDiscount, cartToJson(cart));
}

common::Cart* CartRequestHandler::getOrCreateCart(const QString& userId)
{
    if (!m_userCarts.contains(userId)) {
        auto* cart = new common::Cart;
        cart->setUserId(userId);
        m_userCarts[userId] = cart;
    }
    return m_userCarts[userId];
}

QJsonObject CartRequestHandler::cartToJson(common::Cart* cart) const
{
    if (!cart) return {};

    QJsonObject obj;
    obj["subtotal"] = cart->subtotal();
    obj["discountTotal"] = cart->discountTotal();
    obj["total"] = cart->total();
    obj["itemCount"] = cart->itemCount();

    QJsonArray itemsArray;
    for (common::CartItem* item : cart->items()) {
        QJsonObject itemObj;
        itemObj["bookId"] = item->bookId();
        itemObj["bookTitle"] = item->bookTitle();
        itemObj["unitPrice"] = item->unitPrice();
        itemObj["discountedUnitPrice"] = item->discountedUnitPrice();
        itemObj["quantity"] = item->quantity();
        itemObj["lineTotal"] = item->lineTotal();
        itemsArray.append(itemObj);
    }
    obj["items"] = itemsArray;

    return obj;
}

} // namespace bookclub::server
