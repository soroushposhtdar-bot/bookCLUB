#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QModelIndex>

namespace bookclub::client {

class NotificationListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        MessageRole,
        TypeRole,
        StateRole,
        CreatedAtRole
    };
    Q_ENUM(Roles)

    explicit NotificationListModel(QObject* parent = nullptr);
    ~NotificationListModel() override = default;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setNotificationIds(const QStringList& ids);

private:
    QStringList m_notificationIds;
};

} // namespace bookclub::client
