// common/Network/Serializer.cpp
#include "common/Network/Serializer.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"

namespace bookclub::common {

QJsonObject Serializer::bookToJson(const Book* book) {
    if (!book) return {};

    QJsonObject obj;
    obj["id"] = book->id();
    obj["title"] = book->title();
    obj["authorName"] = book->authorName();
    obj["publisherId"] = book->publisherId();
    obj["genreIds"] = QJsonArray::fromStringList(book->genreIds());
    obj["description"] = book->description();
    obj["coverImagePath"] = book->coverImagePath();
    obj["pdfFilePath"] = book->pdfFilePath();
    obj["price"] = book->price();
    obj["basePrice"] = book->basePrice();
    obj["discountValue"] = book->discountValue();
    obj["averageRating"] = book->averageRating();
    obj["ratingCount"] = book->ratingCount();
    obj["totalSales"] = book->totalSales();
    obj["stockCount"] = book->stockCount();
    obj["isFree"] = book->isFree();
    obj["isActive"] = book->isActive();
    obj["isTimedDiscountActive"] = book->isTimedDiscountActive();
    obj["visibility"] = static_cast<int>(book->visibility());
    obj["availability"] = static_cast<int>(book->availability());
    return obj;
}

Book* Serializer::bookFromJson(const QJsonObject& json, QObject* parent) {
    auto* book = new Book(parent);
    book->setId(json["id"].toString());
    book->setTitle(json["title"].toString());
    book->setAuthorName(json["authorName"].toString());
    book->setPublisherId(json["publisherId"].toString());

    QStringList genres;
    for (const auto& val : json["genreIds"].toArray()) {
        genres.append(val.toString());
    }
    book->setGenreIds(genres);

    book->setDescription(json["description"].toString());
    book->setCoverImagePath(json["coverImagePath"].toString());
    book->setPdfFilePath(json["pdfFilePath"].toString());
    book->setPrice(json["price"].toDouble());
    book->setBasePrice(json["basePrice"].toDouble());
    book->setDiscountValue(json["discountValue"].toDouble());
    book->setAverageRating(json["averageRating"].toDouble());
    book->setRatingCount(json["ratingCount"].toInt());
    book->setTotalSales(json["totalSales"].toInt());
    book->setStockCount(json["stockCount"].toInt());
    book->setVisibility(static_cast<BookVisibility>(json["visibility"].toInt()));
    book->setAvailability(static_cast<BookAvailability>(json["availability"].toInt()));
    return book;
}

QJsonArray Serializer::bookListToJson(const QVector<Book*>& books) {
    QJsonArray array;
    for (const auto* book : books) {
        array.append(bookToJson(book));
    }
    return array;
}

QVector<Book*> Serializer::bookListFromJson(const QJsonArray& array, QObject* parent) {
    QVector<Book*> books;
    for (const auto& val : array) {
        if (val.isObject()) {
            books.append(bookFromJson(val.toObject(), parent));
        }
    }
    return books;
}

QJsonObject Serializer::userToJson(const UserAccount* user) {
    if (!user) return {};

    QJsonObject obj;
    obj["id"] = user->id();
    obj["username"] = user->username();
    obj["displayName"] = user->displayName();
    obj["email"] = user->email();
    obj["phone"] = user->phone();
    obj["status"] = static_cast<int>(user->status());
    obj["role"] = static_cast<int>(user->role());

    // Fields specific to RegularUser
    if (auto* regular = dynamic_cast<const RegularUser*>(user)) {
        obj["favoriteGenreIds"] = QJsonArray::fromStringList(regular->favoriteGenreIds());
        obj["savedBookIds"] = QJsonArray::fromStringList(regular->savedBookIds());
        obj["purchasedBookIds"] = QJsonArray::fromStringList(regular->purchasedBookIds());
    }
    // Fields specific to Publisher
    if (auto* publisher = dynamic_cast<const Publisher*>(user)) {
        obj["publisherName"] = publisher->publisherName();
        obj["biography"] = publisher->biography();
        obj["website"] = publisher->website();
        obj["taxId"] = publisher->taxId();
    }
    return obj;
}

UserAccount* Serializer::userFromJson(const QJsonObject& json, QObject* parent) {
    auto role = static_cast<AccountRole>(json["role"].toInt());
    UserAccount* user = nullptr;

    switch (role) {
        case AccountRole::Admin:
            user = new Admin(parent);
            break;
        case AccountRole::Publisher:
            user = new Publisher(parent);
            break;
        case AccountRole::User:
        default:
            user = new RegularUser(parent);
            break;
    }

    user->setId(json["id"].toString());
    user->setUsername(json["username"].toString());
    user->setDisplayName(json["displayName"].toString());
    user->setEmail(json["email"].toString());
    user->setPhone(json["phone"].toString());
    user->setStatus(static_cast<AccountStatus>(json["status"].toInt()));

    if (auto* regular = dynamic_cast<RegularUser*>(user)) {
        QStringList genres;
        for (const auto& val : json["favoriteGenreIds"].toArray()) {
            genres.append(val.toString());
        }
        regular->setFavoriteGenreIds(genres);

        QStringList saved;
        for (const auto& val : json["savedBookIds"].toArray()) {
            saved.append(val.toString());
        }
        regular->setSavedBookIds(saved);

        QStringList purchased;
        for (const auto& val : json["purchasedBookIds"].toArray()) {
            purchased.append(val.toString());
        }
        regular->setPurchasedBookIds(purchased);
    }

    if (auto* publisher = dynamic_cast<Publisher*>(user)) {
        publisher->setPublisherName(json["publisherName"].toString());
        publisher->setBiography(json["biography"].toString());
        publisher->setWebsite(json["website"].toString());
        publisher->setTaxId(json["taxId"].toString());
    }

    return user;
}

} // namespace bookclub::common
