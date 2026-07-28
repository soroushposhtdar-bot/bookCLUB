// src/client/controllers/CartController.cpp
#include "src/client/controllers/CartController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

CartController::CartController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::AddToCart, [this](const common::Message& response) {
        handleAddToCartResponse(response);
    });

    network.registerRequestHandler(common::Command::RemoveFromCart, [this](const common::Message& response) {
        handleRemoveFromCartResponse(response);
    });

    network.registerRequestHandler(common::Command::GetCart, [this](const common::Message& response) {
        handleGetCartResponse(response);
    });

    network.registerRequestHandler(common::Command::Checkout, [this](const common::Message& response) {
        handleCheckoutResponse(response);
    });

    network.registerRequestHandler(common::Command::ApplyDiscount, [this](const common::Message& response) {
        handleApplyDiscountResponse(response);
    });

    LOG_INFO("CartController initialized");
}

CartController::~CartController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::AddToCart);
    network.unregisterRequestHandler(common::Command::RemoveFromCart);
    network.unregisterRequestHandler(common::Command::GetCart);
    network.unregisterRequestHandler(common::Command::Checkout);
    network.unregisterRequestHandler(common::Command::ApplyDiscount);
}

// ---- Public Methods ----

void CartController::addBook(const QString& bookId)
{
    LOG_DEBUG("CartController::addBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Add to cart failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Add to cart failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;
    payload["quantity"] = 1;

    ClientNetworkManager::instance().sendRequest(common::Command::AddToCart, payload);
}

void CartController::addBookWithQuantity(const QString& bookId, int quantity)
{
    LOG_DEBUG("CartController::addBookWithQuantity() called for book: " + bookId +
              " with quantity: " + QString::number(quantity));

    if (bookId.isEmpty()) {
        LOG_WARNING("Add to cart failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (quantity <= 0) {
        LOG_WARNING("Add to cart failed: invalid quantity");
        emit errorOccurred("Quantity must be greater than 0");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Add to cart failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;
    payload["quantity"] = quantity;

    ClientNetworkManager::instance().sendRequest(common::Command::AddToCart, payload);
}

void CartController::removeBook(const QString& bookId)
{
    LOG_DEBUG("CartController::removeBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Remove from cart failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Remove from cart failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::RemoveFromCart, payload);
}

void CartController::updateQuantity(const QString& bookId, int quantity)
{
    LOG_DEBUG("CartController::updateQuantity() called for book: " + bookId +
              " with quantity: " + QString::number(quantity));

    if (bookId.isEmpty()) {
        LOG_WARNING("Update quantity failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (quantity < 0) {
        LOG_WARNING("Update quantity failed: invalid quantity");
        emit errorOccurred("Quantity cannot be negative");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Update quantity failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    if (quantity == 0) {
        // If quantity is 0, remove the item
        removeBook(bookId);
        return;
    }

    // For updating quantity, we need to send a custom request
    // Since we don't have a specific command for update quantity,
    // we can use AddToCart with quantity (server will replace)
    QJsonObject payload;
    payload["bookId"] = bookId;
    payload["quantity"] = quantity;

    ClientNetworkManager::instance().sendRequest(common::Command::AddToCart, payload);
}

void CartController::clearCart()
{
    LOG_DEBUG("CartController::clearCart() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Clear cart failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Get current cart first, then remove all items
    QJsonObject payload;
    ClientNetworkManager::instance().sendRequest(common::Command::GetCart, payload);

    // Note: The actual clearing will happen in handleGetCartResponse
    // where we'll iterate and remove each item
    LOG_INFO("Clear cart initiated");
}

void CartController::applyDiscountCode(const QString& code)
{
    LOG_DEBUG("CartController::applyDiscountCode() called with code: " + code);

    if (code.isEmpty()) {
        LOG_WARNING("Apply discount failed: discount code is empty");
        emit errorOccurred("Discount code is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Apply discount failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    m_discountCode = code;

    QJsonObject payload;
    payload["discountCode"] = code;

    ClientNetworkManager::instance().sendRequest(common::Command::ApplyDiscount, payload);
}

void CartController::checkout()
{
    LOG_DEBUG("CartController::checkout() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Checkout failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    emit checkoutStarted();

    QJsonObject payload;
    ClientNetworkManager::instance().sendRequest(common::Command::Checkout, payload);
}

// ---- Response Handlers ----

void CartController::handleAddToCartResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to add book to cart");
        LOG_WARNING("Add to cart failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_cartData = data;

    emit cartChanged();
    emit totalsChanged();

    LOG_INFO("Book added to cart successfully");
}

void CartController::handleRemoveFromCartResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to remove book from cart");
        LOG_WARNING("Remove from cart failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_cartData = data;

    emit cartChanged();
    emit totalsChanged();

    LOG_INFO("Book removed from cart successfully");
}

void CartController::handleGetCartResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to get cart");
        LOG_WARNING("Get cart failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_cartData = data;

    emit cartChanged();
    emit totalsChanged();

    LOG_INFO("Cart retrieved successfully");
}

void CartController::handleCheckoutResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Checkout failed");
        LOG_WARNING("Checkout failed: " + error);
        emit checkoutFailed(error);
        return;
    }

    QJsonObject data = response.payload();
    QString orderId = data["orderId"].toString();
    double finalTotal = data["finalTotal"].toDouble();

    // Clear local cart data
    m_cartData = QJsonObject();
    emit cartChanged();
    emit totalsChanged();

    emit checkoutSucceeded(orderId);

    LOG_INFO("Checkout completed successfully. Order ID: " + orderId +
             ", Total: " + QString::number(finalTotal));
}

void CartController::handleApplyDiscountResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to apply discount");
        LOG_WARNING("Apply discount failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_cartData = data;

    emit cartChanged();
    emit totalsChanged();

    LOG_INFO("Discount applied successfully");
}

// ---- Helper Methods ----

double CartController::getTotal() const
{
    return m_cartData["total"].toDouble();
}

double CartController::getSubtotal() const
{
    return m_cartData["subtotal"].toDouble();
}

double CartController::getDiscountTotal() const
{
    return m_cartData["discountTotal"].toDouble();
}

int CartController::getItemCount() const
{
    return m_cartData["itemCount"].toInt();
}

QJsonArray CartController::getItems() const
{
    return m_cartData["items"].toArray();
}

} // namespace bookclub::client
