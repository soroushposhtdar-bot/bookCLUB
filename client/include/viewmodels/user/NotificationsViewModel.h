// =============================================================================
//  NotificationsViewModel.h
// =============================================================================
//  MVVM view-model for the upgraded Notifications Center.
//
//  Features:
//      • Category tabs (all/purchase/review/discount/recommendation/publisher/
//        system/security/reminder)
//      • Per-category unread count
//      • Search notifications
//      • Mark read / unread / all-read
//      • Delete + archive
//      • Real-time push (notificationReceived signal forwarded to UI)
// =============================================================================
#ifndef NOTIFICATIONSVIEWMODEL_H
#define NOTIFICATIONSVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

// Include full service headers so MOC sees complete types for Q_PROPERTY pointers.
#include "services/NotificationService.h"

namespace bookclub::client {


class NotificationsViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(NotificationService* service READ service WRITE setService NOTIFY serviceChanged)

    Q_PROPERTY(QString activeCategory READ activeCategory WRITE setActiveCategory NOTIFY activeCategoryChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

    Q_PROPERTY(QList<QObject*> notifications READ notifications NOTIFY notificationsChanged)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY notificationsChanged)
    Q_PROPERTY(int totalCount  READ totalCount  NOTIFY notificationsChanged)
    Q_PROPERTY(bool hasAny     READ hasAny      NOTIFY notificationsChanged)

    // Per-category counts (for the tab badges)
    Q_PROPERTY(int allCount READ allCount NOTIFY notificationsChanged)
    Q_PROPERTY(int purchaseCount READ purchaseCount NOTIFY notificationsChanged)
    Q_PROPERTY(int reviewCount READ reviewCount NOTIFY notificationsChanged)
    Q_PROPERTY(int discountCount READ discountCount NOTIFY notificationsChanged)
    Q_PROPERTY(int recommendationCount READ recommendationCount NOTIFY notificationsChanged)
    Q_PROPERTY(int publisherCount READ publisherCount NOTIFY notificationsChanged)
    Q_PROPERTY(int systemCount READ systemCount NOTIFY notificationsChanged)
    Q_PROPERTY(int securityCount READ securityCount NOTIFY notificationsChanged)
    Q_PROPERTY(int reminderCount READ reminderCount NOTIFY notificationsChanged)

public:
    explicit NotificationsViewModel(QObject* parent = nullptr);

    NotificationService* service() const { return m_service; }
    void setService(NotificationService* s);

    QString activeCategory() const { return m_activeCategory; }
    QString searchQuery() const { return m_searchQuery; }
    // BUG FIX (notification-topics): marked Q_INVOKABLE so QML can call
    // `viewModel.setActiveCategory(key)` directly. The WRITE function of a
    // Q_PROPERTY is technically callable from QML, but only if it's also
    // a slot or Q_INVOKABLE. Without this, the GenreChip's `onClicked`
    // handler was silently failing — the category never changed, so the
    // notifications list never re-filtered.
    Q_INVOKABLE void setActiveCategory(const QString& v) { if (m_activeCategory != v) { m_activeCategory = v; emit activeCategoryChanged(); emit notificationsChanged(); } }
    Q_INVOKABLE void setSearchQuery(const QString& v) { if (m_searchQuery != v) { m_searchQuery = v; emit searchQueryChanged(); emit notificationsChanged(); } }

    QList<QObject*> notifications();
    int unreadCount();
    int totalCount();
    bool hasAny() { return totalCount() > 0; }

    // BUG FIX (MSVC const-correctness): made non-const because they call
    // NotificationService::countByCategory() which is now non-const.
    int allCount() { return m_service ? m_service->countByCategory("all") : 0; }
    int purchaseCount() { return m_service ? m_service->countByCategory("purchase") : 0; }
    int reviewCount() { return m_service ? m_service->countByCategory("review") : 0; }
    int discountCount() { return m_service ? m_service->countByCategory("discount") : 0; }
    int recommendationCount() { return m_service ? m_service->countByCategory("recommendation") : 0; }
    int publisherCount() { return m_service ? m_service->countByCategory("publisher") : 0; }
    int systemCount() { return m_service ? m_service->countByCategory("system") : 0; }
    int securityCount() { return m_service ? m_service->countByCategory("security") : 0; }
    int reminderCount() { return m_service ? m_service->countByCategory("reminder") : 0; }

    Q_INVOKABLE void markRead(const QString& id);
    Q_INVOKABLE void markUnread(const QString& id);
    Q_INVOKABLE void markAllRead();
    Q_INVOKABLE void deleteNotification(const QString& id);
    Q_INVOKABLE void archiveNotification(const QString& id);

signals:
    void serviceChanged();
    void notificationsChanged();
    void activeCategoryChanged();
    void searchQueryChanged();
    void realtimeNotificationReceived(QObject* dto);

private:
    NotificationService* m_service = nullptr;
    QString m_activeCategory = "all";
    QString m_searchQuery;
};

} // namespace bookclub::client

#endif // NOTIFICATIONSVIEWMODEL_H
