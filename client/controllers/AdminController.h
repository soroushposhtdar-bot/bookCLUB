#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class AdminController : public QObject {
    Q_OBJECT
public:
    explicit AdminController(QObject* parent = nullptr);
    ~AdminController() override = default;

    void loadUsers();
    void searchUsers(const QString& keyword);
    void blockUser(const QString& userId);
    void unblockUser(const QString& userId);
    void deleteUser(const QString& userId);
    void loadBooks();
    void moderateBook(const QString& bookId);
    void removeBook(const QString& bookId);

signals:
    void userListChanged();
    void bookListChanged();
    void errorOccurred(const QString& message);

};

} // namespace bookclub::client
