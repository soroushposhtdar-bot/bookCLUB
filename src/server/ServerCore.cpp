// src/server/ServerCore.cpp
#include "src/server/ServerCore.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/DatabaseManager.h"

// ---- Include Handlers ----
#include "src/server/handlers/AuthRequestHandler.h"
#include "src/server/handlers/BookRequestHandler.h"
#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/handlers/PublisherRequestHandler.h"
#include "src/server/handlers/AdminRequestHandler.h"
#include "src/server/handlers/NotificationRequestHandler.h"
#include "src/server/handlers/StudySessionRequestHandler.h"

// ---- Include Services & Repositories (از common) ----
#include "common/Interfaces/IAuthService.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IOrderRepository.h"
#include "common/Interfaces/IReviewRepository.h"
#include "common/Interfaces/INotificationRepository.h"
#include "src/server/StudySessionManager.h"
#include "src/server/NotificationDispatcher.h"
// ---- Include concrete implementations (از common) ----
// این کلاس‌ها در common/Interfaces/ پیاده‌سازی شده‌اند
// و از توابع create برای ساخت نمونه استفاده می‌کنیم

#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QCoreApplication>
#include <QDir>


namespace bookclub::server {

// ---- Static instances for dependencies ----
static common::IAuthService* s_authService = nullptr;
static common::IUserRepository* s_userRepo = nullptr;
static common::IBookService* s_bookService = nullptr;
static common::IBookRepository* s_bookRepo = nullptr;
static common::IOrderRepository* s_orderRepo = nullptr;
static common::IReviewRepository* s_reviewRepo = nullptr;
static common::INotificationRepository* s_notifRepo = nullptr;

static StudySessionManager* s_sessionManager = nullptr;
static NotificationDispatcher* s_notifDispatcher = nullptr;

ServerCore::ServerCore(QObject* parent)
    : QObject(parent)
{
    m_db = &DatabaseManager::instance();
    m_requestRouter = new RequestRouter(this);
    m_connectionManager = new ConnectionManager(m_requestRouter, this);

    // Connect signals
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

// ============================================================
// initialize: راه‌اندازی سرور
// ============================================================
bool ServerCore::initialize(const QString& dbPath, quint16 port)
{
    LOG_INFO("Initializing server...");

    // 1. Setup database
    if (!setupDatabase(dbPath)) {
        LOG_ERROR("Failed to setup database");
        return false;
    }

    // 2. Initialize dependencies (سرویس‌ها و ریپازیتوری‌ها)
    initializeDependencies();

    // 3. Register default handlers
    registerDefaultHandlers();

    // 4. Start server
    if (!m_connectionManager->startServer(port)) {
        LOG_ERROR("Failed to start server on port " + QString::number(port));
        return false;
    }

    m_port = port;
    m_running = true;
    LOG_INFO("Server initialized successfully on port " + QString::number(port));
    emit serverStarted(port);
    return true;
}

void ServerCore::shutdown()
{
    if (!m_running) return;

    LOG_INFO("Shutting down server...");
    m_connectionManager->stopServer();
    m_running = false;
    emit serverStopped();
    LOG_INFO("Server shutdown complete");
}

bool ServerCore::isRunning() const
{
    return m_running && m_connectionManager && m_connectionManager->isRunning();
}

ConnectionManager* ServerCore::connectionManager() const
{
    return m_connectionManager;
}

RequestRouter* ServerCore::requestRouter() const
{
    return m_requestRouter;
}

quint16 ServerCore::port() const
{
    return m_port;
}

void ServerCore::registerHandler(common::Command command, RequestHandlerBase* handler)
{
    if (m_requestRouter) {
        m_requestRouter->registerHandler(command, handler);
    }
}

bool ServerCore::setupDatabase(const QString& dbPath)
{
    // Ensure directory exists
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
    return true;
}

// ============================================================
// initializeDependencies: ساخت سرویس‌ها و ریپازیتوری‌ها
// ============================================================
void ServerCore::initializeDependencies()
{
    if (s_authService) {
        return; // قبلاً ساخته شده
    }

    LOG_INFO("Initializing dependencies...");

    // ---- ساخت ریپازیتوری‌ها (با استفاده از توابع create در common) ----
    s_userRepo = common::createUserRepository();
    s_bookRepo = common::createBookRepository();
    s_orderRepo = common::createOrderRepository();
    s_reviewRepo = common::createReviewRepository();
    s_notifRepo = common::createNotificationRepository();

    // ---- ساخت سرویس‌ها ----
    s_authService = common::createAuthService();
    s_bookService = common::createBookService(s_bookRepo, s_userRepo, s_orderRepo, s_reviewRepo);

    // ---- ساخت مدیران (برای مطالعه گروهی و اعلان‌ها) ----
    s_sessionManager = new StudySessionManager(m_connectionManager);
    s_notifDispatcher = new NotificationDispatcher(m_connectionManager);

    LOG_INFO("Dependencies initialized successfully");
}

// ============================================================
// registerDefaultHandlers: ثبت تمام هندلرها در RequestRouter
// ============================================================
void ServerCore::registerDefaultHandlers()
{
    LOG_INFO("Registering default handlers...");

    // اطمینان از اینکه وابستگی‌ها ساخته شده‌اند
    if (!s_authService || !s_userRepo || !s_bookService || !s_bookRepo ||
        !s_orderRepo || !s_reviewRepo || !s_notifRepo) {
        LOG_ERROR("Dependencies not initialized. Cannot register handlers.");
        return;
    }

    // ---- ثبت Auth Handlers ----
    registerHandler(common::Command::Login,
                    new AuthRequestHandler(s_authService, s_userRepo, this));
    registerHandler(common::Command::Register,
                    new AuthRequestHandler(s_authService, s_userRepo, this));
    registerHandler(common::Command::ResetPassword,
                    new AuthRequestHandler(s_authService, s_userRepo, this));
    registerHandler(common::Command::ChangePassword,
                    new AuthRequestHandler(s_authService, s_userRepo, this));
    registerHandler(common::Command::Logout,
                    new AuthRequestHandler(s_authService, s_userRepo, this));

    // ---- ثبت Book Handlers ----
    registerHandler(common::Command::GetHomeSections,
                    new BookRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::SearchBooks,
                    new BookRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::GetBookDetails,
                    new BookRequestHandler(s_bookService, s_bookRepo, this));

    // ---- ثبت Cart Handlers ----
    registerHandler(common::Command::AddToCart,
                    new CartRequestHandler(s_bookService, s_orderRepo, this));
    registerHandler(common::Command::RemoveFromCart,
                    new CartRequestHandler(s_bookService, s_orderRepo, this));
    registerHandler(common::Command::GetCart,
                    new CartRequestHandler(s_bookService, s_orderRepo, this));
    registerHandler(common::Command::Checkout,
                    new CartRequestHandler(s_bookService, s_orderRepo, this));
    registerHandler(common::Command::ApplyDiscount,
                    new CartRequestHandler(s_bookService, s_orderRepo, this));

    // ---- ثبت Library Handlers ----
    registerHandler(common::Command::GetLibrary,
                    new LibraryRequestHandler(s_userRepo, this));
    registerHandler(common::Command::GetPurchasedBooks,
                    new LibraryRequestHandler(s_userRepo, this));
    registerHandler(common::Command::CreateShelf,
                    new LibraryRequestHandler(s_userRepo, this));
    registerHandler(common::Command::DeleteShelf,
                    new LibraryRequestHandler(s_userRepo, this));
    registerHandler(common::Command::AddBookToShelf,
                    new LibraryRequestHandler(s_userRepo, this));
    registerHandler(common::Command::RemoveBookFromShelf,
                    new LibraryRequestHandler(s_userRepo, this));

    // ---- ثبت Publisher Handlers ----
    registerHandler(common::Command::GetPublisherBooks,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::PublishBook,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::UpdateBook,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::DeactivateBook,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::ActivateBook,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::ApplyTimedDiscount,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));
    registerHandler(common::Command::GetPublisherAnalytics,
                    new PublisherRequestHandler(s_bookService, s_bookRepo, this));

    // ---- ثبت Admin Handlers ----
    registerHandler(common::Command::GetUsersList,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));
    registerHandler(common::Command::BlockUser,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));
    registerHandler(common::Command::UnblockUser,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));
    registerHandler(common::Command::DeleteUser,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));
    registerHandler(common::Command::ModerateBook,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));
    registerHandler(common::Command::RemoveBookByAdmin,
                    new AdminRequestHandler(s_userRepo, s_bookRepo, this));

    // ---- ثبت Notification Handlers ----
    registerHandler(common::Command::GetNotifications,
                    new NotificationRequestHandler(s_notifRepo, this));
    registerHandler(common::Command::MarkNotificationRead,
                    new NotificationRequestHandler(s_notifRepo, this));
    registerHandler(common::Command::MarkAllNotificationsRead,
                    new NotificationRequestHandler(s_notifRepo, this));

    // ---- ثبت Study Session Handlers (بخش امتیازی) ----
    if (s_sessionManager) {
        registerHandler(common::Command::CreateStudySession,
                        new StudySessionRequestHandler(s_sessionManager, this));
        registerHandler(common::Command::JoinStudySession,
                        new StudySessionRequestHandler(s_sessionManager, this));
        registerHandler(common::Command::LeaveStudySession,
                        new StudySessionRequestHandler(s_sessionManager, this));
        registerHandler(common::Command::SyncStudyPage,
                        new StudySessionRequestHandler(s_sessionManager, this));
    }

    LOG_INFO("All default handlers registered successfully");
}

// ============================================================
// Slots
// ============================================================
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

} // namespace bookclub::server
