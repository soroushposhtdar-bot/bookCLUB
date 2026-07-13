// =============================================================================
//  App.qml
// =============================================================================
//  Root application window.
//
//  Two-phase routing:
//      Phase 1 — Auth flow (SplashPage → WelcomePage → Login/Register/…)
//      Phase 2 — User dashboard (UserShell with sidebar + 9 pages)
//
//  StackView transitions:
//      • Auth pages use a subtle horizontal slide + fade.
//      • Hand-off from auth → dashboard uses a cross-fade.
//
//  Service singletons:
//      AuthService, BookService, CartService, LibraryService,
//      NotificationService, ReaderService, UserService — all registered in
//      main.cpp. App.qml wires the post-login MockDataStore into every
//      non-auth service so they share the same in-memory catalog/user state.
//
//  Theme:
//      Theme.mode flips between "light" and "dark". The toggle is exposed on
//      the dashboard topbar; persisted settings can be added later via
//      ThemeManager (the existing stub).
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import BookClub.ViewModels 1.0
import BookClub.Services 1.0
import "./theme"
import "./auth"
import "./user"
import "./publisher"
import "./admin"
import "./server"
import "./components/feedback"

ApplicationWindow {
    id: _app
    visible: true
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    title: "BookClub"
    color: Theme.color.pageBackground

    // ----- View Models (auth) -----
    LoginViewModel           { id: _loginVM;           authService: AuthService }
    RegisterViewModel        { id: _registerVM;        authService: AuthService }
    ForgotPasswordViewModel  { id: _forgotPasswordVM;  authService: AuthService }
    ResetPasswordViewModel   { id: _resetPasswordVM;   authService: AuthService }
    GenreSelectionViewModel  { id: _genreSelectionVM;  authService: AuthService }

    // ----- Shared mock data store -----
    // One instance, owned by App.qml, handed to every User service so the cart
    // → library → notifications flow stays consistent.
    MockDataStore { id: _dataStore }

    // ----- Wire services to the shared store (post-login data backbone) -----
    Component.onCompleted: {
        BookService.setDataStore(_dataStore)
        UserService.setDataStore(_dataStore)
        CartService.setDataStore(_dataStore)
        LibraryService.setDataStore(_dataStore)
        NotificationService.setDataStore(_dataStore)
        ReaderService.setDataStore(_dataStore)
        PublisherService.setDataStore(_dataStore)
        AdminService.setDataStore(_dataStore)
        ServerService.setDataStore(_dataStore)

        console.info("BookClub client ready. Demo accounts:")
        console.info("  username: alice   password: password123")
        console.info("  username: bob     password: password123")
    }

    // ----- Root StackView (auth ↔ dashboard) -----
    StackView {
        id: _router
        anchors.fill: parent
        initialItem: _splashPage

        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: _app.width * 0.04; to: 0; duration: Theme.motion.durationPage; easing.type: Easing.OutExpo }
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
            }
        }
        pushExit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: 0; to: -_app.width * 0.04; duration: Theme.motion.durationPage; easing.type: Easing.OutExpo }
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
            }
        }
        popEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: -_app.width * 0.04; to: 0; duration: Theme.motion.durationPage; easing.type: Easing.OutExpo }
                NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
            }
        }
        popExit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: 0; to: _app.width * 0.04; duration: Theme.motion.durationPage; easing.type: Easing.OutExpo }
                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
            }
        }
        replaceEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
        }
        replaceExit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.motion.durationPage; easing.type: Easing.OutCubic }
        }
    }

    // ----- Toast host (always on top) -----
    ToastManager {
        id: _toasts
        z: Theme.z.toast
    }

    function toast(variant, title, description, duration) {
        _toasts.show(variant, title, description, "", duration || 4000)
    }

    // ========================================================================
    //  Phase 1 — Auth flow
    // ========================================================================

    Component {
        id: _splashPage
        SplashPage {
            onFinished: _router.replace(_welcomePage)
        }
    }

    Component {
        id: _welcomePage
        WelcomePage {
            onLoginRequested:    _router.push(_loginPage)
            onRegisterRequested: _router.push(_registerPage)
        }
    }

    Component {
        id: _loginPage
        LoginPage {
            viewModel: _loginVM
            onLoginSuccess: {
                // Wire the MockDataStore to the freshly authenticated user.
                // Pull the real display name + role from the AuthService so
                // the home greeting and the role dispatcher both see the
                // post-login state.
                _dataStore.setCurrentUser(AuthService.currentUsername,
                                          AuthService.currentDisplayName)

                if (AuthService.requiresGenreSetup(AuthService.currentUsername)) {
                    _genreSelectionVM.username = AuthService.currentUsername
                    _router.push(_genreSelectionPage)
                } else {
                    _app._enterRoleShell()
                }
            }
            onRegisterRequested:       _router.replace(_registerPage)
            onForgotPasswordRequested: _router.push(_forgotPasswordPage)
        }
    }

    Component {
        id: _registerPage
        RegisterPage {
            viewModel: _registerVM
            onRegisterSuccess: {
                _genreSelectionVM.username = _registerVM.username
                _router.push(_genreSelectionPage)
            }
            onLoginRequested: _router.replace(_loginPage)
            onBackRequested:  _router.pop()
        }
    }

    Component {
        id: _forgotPasswordPage
        ForgotPasswordPage {
            viewModel: _forgotPasswordVM
            onResetPasswordRequested: {
                _resetPasswordVM.username   = username
                _resetPasswordVM.resetToken = resetToken
                _router.push(_resetPasswordPage)
            }
            onBackRequested: _router.pop()
        }
    }

    Component {
        id: _resetPasswordPage
        ResetPasswordPage {
            viewModel: _resetPasswordVM
            onResetSuccess: {
                _app.toast("success", "Password reset", "You can now log in with your new password.")
                _router.clear(StackView.Immediate)
                _router.push(_loginPage)
            }
            onBackRequested: _router.pop()
        }
    }

    Component {
        id: _genreSelectionPage
        GenreSelectionPage {
            viewModel: _genreSelectionVM
            onCompleted: {
                // Sync the selected genres into the MockDataStore so the
                // dashboard's "Recommended for you" reflects them.
                _dataStore.setFavoriteGenres(_genreSelectionVM.selectedGenres)
                _app.toast("success", "All set!", "Your reading preferences have been saved.")
                _app._enterRoleShell()
            }
            onBackRequested: _router.pop()
        }
    }

    // ========================================================================
    //  Phase 2 — Role-based dashboards
    //
    //  The role dispatcher reads AuthService.currentRole (set on successful
    //  login) and pushes the matching shell onto the router:
    //      "user"      → UserShell      (reader experience)
    //      "publisher" → PublisherShell (catalog + sales + promotions)
    //      "admin"     → AdminShell     (users + moderation + reports)
    //      "server"    → ServerShell    (clients + sessions + logs)
    // ========================================================================

    function _enterRoleShell() {
        const role = AuthService.currentRole
        let shell = _userShell
        let title = "Welcome back"
        let desc  = ""
        switch (role) {
            case "publisher": shell = _publisherShell; title = "Publisher signed in"; desc = "Catalog and analytics are ready."; break
            case "admin":     shell = _adminShell;     title = "Admin signed in";     desc = "Moderation tools are ready.";    break
            case "server":    shell = _serverShell;    title = "Operator signed in";  desc = "Server dashboard is live.";     break
            default:          shell = _userShell;      title = "Welcome back";        desc = "Your library is ready.";        break
        }
        _router.clear(StackView.Immediate)
        _router.replace(shell)
        if (desc.length > 0) _app.toast("success", title, desc)
    }

    function _performLogout() {
        // Tear down whatever role shell is active and return to the welcome
        // screen. AuthService.logout() clears the session state so the next
        // login can dispatch to a different role.
        AuthService.logout()
        _router.clear(StackView.Immediate)
        _router.push(_welcomePage)
        _app.toast("info", "Signed out", "See you soon!")
    }

    Component {
        id: _userShell
        UserShell {
            onLogoutRequested: _app._performLogout()
            onThemeToggled: {
                Theme.mode = Theme.isDark ? "light" : "dark"
            }
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }

    Component {
        id: _publisherShell
        PublisherShell {
            onLogoutRequested: _app._performLogout()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }

    Component {
        id: _adminShell
        AdminShell {
            onLogoutRequested: _app._performLogout()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }

    Component {
        id: _serverShell
        ServerShell {
            onLogoutRequested: _app._performLogout()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }
}
