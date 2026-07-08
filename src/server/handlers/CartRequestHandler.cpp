#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Utils/Logger.h"

#include <QJsonArray>
#include "common/Utils/IdGenerator.h"
#include "common/Models/Order.h"
#include "common/Models/Cart.h"
#include "common/Models/CartItem.h"

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
    // Add book to cart (simplified)
    auto* item = new common::CartItem(bookId);
    // TODO: Load book details from database
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

void CartRequestHandler::handleCheckout(const QJsonObject& payload, ClientConnection* client)
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
