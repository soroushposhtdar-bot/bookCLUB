#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IOrderRepository.h"
#include "common/Models/Cart.h"

namespace bookclub::server {

class CartRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit CartRequestHandler(common::IBookService* bookService,
                                common::IOrderRepository* orderRepo,
                                QObject* parent = nullptr);
    ~CartRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleAddToCart(const QJsonObject& payload, ClientConnection* client);
    void handleRemoveFromCart(const QJsonObject& payload, ClientConnection* client);
    void handleGetCart(const QJsonObject& payload, ClientConnection* client);
    void handleCheckout(const QJsonObject& payload, ClientConnection* client);
    void handleApplyDiscount(const QJsonObject& payload, ClientConnection* client);

    common::Cart* getOrCreateCart(const QString& userId);
    QJsonObject cartToJson(common::Cart* cart) const;

    common::IBookService* m_bookService;
    common::IOrderRepository* m_orderRepo;
    QMap<QString, common::Cart*> m_userCarts;
};

} // namespace bookclub::server
