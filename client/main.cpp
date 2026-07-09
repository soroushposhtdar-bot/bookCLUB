// =============================================================================
//  main.cpp — BookClub client entry point
// =============================================================================
//  Sets up the QML engine, registers C++ types (ViewModels + AuthService
//  singleton), loads the bundled Material Symbols font, and shows App.qml.
//
//  Designed for Qt5 (the project's currently configured version). Switch to
//  QML_ELEMENT-based registration when the project upgrades to Qt6.
// =============================================================================
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFontDatabase>
#include <QIcon>
#include <QtQml>

#include "services/AuthService.h"
#include "viewmodels/auth/AuthViewModelBase.h"
#include "viewmodels/auth/LoginViewModel.h"
#include "viewmodels/auth/RegisterViewModel.h"
#include "viewmodels/auth/ForgotPasswordViewModel.h"
#include "viewmodels/auth/ResetPasswordViewModel.h"
#include "viewmodels/auth/GenreSelectionViewModel.h"

// ----- AuthService singleton factory -----
// QML accesses this as `AuthService` (the singleton instance).
static QObject* _authServiceSingletonProvider(QQmlEngine* engine, QJSEngine* /*scriptEngine*/) {
    // The instance is owned by the QML engine — QML will delete it on engine
    // teardown. We mark it with CLOUD storage duration so it survives engine
    // reloads during development.
    static bookclub::client::AuthService* instance = nullptr;
    if (!instance) {
        instance = new bookclub::client::AuthService();
    }
    engine->setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

int main(int argc, char* argv[]) {
    // High-DPI defaults — produces crisp rendering on Retina / 4K displays.
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("BookClub"));
    QGuiApplication::setOrganizationName(QStringLiteral("BookClub"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // ----- Load Material Symbols font (if bundled) -----
    // The font file ships under client/resources/fonts/. If absent, AppIcon
    // glyphs render as the .notdef glyph — warn the user but continue.
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
    // ViewModels
    qmlRegisterType<bookclub::client::LoginViewModel>          ("BookClub.ViewModels", 1, 0, "LoginViewModel");
    qmlRegisterType<bookclub::client::RegisterViewModel>       ("BookClub.ViewModels", 1, 0, "RegisterViewModel");
    qmlRegisterType<bookclub::client::ForgotPasswordViewModel> ("BookClub.ViewModels", 1, 0, "ForgotPasswordViewModel");
    qmlRegisterType<bookclub::client::ResetPasswordViewModel>  ("BookClub.ViewModels", 1, 0, "ResetPasswordViewModel");
    qmlRegisterType<bookclub::client::GenreSelectionViewModel> ("BookClub.ViewModels", 1, 0, "GenreSelectionViewModel");

    // AuthService (singleton)
    qmlRegisterSingletonType<bookclub::client::AuthService>(
        "BookClub.Services", 1, 0, "AuthService", _authServiceSingletonProvider);

    // ----- Load QML -----
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml/bookclub/client/qml"));
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/bookclub/client/qml/App.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root — aborting.";
        return -1;
    }

    return app.exec();
}
