// src/client/controllers/BookCatalogController.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class BookCatalogController : public QObject {
    Q_OBJECT
public:
    explicit BookCatalogController(QObject* parent = nullptr);
    ~BookCatalogController() override;

    void loadHomeSections();
    void search(const QString& keyword);
    void searchByAuthor(const QString& author);
    void searchByPublisher(const QString& publisher);
    void searchByGenre(const QString& genreId);
    void loadBookDetails(const QString& bookId);

signals:
    void featuredBooksChanged();
    void newBooksChanged();
    void bestSellersChanged();
    void freeBooksChanged();
    void searchResultsChanged();
    void bookDetailsLoaded(const QString& bookId);
    void errorOccurred(const QString& message);

private:
    void handleHomeSectionsResponse(const common::Message& response);
    void handleSearchResponse(const common::Message& response);
    void handleBookDetailsResponse(const common::Message& response);

    QString m_lastKeyword;
    QJsonArray m_lastResults;
};

} // namespace bookclub::client
