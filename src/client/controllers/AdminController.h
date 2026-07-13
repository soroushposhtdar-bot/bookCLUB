// src/client/controllers/AdminController.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class AdminController : public QObject {
    Q_OBJECT
public:
    explicit AdminController(QObject* parent = nullptr);
    ~AdminController() override;

    // ---- Public Methods ----
    void loadUsers();
    void searchUsers(const QString& keyword);
    void blockUser(const QString& userId);
    void unblockUser(const QString& userId);
    void deleteUser(const QString& userId);
    void loadBooks();
    void moderateBook(const QString& bookId);
    void removeBook(const QString& bookId);

    // ---- Accessors ----
    QJsonArray getUsers() const;
    int getUserCount() const;
    QJsonObject getUser(const QString& userId) const;
    QJsonArray getBooks() const;
    int getBookCount() const;
    void setBooksData(const QJsonObject& booksData);

signals:
    void userListChanged();
    void bookListChanged();
    void errorOccurred(const QString& message);

private:
    void handleGetUsersListResponse(const common::Message& response);
    void handleBlockUserResponse(const common::Message& response);
    void handleUnblockUserResponse(const common::Message& response);
    void handleDeleteUserResponse(const common::Message& response);
    void handleModerateBookResponse(const common::Message& response);
    void handleRemoveBookByAdminResponse(const common::Message& response);

    QJsonObject m_usersData;
    QJsonObject m_booksData;
    QString m_searchKeyword;   // set by searchUsers(), applied in handleGetUsersListResponse
};

} // namespace bookclub::client
