// src/client/controllers/ReaderController.cpp
#include "src/client/controllers/ReaderController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

namespace bookclub::client {

ReaderController::ReaderController(QObject* parent)
    : QObject(parent)
    , m_currentPage(0)
    , m_zoomLevel(1.0)
    , m_isBookOpen(false)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::SyncStudyPage, [this](const common::Message& response) {
        handleSyncStudyPageResponse(response);
    });

    LOG_INFO("ReaderController initialized");
}

ReaderController::~ReaderController()
{
    // Save current page if book is open
    if (m_isBookOpen && !m_bookId.isEmpty()) {
        saveCurrentPage();
    }
}

// ---- Public Methods ----

void ReaderController::openBook(const QString& bookId, const QString& pdfPath)
{
    LOG_DEBUG("ReaderController::openBook() called for book: " + bookId +
              ", PDF: " + pdfPath);

    if (bookId.isEmpty() || pdfPath.isEmpty()) {
        LOG_WARNING("Open book failed: book ID or PDF path is empty");
        emit readerError("Book ID and PDF path are required");
        return;
    }

    // Check if PDF file exists
    if (!common::FileSystemUtils::fileExists(pdfPath)) {
        LOG_WARNING("Open book failed: PDF file not found: " + pdfPath);
        emit readerError("PDF file not found: " + pdfPath);
        return;
    }

    // Close current book if any
    if (m_isBookOpen) {
        closeBook();
    }

    m_bookId = bookId;
    m_pdfPath = pdfPath;
    m_isBookOpen = true;
    m_currentPage = 0;

    // Try to load last saved page
    loadLastPage();

    emit bookOpened(bookId);
    emit pageChanged(m_currentPage);
    emit zoomChanged(m_zoomLevel);

    LOG_INFO("Book opened: " + bookId + " at page: " + QString::number(m_currentPage));
}

void ReaderController::closeBook()
{
    if (!m_isBookOpen) {
        return;
    }

    LOG_DEBUG("ReaderController::closeBook() called for book: " + m_bookId);

    // Save current page before closing
    saveCurrentPage();

    m_isBookOpen = false;
    m_currentPage = 0;

    LOG_INFO("Book closed: " + m_bookId);
    // Note: bookClosed signal is not defined in the header, so we don't emit it
}

void ReaderController::nextPage()
{
    if (!m_isBookOpen) {
        LOG_WARNING("Next page failed: no book is open");
        emit readerError("No book is open");
        return;
    }

    // In a real implementation, we would check if we're at the last page
    // For now, we just increment
    m_currentPage++;
    emit pageChanged(m_currentPage);

    // Save current page for future resume
    saveCurrentPage();

    LOG_DEBUG("Next page: " + QString::number(m_currentPage));
}

void ReaderController::previousPage()
{
    if (!m_isBookOpen) {
        LOG_WARNING("Previous page failed: no book is open");
        emit readerError("No book is open");
        return;
    }

    if (m_currentPage > 0) {
        m_currentPage--;
        emit pageChanged(m_currentPage);
        saveCurrentPage();
        LOG_DEBUG("Previous page: " + QString::number(m_currentPage));
    } else {
        LOG_DEBUG("Already at first page");
    }
}

void ReaderController::goToPage(int pageNumber)
{
    if (!m_isBookOpen) {
        LOG_WARNING("Go to page failed: no book is open");
        emit readerError("No book is open");
        return;
    }

    if (pageNumber < 0) {
        LOG_WARNING("Go to page failed: invalid page number: " + QString::number(pageNumber));
        emit readerError("Invalid page number");
        return;
    }

    m_currentPage = pageNumber;
    emit pageChanged(m_currentPage);
    saveCurrentPage();

    LOG_DEBUG("Go to page: " + QString::number(pageNumber));
}

void ReaderController::setZoom(double zoomLevel)
{
    if (zoomLevel < 0.1 || zoomLevel > 5.0) {
        LOG_WARNING("Set zoom failed: invalid zoom level: " + QString::number(zoomLevel));
        emit readerError("Invalid zoom level. Must be between 0.1 and 5.0");
        return;
    }

    m_zoomLevel = zoomLevel;
    emit zoomChanged(m_zoomLevel);

    LOG_DEBUG("Zoom set to: " + QString::number(zoomLevel));
}

void ReaderController::saveCurrentPage()
{
    if (!m_isBookOpen || m_bookId.isEmpty()) {
        return;
    }

    LOG_DEBUG("ReaderController::saveCurrentPage() called for book: " + m_bookId +
              ", page: " + QString::number(m_currentPage));

    // In a real implementation, you would save this to a database or file
    // For now, we'll just store it in a map (simulated)
    // We emit the signal to notify the UI that the page was saved
    emit lastPageSaved(m_currentPage);

    // If we have a study session, sync it
    // This will be handled by StudySessionController
    if (!m_sessionId.isEmpty()) {
        syncStudySession(m_sessionId);
    }
}

void ReaderController::loadLastPage()
{
    if (!m_isBookOpen || m_bookId.isEmpty()) {
        return;
    }

    LOG_DEBUG("ReaderController::loadLastPage() called for book: " + m_bookId);

    // In a real implementation, you would load from a database or file
    // For now, we just keep the current page (default: 0)
    // The UI can override this via goToPage()

    LOG_DEBUG("Last page loaded: " + QString::number(m_currentPage));
}

void ReaderController::syncStudySession(const QString& sessionId)
{
    LOG_DEBUG("ReaderController::syncStudySession() called for session: " + sessionId);

    if (!m_isBookOpen) {
        LOG_WARNING("Sync study session failed: no book is open");
        emit readerError("No book is open");
        return;
    }

    m_sessionId = sessionId;

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Sync study session failed: not connected to server");
        emit readerError("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["sessionId"] = sessionId;
    payload["page"] = m_currentPage;
    payload["zoom"] = m_zoomLevel;

    ClientNetworkManager::instance().sendRequest(common::Command::SyncStudyPage, payload);

    LOG_INFO("Study session synced: " + sessionId + ", page: " + QString::number(m_currentPage));
}

// ---- Response Handlers ----

void ReaderController::handleSyncStudyPageResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to sync study session");
        LOG_WARNING("Sync study session failed: " + error);
        emit readerError(error);
        return;
    }

    LOG_INFO("Study session synced successfully");
}

// ---- Helper Methods ----

QString ReaderController::getCurrentPageString() const
{
    if (!m_isBookOpen) {
        return "";
    }
    return QString::number(m_currentPage);
}

int ReaderController::getTotalPages() const
{
    // In a real implementation, you would get this from the PDF document
    // For now, return a default value (e.g., 100)
    return 100;
}

QString ReaderController::getProgressText() const
{
    if (!m_isBookOpen) {
        return "";
    }
    int total = getTotalPages();
    return QString::number(m_currentPage) + " / " + QString::number(total);
}

double ReaderController::getProgressPercent() const
{
    if (!m_isBookOpen) {
        return 0.0;
    }
    int total = getTotalPages();
    if (total == 0) {
        return 0.0;
    }
    return (static_cast<double>(m_currentPage) / total) * 100.0;
}

bool ReaderController::hasNextPage() const
{
    if (!m_isBookOpen) {
        return false;
    }
    return m_currentPage < (getTotalPages() - 1);
}

bool ReaderController::hasPreviousPage() const
{
    if (!m_isBookOpen) {
        return false;
    }
    return m_currentPage > 0;
}

} // namespace bookclub::client
