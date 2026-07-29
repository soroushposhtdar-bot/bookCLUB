#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IOrderRepository.h"

namespace bookclub::server {

class NotificationDispatcher;

class CartRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit CartRequestHandler(common::IBookService* bookService,
                                common::IOrderRepository* orderRepo,
                                NotificationDispatcher* dispatcher = nullptr,
                                QObject* parent = nullptr);
    ~CartRequestHandler() override;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleAddToCart(const QJsonObject& payload, ClientConnection* client);
    void handleRemoveFromCart(const QJsonObject& payload, ClientConnection* client);
    void handleGetCart(const QJsonObject& payload, ClientConnection* client);
    void handleCheckout(const QJsonObject& payload, ClientConnection* client);
    void handleApplyDiscount(const QJsonObject& payload, ClientConnection* client);
    void handleClearCart(const QJsonObject& payload, ClientConnection* client);

    common::IBookService* m_bookService;
    common::IOrderRepository* m_orderRepo;
    NotificationDispatcher* m_dispatcher;
};

} // namespace bookclub::server
