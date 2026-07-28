// src/server/ServerCore.cpp
//
// Wires every Command in Protocol.h to its handler.
// Owns the ConnectionManager, RequestRouter, NotificationDispatcher and
// StudySessionManager.
#include "src/server/ServerCore.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/DatabaseManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"
#include "common/Utils/DbConnection.h"

// Handlers
#include "src/server/handlers/AuthRequestHandler.h"
#include "src/server/handlers/BookRequestHandler.h"
#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/handlers/PublisherRequestHandler.h"
#include "src/server/handlers/AdminRequestHandler.h"
#include "src/server/handlers/NotificationRequestHandler.h"
#include "src/server/handlers/StudySessionRequestHandler.h"
#include "src/server/handlers/ServerInfoRequestHandler.h"
#include "src/server/NotificationDispatcher.h"

// Interfaces (repository + service factories)
#include "common/Interfaces/IAuthService.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IOrderRepository.h"
#include "common/Interfaces/IReviewRepository.h"
#include "common/Interfaces/INotificationRepository.h"
#include "common/Interfaces/ICartRepository.h"
#include "common/Interfaces/IShelfRepository.h"
#include "common/Interfaces/IRatingRepository.h"
#include "common/Interfaces/IDiscountRepository.h"
#include "common/Interfaces/IGenreRepository.h"
#include "common/Interfaces/IAuthorRepository.h"
#include "common/Interfaces/IStudySessionRepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace bookclub::server {

ServerCore::ServerCore(QObject* parent)
    : QObject(parent)
{
    m_db = &DatabaseManager::instance();
    m_requestRouter = new RequestRouter(this);
    m_connectionManager = new ConnectionManager(m_requestRouter, this);
    m_studySessionManager = new StudySessionManager(m_connectionManager, this);
    m_notificationDispatcher = new NotificationDispatcher(m_connectionManager, this);

    // Forward connection events for logging / dashboard.
    connect(m_connectionManager, &ConnectionManager::clientConnected,
            this, &ServerCore::onClientConnected);
    connect(m_connectionManager, &ConnectionManager::clientDisconnected,
            this, &ServerCore::onClientDisconnected);
    connect(m_requestRouter, &RequestRouter::unhandledCommand,
            this, &ServerCore::onUnhandledCommand);
}

ServerCore::~ServerCore()
{
    shutdown();
}

bool ServerCore::initialize(const QString& dbPath, quint16 port)
{
    LOG_INFO("Initializing server...");

    // 1. Database
    if (!setupDatabase(dbPath)) {
        LOG_ERROR("Failed to setup database");
        return false;
    }

    // 2. Register handlers
    registerDefaultHandlers();

    // 3. Start TCP listener
    if (!m_connectionManager->startServer(port)) {
        LOG_ERROR("Failed to start server on port " + QString::number(port));
        return false;
    }

    m_port = port;
    m_running = true;
    LOG_INFO("Server initialized successfully on port " + QString::number(port));
    emit serverStarted(port);

    // Start background housekeeping (discount expiry, etc.).
    startHousekeepingTimers();

    return true;
}

void ServerCore::shutdown()
{
    if (!m_running) return;

    LOG_INFO("Shutting down server...");
    if (m_connectionManager) m_connectionManager->stopServer();
    m_running = false;
    emit serverStopped();
    LOG_INFO("Server shutdown complete");
}

bool ServerCore::isRunning() const
{
    return m_running && m_connectionManager && m_connectionManager->isRunning();
}

ConnectionManager* ServerCore::connectionManager() const { return m_connectionManager; }
RequestRouter* ServerCore::requestRouter() const { return m_requestRouter; }
NotificationDispatcher* ServerCore::notificationDispatcher() const { return m_notificationDispatcher; }
quint16 ServerCore::port() const { return m_port; }

void ServerCore::registerHandler(common::Command command, RequestHandlerBase* handler)
{
    if (m_requestRouter) {
        m_requestRouter->registerHandler(command, handler);
    }
}

bool ServerCore::setupDatabase(const QString& dbPath)
{
    QString dbDir = QFileInfo(dbPath).absolutePath();
    if (!common::FileSystemUtils::ensureDirectoryExists(dbDir)) {
        LOG_ERROR("Failed to create database directory: " + dbDir);
        return false;
    }

    if (!m_db->initialize(dbPath)) {
        LOG_ERROR("Failed to initialize database: " + m_db->lastError());
        return false;
    }

    LOG_INFO("Database initialized: " + dbPath);

    // Touch the shared connection so subsequent repository calls go through
    // the same named QSqlDatabase that DatabaseManager registered.
    common::DbConnection::database();

    return true;
}

void ServerCore::registerDefaultHandlers()
{
    LOG_INFO("Registering request handlers...");

    // ----- Create the repository/service singletons (dependency injection) -----
    auto* userRepo    = common::createUserRepository();
    auto* bookRepo    = common::createBookRepository();
    auto* orderRepo   = common::createOrderRepository();
    auto* reviewRepo  = common::createReviewRepository();
    auto* notifRepo   = common::createNotificationRepository();
    auto* cartRepo    = common::createCartRepository();
    auto* shelfRepo   = common::createShelfRepository();
    auto* ratingRepo  = common::createRatingRepository();
    auto* discountRepo= common::createDiscountRepository();
    auto* genreRepo   = common::createGenreRepository();
    auto* authorRepo  = common::createAuthorRepository();
    auto* sessionRepo = common::createStudySessionRepository();

    auto* authService = common::createAuthService();
    auto* bookService = common::createBookService(bookRepo, userRepo, orderRepo, reviewRepo);

    // ----- Create handlers (parented to ServerCore for cleanup) -----
    auto* authHandler      = new AuthRequestHandler(authService, userRepo, this);
    auto* bookHandler      = new BookRequestHandler(bookService, bookRepo, reviewRepo, ratingRepo, m_notificationDispatcher, this);
    auto* cartHandler      = new CartRequestHandler(bookService, orderRepo, m_notificationDispatcher, this);
    auto* libraryHandler   = new LibraryRequestHandler(userRepo, this);
    auto* publisherHandler = new PublisherRequestHandler(bookService, bookRepo, m_notificationDispatcher, this);
    auto* adminHandler     = new AdminRequestHandler(userRepo, bookRepo, reviewRepo, m_connectionManager, this);
    auto* notifHandler     = new NotificationRequestHandler(notifRepo, this);
    auto* studyHandler     = new StudySessionRequestHandler(m_studySessionManager, this);
    auto* serverInfoHandler= new ServerInfoRequestHandler(nullptr, m_connectionManager, this);

    // ----- Auth -----
    registerHandler(common::Command::Login,              authHandler);
    registerHandler(common::Command::Register,           authHandler);
    registerHandler(common::Command::ResetPassword,      authHandler);
    registerHandler(common::Command::ChangePassword,     authHandler);
    registerHandler(common::Command::Logout,             authHandler);
    registerHandler(common::Command::GetCurrentUser,     authHandler);
    registerHandler(common::Command::UpdateProfile,      authHandler);
    registerHandler(common::Command::SaveFavoriteGenres, authHandler);

    // ----- Books -----
    registerHandler(common::Command::GetHomeSections, bookHandler);
    registerHandler(common::Command::SearchBooks,     bookHandler);
    registerHandler(common::Command::GetBookDetails,  bookHandler);
    registerHandler(common::Command::GetBooksByIds,   bookHandler);
    registerHandler(common::Command::UploadBookPdf,   bookHandler);
    registerHandler(common::Command::DownloadBookPdf, bookHandler);

    // ----- Reviews + Ratings -----
    registerHandler(common::Command::SubmitReview,    bookHandler);
    registerHandler(common::Command::UpdateReview,    bookHandler);
    registerHandler(common::Command::DeleteReview,    bookHandler);
    registerHandler(common::Command::SetRating,       bookHandler);
    // Issue 7: helpful-mark + reply counters.
    registerHandler(common::Command::MarkReviewHelpful, bookHandler);
    registerHandler(common::Command::AddReviewReply,     bookHandler);

    // ----- Cart -----
    registerHandler(common::Command::AddToCart,       cartHandler);
    registerHandler(common::Command::RemoveFromCart,  cartHandler);
    registerHandler(common::Command::GetCart,         cartHandler);
    registerHandler(common::Command::Checkout,        cartHandler);
    registerHandler(common::Command::ApplyDiscount,   cartHandler);
    registerHandler(common::Command::ClearCart,       cartHandler);

    // ----- Library / Shelves -----
    registerHandler(common::Command::GetLibrary,           libraryHandler);
    registerHandler(common::Command::GetPurchasedBooks,    libraryHandler);
    registerHandler(common::Command::CreateShelf,          libraryHandler);
    registerHandler(common::Command::DeleteShelf,          libraryHandler);
    registerHandler(common::Command::AddBookToShelf,       libraryHandler);
    registerHandler(common::Command::RemoveBookFromShelf,  libraryHandler);
    registerHandler(common::Command::RenameShelf,          libraryHandler);
    registerHandler(common::Command::UpdateShelf,          libraryHandler);  // v15e

    // ----- Publisher -----
    registerHandler(common::Command::GetPublisherBooks,     publisherHandler);
    registerHandler(common::Command::PublishBook,           publisherHandler);
    registerHandler(common::Command::UpdateBook,            publisherHandler);
    registerHandler(common::Command::DeactivateBook,        publisherHandler);
    registerHandler(common::Command::ActivateBook,          publisherHandler);
    registerHandler(common::Command::ApplyTimedDiscount,    publisherHandler);
    registerHandler(common::Command::GetPublisherAnalytics, publisherHandler);

    // ----- Admin -----
    registerHandler(common::Command::GetUsersList,      adminHandler);
    registerHandler(common::Command::BlockUser,         adminHandler);
    registerHandler(common::Command::UnblockUser,       adminHandler);
    registerHandler(common::Command::DeleteUser,        adminHandler);
    registerHandler(common::Command::ModerateBook,      adminHandler);
    registerHandler(common::Command::RemoveBookByAdmin, adminHandler);
    registerHandler(common::Command::AdminDeleteReview, adminHandler);
    registerHandler(common::Command::AdminApproveReview, adminHandler);

    // ----- Notifications -----
    registerHandler(common::Command::GetNotifications,         notifHandler);
    registerHandler(common::Command::MarkNotificationRead,     notifHandler);
    registerHandler(common::Command::MarkAllNotificationsRead, notifHandler);

    // ----- Study Sessions -----
    registerHandler(common::Command::CreateStudySession,    studyHandler);
    registerHandler(common::Command::JoinStudySession,      studyHandler);
    registerHandler(common::Command::LeaveStudySession,     studyHandler);
    registerHandler(common::Command::SyncStudyPage,         studyHandler);
    registerHandler(common::Command::GetStudySessions,      studyHandler);  // Bug 2
    registerHandler(common::Command::SendStudyMessage,      studyHandler);  // Bug 3
    registerHandler(common::Command::InviteToStudySession,  studyHandler);  // Bug 4
    registerHandler(common::Command::AddStudyNote,          studyHandler);  // Bug 5
    registerHandler(common::Command::GetStudyNotes,         studyHandler);  // Bug 5

    // ----- Server / Operator -----
    registerHandler(common::Command::GetServerHealth,  serverInfoHandler);
    registerHandler(common::Command::GetServerLogs,    serverInfoHandler);
    registerHandler(common::Command::GetServerClients, serverInfoHandler);

    LOG_INFO("All request handlers registered successfully.");
}

void ServerCore::onClientConnected(const QString& clientId)
{
    LOG_INFO("Client connected: " + clientId);
}

void ServerCore::onClientDisconnected(const QString& clientId)
{
    LOG_INFO("Client disconnected: " + clientId);
}

void ServerCore::onUnhandledCommand(common::Command command, const QString& clientId)
{
    LOG_WARNING("Unhandled command " + common::commandToString(command) +
                " from client " + clientId);
}

void ServerCore::startHousekeepingTimers()
{
    // Discount expiry: every 60 seconds, deactivate discounts whose
    // endsAt < now. The IDiscountRepository::deactivateExpired() runs
    // a single UPDATE statement — cheap even with many discounts.
    m_discountExpiryTimer = new QTimer(this);
    m_discountExpiryTimer->setInterval(60000);  // 60s
    connect(m_discountExpiryTimer, &QTimer::timeout, this, []() {
        auto* discountRepo = common::createDiscountRepository();
        const int expired = discountRepo->deactivateExpired() ? 1 : 0;
        if (expired) {
            LOG_INFO("Discount expiry sweep completed");
        }
    });
    m_discountExpiryTimer->start();
    LOG_INFO("Housekeeping timers started (discount expiry: 60s interval)");
}

} // namespace bookclub::server
