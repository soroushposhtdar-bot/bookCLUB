// common/Network/Serializer.h
#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include "common/Models/Book.h"
#include "common/Models/UserAccount.h"
// سایر مدل‌ها...

namespace bookclub::common {

class Serializer {
public:
    // تبدیل Book به JSON و برعکس
    static QJsonObject bookToJson(const Book* book);
    static Book* bookFromJson(const QJsonObject& json, QObject* parent = nullptr);

    // تبدیل UserAccount به JSON
    static QJsonObject userToJson(const UserAccount* user);
    static UserAccount* userFromJson(const QJsonObject& json, QObject* parent = nullptr);

    // لیست‌ها
    static QJsonArray bookListToJson(const QVector<Book*>& books);
    static QVector<Book*> bookListFromJson(const QJsonArray& array, QObject* parent = nullptr);
};

} // namespace bookclub::common
