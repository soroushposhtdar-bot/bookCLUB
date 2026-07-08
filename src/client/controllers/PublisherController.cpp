// src/client/controllers/PublisherController.cpp
#include "src/client/controllers/PublisherController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

namespace bookclub::client {

PublisherController::PublisherController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::GetPublisherBooks, [this](const common::Message& response) {
        handleGetPublisherBooksResponse(response);
    });

    network.registerRequestHandler(common::Command::PublishBook, [this](const common::Message& response) {
        handlePublishBookResponse(response);
    });

    network.registerRequestHandler(common::Command::UpdateBook, [this](const common::Message& response) {
        handleUpdateBookResponse(response);
    });

    network.registerRequestHandler(common::Command::DeactivateBook, [this](const common::Message& response) {
        handleDeactivateBookResponse(response);
    });

    network.registerRequestHandler(common::Command::ActivateBook, [this](const common::Message& response) {
        handleActivateBookResponse(response);
    });

    network.registerRequestHandler(common::Command::ApplyTimedDiscount, [this](const common::Message& response) {
        handleApplyTimedDiscountResponse(response);
    });

    network.registerRequestHandler(common::Command::GetPublisherAnalytics, [this](const common::Message& response) {
        handleGetPublisherAnalyticsResponse(response);
    });

    LOG_INFO("PublisherController initialized");
}

PublisherController::~PublisherController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::GetPublisherBooks);
    network.unregisterRequestHandler(common::Command::PublishBook);
    network.unregisterRequestHandler(common::Command::UpdateBook);
    network.unregisterRequestHandler(common::Command::DeactivateBook);
    network.unregisterRequestHandler(common::Command::ActivateBook);
    network.unregisterRequestHandler(common::Command::ApplyTimedDiscount);
    network.unregisterRequestHandler(common::Command::GetPublisherAnalytics);
}

// ---- Public Methods ----

void PublisherController::loadMyBooks()
{
    LOG_DEBUG("PublisherController::loadMyBooks() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load my books failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetPublisherBooks, {});
}

void PublisherController::createBook()
{
    LOG_DEBUG("PublisherController::createBook() called");

    if (m_bookData.isEmpty()) {
        LOG_WARNING("Create book failed: book data is empty");
        emit errorOccurred("Book data is empty. Please fill all required fields");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Create book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Validate required fields
    QStringList requiredFields = {"title", "authorName", "basePrice"};
    QString missingField;
    for (const QString& field : requiredFields) {
        if (!m_bookData.contains(field) || m_bookData[field].toString().isEmpty()) {
            missingField = field;
            break;
        }
    }

    if (!missingField.isEmpty()) {
        LOG_WARNING("Create book failed: missing required field: " + missingField);
        emit errorOccurred("Missing required field: " + missingField);
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::PublishBook, m_bookData);
}

void PublisherController::updateBook(const QString& bookId)
{
    LOG_DEBUG("PublisherController::updateBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Update book failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (m_bookData.isEmpty()) {
        LOG_WARNING("Update book failed: book data is empty");
        emit errorOccurred("Book data is empty. Please fill all required fields");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Update book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload = m_bookData;
    payload["id"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::UpdateBook, payload);
}

void PublisherController::deactivateBook(const QString& bookId)
{
    LOG_DEBUG("PublisherController::deactivateBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Deactivate book failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Deactivate book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::DeactivateBook, payload);
}

void PublisherController::activateBook(const QString& bookId)
{
    LOG_DEBUG("PublisherController::activateBook() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Activate book failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Activate book failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::ActivateBook, payload);
}

void PublisherController::applyTimedDiscount(const QString& bookId)
{
    LOG_DEBUG("PublisherController::applyTimedDiscount() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Apply timed discount failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (m_discountData.isEmpty()) {
        LOG_WARNING("Apply timed discount failed: discount data is empty");
        emit errorOccurred("Discount data is empty. Please set discount value and dates");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Apply timed discount failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload = m_discountData;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::ApplyTimedDiscount, payload);
}

void PublisherController::loadAnalytics()
{
    LOG_DEBUG("PublisherController::loadAnalytics() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load analytics failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetPublisherAnalytics, {});
}

// ---- Setter Methods ----

void PublisherController::setBookData(const QJsonObject& bookData)
{
    m_bookData = bookData;
    LOG_DEBUG("Book data updated");
}

void PublisherController::setBookField(const QString& field, const QJsonValue& value)
{
    m_bookData[field] = value;
    LOG_DEBUG("Book field updated: " + field);
}

void PublisherController::clearBookData()
{
    m_bookData = QJsonObject();
    LOG_DEBUG("Book data cleared");
}

void PublisherController::setDiscountData(const QJsonObject& discountData)
{
    m_discountData = discountData;
    LOG_DEBUG("Discount data updated");
}

void PublisherController::setDiscountField(const QString& field, const QJsonValue& value)
{
    m_discountData[field] = value;
    LOG_DEBUG("Discount field updated: " + field);
}

void PublisherController::clearDiscountData()
{
    m_discountData = QJsonObject();
    LOG_DEBUG("Discount data cleared");
}

// ---- Response Handlers ----

void PublisherController::handleGetPublisherBooksResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load publisher books");
        LOG_WARNING("Load publisher books failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QJsonArray books = data["books"].toArray();
    int count = data["count"].toInt();

    m_booksData = data;

    emit bookListChanged();

    LOG_INFO("Publisher books loaded successfully. Count: " + QString::number(count));
}

void PublisherController::handlePublishBookResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to publish book");
        LOG_WARNING("Publish book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QString bookId = data["id"].toString();
    QString title = data["title"].toString();

    // Clear book data after successful publication
    m_bookData = QJsonObject();

    emit bookListChanged();

    LOG_INFO("Book published successfully: " + title + " (ID: " + bookId + ")");
}

void PublisherController::handleUpdateBookResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to update book");
        LOG_WARNING("Update book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QString bookId = data["id"].toString();
    QString title = data["title"].toString();

    // Clear book data after successful update
    m_bookData = QJsonObject();

    emit bookListChanged();

    LOG_INFO("Book updated successfully: " + title + " (ID: " + bookId + ")");
}

void PublisherController::handleDeactivateBookResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to deactivate book");
        LOG_WARNING("Deactivate book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    emit bookListChanged();

    LOG_INFO("Book deactivated successfully");
}

void PublisherController::handleActivateBookResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to activate book");
        LOG_WARNING("Activate book failed: " + error);
        emit errorOccurred(error);
        return;
    }

    emit bookListChanged();

    LOG_INFO("Book activated successfully");
}

void PublisherController::handleApplyTimedDiscountResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to apply timed discount");
        LOG_WARNING("Apply timed discount failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Clear discount data after successful application
    m_discountData = QJsonObject();

    emit bookListChanged();

    LOG_INFO("Timed discount applied successfully");
}

void PublisherController::handleGetPublisherAnalyticsResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load analytics");
        LOG_WARNING("Load analytics failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_analyticsData = data;

    emit analyticsChanged();

    LOG_INFO("Publisher analytics loaded successfully. Total books: " +
             QString::number(data["totalBooks"].toInt()) +
             ", Total sales: " + QString::number(data["totalSales"].toInt()) +
             ", Total revenue: " + QString::number(data["totalRevenue"].toDouble()));
}

// ---- Helper Methods ----

QJsonArray PublisherController::getMyBooks() const
{
    return m_booksData["books"].toArray();
}

QJsonObject PublisherController::getBook(const QString& bookId) const
{
    QJsonArray books = m_booksData["books"].toArray();
    for (const auto& book : books) {
        QJsonObject bookObj = book.toObject();
        if (bookObj["id"].toString() == bookId) {
            return bookObj;
        }
    }
    return {};
}

int PublisherController::getBookCount() const
{
    return m_booksData["count"].toInt();
}

double PublisherController::getTotalRevenue() const
{
    return m_analyticsData["totalRevenue"].toDouble();
}

int PublisherController::getTotalSales() const
{
    return m_analyticsData["totalSales"].toInt();
}

int PublisherController::getTotalBooksPublished() const
{
    return m_analyticsData["totalBooks"].toInt();
}

QJsonArray PublisherController::getBookStats() const
{
    return m_analyticsData["bookStats"].toArray();
}

QJsonObject PublisherController::getBookStat(const QString& bookId) const
{
    QJsonArray stats = m_analyticsData["bookStats"].toArray();
    for (const auto& stat : stats) {
        QJsonObject statObj = stat.toObject();
        if (statObj["bookId"].toString() == bookId) {
            return statObj;
        }
    }
    return {};
}

} // namespace bookclub::client
