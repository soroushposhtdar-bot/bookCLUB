// =============================================================================
//  LibraryDtos.cpp
// =============================================================================
#include "services/LibraryDtos.h"

namespace bookclub::client {

ShelfDto::ShelfDto(const MockShelf& s, QObject* parent)
    : QObject(parent), m_s(s)
{}

PurchaseDto::PurchaseDto(const MockPurchase& p, QObject* parent)
    : QObject(parent), m_p(p)
{}

NotificationDto::NotificationDto(const MockNotification& n, QObject* parent)
    : QObject(parent), m_n(n)
{}

} // namespace bookclub::client
