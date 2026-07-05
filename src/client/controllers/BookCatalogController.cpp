// src/client/controllers/BookCatalogController.cpp
#include "src/client/controllers/BookCatalogController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

BookCatalogController::BookCatalogController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::GetHomeSections, [this](const common::Message& response) {
        handleHomeSectionsResponse(response);
    });

    network.registerRequestHandler(common::Command::SearchBooks, [this](const common::Message& response) {
        handleSearchResponse(response);
    });

    network.registerRequestHandler(common::Command::GetBookDetails, [this](const common::Message& response) {
        handleBookDetailsResponse(response);
    });

    LOG_INFO("BookCatalogController initialized");
}

BookCatalogController::~BookCatalogController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::GetHomeSections);
    network.unregisterRequestHandler(common::Command::SearchBooks);
    network.unregisterRequestHandler(common::Command::GetBookDetails);
}

// ---- Public Methods ----

void BookCatalogController::loadHomeSections()
{
    LOG_DEBUG("BookCatalogController::loadHomeSections() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("loadHomeSections failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetHomeSections, {});
}

void BookCatalogController::search(const QString& keyword)
{
    LOG_DEBUG("BookCatalogController::search() called with keyword: " + keyword);

    if (keyword.isEmpty()) {
        LOG_WARNING("Search failed: keyword is empty");
        emit errorOccurred("Search keyword is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Search failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    m_lastKeyword = keyword;

    QJsonObject payload;
    payload["keyword"] = keyword;

    ClientNetworkManager::instance().sendRequest(common::Command::SearchBooks, payload);
}

void BookCatalogController::searchByAuthor(const QString& author)
{
    LOG_DEBUG("BookCatalogController::searchByAuthor() called with: " + author);

    if (author.isEmpty()) {
        LOG_WARNING("Search by author failed: author name is empty");
        emit errorOccurred("Author name is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Search by author failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["field"] = static_cast<int>(common::SearchField::Author);
    payload["keyword"] = author;

    ClientNetworkManager::instance().sendRequest(common::Command::SearchBooks, payload);
}

void BookCatalogController::searchByPublisher(const QString& publisher)
{
    LOG_DEBUG("BookCatalogController::searchByPublisher() called with: " + publisher);

    if (publisher.isEmpty()) {
        LOG_WARNING("Search by publisher failed: publisher name is empty");
        emit errorOccurred("Publisher name is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Search by publisher failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["field"] = static_cast<int>(common::SearchField::Publisher);
    payload["keyword"] = publisher;

    ClientNetworkManager::instance().sendRequest(common::Command::SearchBooks, payload);
}

void BookCatalogController::searchByGenre(const QString& genreId)
{
    LOG_DEBUG("BookCatalogController::searchByGenre() called with: " + genreId);

    if (genreId.isEmpty()) {
        LOG_WARNING("Search by genre failed: genre ID is empty");
        emit errorOccurred("Genre ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Search by genre failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["field"] = static_cast<int>(common::SearchField::Genre);
    payload["genreId"] = genreId;

    ClientNetworkManager::instance().sendRequest(common::Command::SearchBooks, payload);
}

void BookCatalogController::loadBookDetails(const QString& bookId)
{
    LOG_DEBUG("BookCatalogController::loadBookDetails() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Load book details failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load book details failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::GetBookDetails, payload);
}

// ---- Response Handlers ----

void BookCatalogController::handleHomeSectionsResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load home sections");
        LOG_WARNING("Home sections loading failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();

    // Parse featured books
    QJsonArray featuredArray = data["featured"].toArray();
    emit featuredBooksChanged();

    // Parse new books
    QJsonArray newBooksArray = data["newBooks"].toArray();
    emit newBooksChanged();

    // Parse bestsellers
    QJsonArray bestSellersArray = data["bestSellers"].toArray();
    emit bestSellersChanged();

    // Parse free books
    QJsonArray freeBooksArray = data["freeBooks"].toArray();
    emit freeBooksChanged();

    LOG_INFO("Home sections loaded successfully. Featured: " + QString::number(featuredArray.size()) +
             ", New: " + QString::number(newBooksArray.size()) +
             ", Bestsellers: " + QString::number(bestSellersArray.size()) +
             ", Free: " + QString::number(freeBooksArray.size()));
}

void BookCatalogController::handleSearchResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Search failed");
        LOG_WARNING("Search failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QJsonArray resultsArray = data["results"].toArray();
    int count = data["count"].toInt();

    // Store results for later use
    m_lastResults = resultsArray;

    emit searchResultsChanged();
    LOG_INFO("Search completed. Found " + QString::number(count) + " results");
}

void BookCatalogController::handleBookDetailsResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load book details");
        LOG_WARNING("Book details loading failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QString bookId = data["id"].toString();

    // Emit signal with book details
    emit bookDetailsLoaded(bookId);

    LOG_INFO("Book details loaded for book: " + bookId);
}

} // namespace bookclub::client
