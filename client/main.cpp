// =============================================================================
//  main.cpp — BookClub client entry point
// =============================================================================
//  Sets up the QML engine, registers every C++ type (auth + User ViewModels
//  + the six User-role services as singletons),
//  loads the bundled Material Symbols font, and shows App.qml.
//
//  Designed for Qt6. The QML_ELEMENT / QML_SINGLETON annotations are already
//  in place on the classes, but we keep the explicit qmlRegisterType /
//  qmlRegisterSingletonType calls for maximum compatibility.
// =============================================================================
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QFont>
#include <QIcon>
#include <QDir>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QtQml>

// Auth ViewModels + service
#include "services/AuthService.h"
#include "viewmodels/auth/AuthViewModelBase.h"
#include "viewmodels/auth/LoginViewModel.h"
#include "viewmodels/auth/RegisterViewModel.h"
#include "viewmodels/auth/ForgotPasswordViewModel.h"
#include "viewmodels/auth/ResetPasswordViewModel.h"
#include "viewmodels/auth/GenreSelectionViewModel.h"

// User-role services (singletons)
#include "services/BookService.h"
#include "services/CartService.h"
#include "services/LibraryService.h"
#include "services/NotificationService.h"
#include "services/ReaderService.h"
#include "services/UserService.h"
#include "services/BookDto.h"
#include "services/ReviewDto.h"
#include "services/CartItemDto.h"
#include "services/LibraryDtos.h"
#include "services/RatingDistDto.h"
#include "services/FilterChipDto.h"

// User-role ViewModels
#include "viewmodels/user/UserViewModelBase.h"
#include "viewmodels/user/HomeViewModel.h"
#include "viewmodels/user/SearchViewModel.h"
#include "viewmodels/user/BookDetailViewModel.h"
#include "viewmodels/user/CartViewModel.h"
#include "viewmodels/user/LibraryViewModel.h"
#include "viewmodels/user/ReaderViewModel.h"
#include "viewmodels/user/NotificationsViewModel.h"
#include "viewmodels/user/ProfileViewModel.h"
#include "viewmodels/user/WishlistViewModel.h"
#include "viewmodels/user/ShelfViewModel.h"
#include "viewmodels/user/StudySessionViewModel.h"

// Role services + ViewModels
#include "services/PublisherService.h"
#include "services/AdminService.h"
#include "services/ServerService.h"
#include "viewmodels/publisher/PublisherViewModel.h"
#include "viewmodels/admin/AdminViewModel.h"
#include "viewmodels/server/ServerViewModel.h"

// ----- AuthService singleton factory -----
// QML accesses this as `AuthService` (the singleton instance).
// IMPORTANT: we return the SAME static instance that AuthService::instance()
// returns, so that C++ services (UserService, PublisherService) which call
// AuthService::instance() read the same session state as QML.
static QObject* _authServiceSingletonProvider(QQmlEngine* engine, QJSEngine* /*scriptEngine*/) {
    bookclub::client::AuthService* instance = &bookclub::client::AuthService::instance();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

// ----- User-role service singleton factories -----
// Each User service is a singleton so every ViewModel shares the same
// instance. Each service is a self-contained singleton that talks to
// (App.qml::Component.onCompleted) so the data layer can be swapped without
// touching this file.

static QObject* _bookServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::BookService* instance = nullptr;
    if (!instance) instance = new bookclub::client::BookService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _cartServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::CartService* instance = nullptr;
    if (!instance) instance = new bookclub::client::CartService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _libraryServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::LibraryService* instance = nullptr;
    if (!instance) instance = new bookclub::client::LibraryService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _notificationServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::NotificationService* instance = nullptr;
    if (!instance) instance = new bookclub::client::NotificationService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _readerServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::ReaderService* instance = nullptr;
    if (!instance) instance = new bookclub::client::ReaderService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _userServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::UserService* instance = nullptr;
    if (!instance) instance = new bookclub::client::UserService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

// ----- Role service singleton factories (Publisher / Admin / Server) -----
static QObject* _publisherServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::PublisherService* instance = nullptr;
    if (!instance) instance = new bookclub::client::PublisherService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _adminServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::AdminService* instance = nullptr;
    if (!instance) instance = new bookclub::client::AdminService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

static QObject* _serverServiceSingletonProvider(QQmlEngine* engine, QJSEngine*) {
    static bookclub::client::ServerService* instance = nullptr;
    if (!instance) instance = new bookclub::client::ServerService();
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

int main(int argc, char* argv[]) {
    // Qt6 handles High-DPI automatically — no AA_EnableHighDpiScaling needed.

    // v6: Force English (United States) locale so ALL numbers render with
    // Western Arabic digits (0-9) regardless of the user's system locale.
    // Without this, Persian-locale systems render Persian-style digits
    // (۰۱۲۳۴۵۶۷۸۹) in every Text element, chart label, and KPI value —
    // which looks inconsistent and breaks number parsing.
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

    // CRITICAL: Set the QML import path BEFORE QGuiApplication is created.
    // windeployqt creates a qt.conf that overrides QLibraryInfo, so we must
    // set the environment variable to point to the real Qt QML directory.
#ifdef BOOKCLUB_QT_QML_DIR
    qputenv("QML2_IMPORT_PATH", QByteArray(BOOKCLUB_QT_QML_DIR));
    qputenv("QT_PLUGIN_PATH", QByteArray(BOOKCLUB_QT_QML_DIR) + "/.."); // plugins are sibling to qml
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("BookClub"));
    QGuiApplication::setOrganizationName(QStringLiteral("BookClub"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // ----- Load bundled fonts -----
    // v6: Load DejaVu Sans as the app's primary text font so the UI does
    // NOT fall back to device-local fonts (which can render Persian digits
    // and use inconsistent typefaces across platforms). DejaVu Sans is
    // bundled in client/resources/fonts/ and shipped inside the binary
    // via fonts.qrc.
    const int textFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSans.ttf"));
    const int boldFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSans-Bold.ttf"));
    const int monoFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/DejaVuSansMono.ttf"));

    if (textFontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(textFontId);
        qDebug() << "Loaded text font families:" << families;
        // Set as the application-wide default font so every widget that
        // doesn't explicitly set font.family picks it up.
        if (!families.isEmpty()) {
            QFont defaultFont(families.first());
            defaultFont.setPixelSize(14);
            QGuiApplication::setFont(defaultFont);
        }
    } else {
        qWarning() << "DejaVu Sans font not found at :/fonts/DejaVuSans.ttf —"
                   << "falling back to system fonts.";
    }

    // Material Symbols Outlined — icon font
    const int fontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/MaterialSymbolsOutlined-Regular.ttf"));
    if (fontId < 0) {
        qWarning() << "Material Symbols font not found at :/fonts/MaterialSymbolsOutlined-Regular.ttf —"
                   << "icons will not render. Add the file to client/resources/fonts/fonts.qrc.";
    } else {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        qDebug() << "Loaded icon font families:" << families;
    }

    // ----- Register C++ types with QML -----

    // Auth ViewModels
    qmlRegisterType<bookclub::client::LoginViewModel>          ("BookClub.ViewModels", 1, 0, "LoginViewModel");
    qmlRegisterType<bookclub::client::RegisterViewModel>       ("BookClub.ViewModels", 1, 0, "RegisterViewModel");
    qmlRegisterType<bookclub::client::ForgotPasswordViewModel> ("BookClub.ViewModels", 1, 0, "ForgotPasswordViewModel");
    qmlRegisterType<bookclub::client::ResetPasswordViewModel>  ("BookClub.ViewModels", 1, 0, "ResetPasswordViewModel");
    qmlRegisterType<bookclub::client::GenreSelectionViewModel> ("BookClub.ViewModels", 1, 0, "GenreSelectionViewModel");

    // User-role ViewModels
    qmlRegisterType<bookclub::client::HomeViewModel>           ("BookClub.ViewModels", 1, 0, "HomeViewModel");
    qmlRegisterType<bookclub::client::SearchViewModel>         ("BookClub.ViewModels", 1, 0, "SearchViewModel");
    qmlRegisterType<bookclub::client::BookDetailViewModel>     ("BookClub.ViewModels", 1, 0, "BookDetailViewModel");
    qmlRegisterType<bookclub::client::CartViewModel>           ("BookClub.ViewModels", 1, 0, "CartViewModel");
    qmlRegisterType<bookclub::client::LibraryViewModel>        ("BookClub.ViewModels", 1, 0, "LibraryViewModel");
    qmlRegisterType<bookclub::client::ReaderViewModel>         ("BookClub.ViewModels", 1, 0, "ReaderViewModel");
    qmlRegisterType<bookclub::client::NotificationsViewModel>  ("BookClub.ViewModels", 1, 0, "NotificationsViewModel");
    qmlRegisterType<bookclub::client::ProfileViewModel>        ("BookClub.ViewModels", 1, 0, "ProfileViewModel");
    qmlRegisterType<bookclub::client::WishlistViewModel>       ("BookClub.ViewModels", 1, 0, "WishlistViewModel");
    qmlRegisterType<bookclub::client::ShelfViewModel>          ("BookClub.ViewModels", 1, 0, "ShelfViewModel");
    qmlRegisterType<bookclub::client::StudySessionViewModel>   ("BookClub.ViewModels", 1, 0, "StudySessionViewModel");

    // Role ViewModels (Publisher / Admin / Server)
    qmlRegisterType<bookclub::client::PublisherViewModel>      ("BookClub.ViewModels", 1, 0, "PublisherViewModel");
    qmlRegisterType<bookclub::client::AdminViewModel>          ("BookClub.ViewModels", 1, 0, "AdminViewModel");
    qmlRegisterType<bookclub::client::ServerViewModel>         ("BookClub.ViewModels", 1, 0, "ServerViewModel");

    // Auth service (singleton)
    qmlRegisterSingletonType<bookclub::client::AuthService>(
        "BookClub.Services", 1, 0, "AuthService", _authServiceSingletonProvider);

    // User-role services (singletons)
    qmlRegisterSingletonType<bookclub::client::BookService>(
        "BookClub.Services", 1, 0, "BookService", _bookServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::CartService>(
        "BookClub.Services", 1, 0, "CartService", _cartServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::LibraryService>(
        "BookClub.Services", 1, 0, "LibraryService", _libraryServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::NotificationService>(
        "BookClub.Services", 1, 0, "NotificationService", _notificationServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::ReaderService>(
        "BookClub.Services", 1, 0, "ReaderService", _readerServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::UserService>(
        "BookClub.Services", 1, 0, "UserService", _userServiceSingletonProvider);

    // Role services (singletons)
    qmlRegisterSingletonType<bookclub::client::PublisherService>(
        "BookClub.Services", 1, 0, "PublisherService", _publisherServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::AdminService>(
        "BookClub.Services", 1, 0, "AdminService", _adminServiceSingletonProvider);
    qmlRegisterSingletonType<bookclub::client::ServerService>(
        "BookClub.Services", 1, 0, "ServerService", _serverServiceSingletonProvider);

    // DTO wrappers — registered so QML can recognize their types. They're
    // never instantiated from QML directly (the services create them).
    qmlRegisterType<bookclub::client::BookDto>("BookClub.Services", 1, 0, "BookDto");
    qmlRegisterType<bookclub::client::ReviewDto>("BookClub.Services", 1, 0, "ReviewDto");
    qmlRegisterType<bookclub::client::ShelfDto>("BookClub.Services", 1, 0, "ShelfDto");
    qmlRegisterType<bookclub::client::PurchaseDto>("BookClub.Services", 1, 0, "PurchaseDto");
    qmlRegisterType<bookclub::client::NotificationDto>("BookClub.Services", 1, 0, "NotificationDto");
    qmlRegisterType<bookclub::client::CartItemDto>("BookClub.Services", 1, 0, "CartItemDto");
    qmlRegisterType<bookclub::client::RatingDistDto>("BookClub.Services", 1, 0, "RatingDistDto");
    qmlRegisterType<bookclub::client::FilterChipDto>("BookClub.Services", 1, 0, "FilterChipDto");

    // ----- Load QML -----
    QQmlApplicationEngine engine;

    // CRITICAL: windeployqt creates a qt.conf next to the exe that overrides
    // QLibraryInfo paths. So QLibraryInfo::path(Qml2ImportsPath) returns a
    // path relative to the exe (which doesn't have the QML modules), not the
    // real Qt installation path.
    //
    // Solution: use the Qt QML directory that CMake baked into the binary
    // at compile time (BOOKCLUB_QT_QML_DIR). This is the absolute path to
    // the Qt installation's qml/ folder, e.g. "D:/apps/qt/6.11.1/msvc2022_64/qml".
#ifdef BOOKCLUB_QT_QML_DIR
    const QString qtQmlPath = QStringLiteral(BOOKCLUB_QT_QML_DIR);
    qDebug() << "BookClub: Qt QML dir (compile-time):" << qtQmlPath;
    engine.addImportPath(qtQmlPath);
#else
    // Fallback: ask QLibraryInfo (works when no qt.conf is present).
    const QString qtQmlPath = QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath);
    qDebug() << "BookClub: Qt QML dir (QLibraryInfo):" << qtQmlPath;
    engine.addImportPath(qtQmlPath);
#endif

    // Also add the exe-relative qml/ folder (where windeployqt copies things).
    const QString exeDir = QCoreApplication::applicationDirPath();
    engine.addImportPath(exeDir + QStringLiteral("/qml"));

    // Keep the qrc import path for our own QML files.
    engine.addImportPath(QStringLiteral("qrc:/qt/qml/bookclub/client/qml"));

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/bookclub/client/qml/App.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root — aborting.";
        return -1;
    }

    return app.exec();
}
