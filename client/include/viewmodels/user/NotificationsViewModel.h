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

namespace bookclub::client {

class NotificationService;

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
    void setActiveCategory(const QString& v) { if (m_activeCategory != v) { m_activeCategory = v; emit activeCategoryChanged(); emit notificationsChanged(); } }
    void setSearchQuery(const QString& v) { if (m_searchQuery != v) { m_searchQuery = v; emit searchQueryChanged(); emit notificationsChanged(); } }

    QList<QObject*> notifications() const;
    int unreadCount() const;
    int totalCount() const;
    bool hasAny() const { return totalCount() > 0; }

    int allCount() const { return m_service ? m_service->countByCategory("all") : 0; }
    int purchaseCount() const { return m_service ? m_service->countByCategory("purchase") : 0; }
    int reviewCount() const { return m_service ? m_service->countByCategory("review") : 0; }
    int discountCount() const { return m_service ? m_service->countByCategory("discount") : 0; }
    int recommendationCount() const { return m_service ? m_service->countByCategory("recommendation") : 0; }
    int publisherCount() const { return m_service ? m_service->countByCategory("publisher") : 0; }
    int systemCount() const { return m_service ? m_service->countByCategory("system") : 0; }
    int securityCount() const { return m_service ? m_service->countByCategory("security") : 0; }
    int reminderCount() const { return m_service ? m_service->countByCategory("reminder") : 0; }

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
