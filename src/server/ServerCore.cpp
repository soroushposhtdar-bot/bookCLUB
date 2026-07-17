// src/server/ServerCore.cpp
#include "src/server/ServerCore.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/DatabaseManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

// اضافه کردن هدر هندلرها
#include "src/server/handlers/AuthRequestHandler.h"
#include "src/server/handlers/BookRequestHandler.h"
#include "src/server/handlers/CartRequestHandler.h"
#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/handlers/PublisherRequestHandler.h"
#include "src/server/handlers/AdminRequestHandler.h"
#include "src/server/handlers/NotificationRequestHandler.h"
#include "src/server/handlers/StudySessionRequestHandler.h"
#include "src/server/NotificationDispatcher.h"

// اضافه کردن هدر سرویس‌ها و ریپازیتوری‌ها
#include "common/Interfaces/IAuthService.h"
#include "common/Interfaces/IBookService.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IOrderRepository.h"
#include "common/Interfaces/IReviewRepository.h"
#include "common/Interfaces/INotificationRepository.h"

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
    m_studySessionManager = new StudySessionManager(m_connectionManager, this); // ساخت مدیریت جلسات
    m_notificationDispatcher = new NotificationDispatcher(m_connectionManager, this);

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

bool ServerCore::initialize(const QString& dbPath, quint16 port)
{
    LOG_INFO("Initializing server...");

    // ۱. راه‌اندازی دیتابیس
    if (!setupDatabase(dbPath)) {
        LOG_ERROR("Failed to setup database");
        return false;
    }

    // ۲. ثبت هندلرها در روتر
    registerDefaultHandlers();

    // ۳. استارت سرور
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

ConnectionManager* ServerCore::connectionManager() const { return m_connectionManager; }
RequestRouter* ServerCore::requestRouter() const { return m_requestRouter; }
quint16 ServerCore::port() const { return m_port; }
NotificationDispatcher* ServerCore::notificationDispatcher() const { return m_notificationDispatcher; }

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

    // Schema + seed are applied inside DatabaseManager::initialize() via
    // runSchemaScript()/runSeedScript(), so there is nothing extra to do
    // here. The previous version tried to re-execute schema.sql as a single
    // multi-statement QSqlQuery::exec() call, which silently failed because
    // SQLite's exec() only runs one statement at a time.

    return true;
}

void ServerCore::registerDefaultHandlers()
{
    LOG_INFO("Registering request handlers...");

    // ۱. ساخت سرویس‌ها و ریپازیتوری‌ها (Dependency Injection)
    auto* userRepo = common::createUserRepository();
    auto* bookRepo = common::createBookRepository();
    auto* orderRepo = common::createOrderRepository();
    auto* reviewRepo = common::createReviewRepository();
    auto* notifRepo = common::createNotificationRepository();

    auto* authService = common::createAuthService();
    auto* bookService = common::createBookService(bookRepo, userRepo, orderRepo, reviewRepo);

    // ۲. ساخت هندلرها
    auto* authHandler = new AuthRequestHandler(authService, userRepo, this);
    auto* bookHandler = new BookRequestHandler(bookService, bookRepo, this);
    auto* cartHandler = new CartRequestHandler(bookService, orderRepo, this);
    auto* libraryHandler = new LibraryRequestHandler(userRepo, this);
    auto* publisherHandler = new PublisherRequestHandler(bookService, bookRepo, this);
    auto* adminHandler = new AdminRequestHandler(userRepo, bookRepo, this);
    auto* notifHandler = new NotificationRequestHandler(notifRepo, this);
    auto* studyHandler = new StudySessionRequestHandler(m_studySessionManager, this);

    // ۳. وصل کردن دستورات (Commands) به هندلرها
    // Auth
    registerHandler(common::Command::Login, authHandler);
    registerHandler(common::Command::Register, authHandler);
    registerHandler(common::Command::ResetPassword, authHandler);
    registerHandler(common::Command::ChangePassword, authHandler);
    registerHandler(common::Command::Logout, authHandler);

    // Books
    registerHandler(common::Command::GetHomeSections, bookHandler);
    registerHandler(common::Command::SearchBooks, bookHandler);
    registerHandler(common::Command::GetBookDetails, bookHandler);

    // Cart
    registerHandler(common::Command::AddToCart, cartHandler);
    registerHandler(common::Command::RemoveFromCart, cartHandler);
    registerHandler(common::Command::GetCart, cartHandler);
    registerHandler(common::Command::Checkout, cartHandler);
    registerHandler(common::Command::ApplyDiscount, cartHandler);

    // Library
    registerHandler(common::Command::GetLibrary, libraryHandler);
    registerHandler(common::Command::GetPurchasedBooks, libraryHandler);
    registerHandler(common::Command::CreateShelf, libraryHandler);
    registerHandler(common::Command::DeleteShelf, libraryHandler);
    registerHandler(common::Command::AddBookToShelf, libraryHandler);
    registerHandler(common::Command::RemoveBookFromShelf, libraryHandler);

    // Publisher
    registerHandler(common::Command::GetPublisherBooks, publisherHandler);
    registerHandler(common::Command::PublishBook, publisherHandler);
    registerHandler(common::Command::UpdateBook, publisherHandler);
    registerHandler(common::Command::DeactivateBook, publisherHandler);
    registerHandler(common::Command::ActivateBook, publisherHandler);
    registerHandler(common::Command::ApplyTimedDiscount, publisherHandler);
    registerHandler(common::Command::GetPublisherAnalytics, publisherHandler);

    // Admin
    registerHandler(common::Command::GetUsersList, adminHandler);
    registerHandler(common::Command::BlockUser, adminHandler);
    registerHandler(common::Command::UnblockUser, adminHandler);
    registerHandler(common::Command::DeleteUser, adminHandler);
    registerHandler(common::Command::ModerateBook, adminHandler);
    registerHandler(common::Command::RemoveBookByAdmin, adminHandler);

    // Notifications
    registerHandler(common::Command::GetNotifications, notifHandler);
    registerHandler(common::Command::MarkNotificationRead, notifHandler);
    registerHandler(common::Command::MarkAllNotificationsRead, notifHandler);

    // Study Sessions
    registerHandler(common::Command::CreateStudySession, studyHandler);
    registerHandler(common::Command::JoinStudySession, studyHandler);
    registerHandler(common::Command::LeaveStudySession, studyHandler);
    registerHandler(common::Command::SyncStudyPage, studyHandler);

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

} // namespace bookclub::server
