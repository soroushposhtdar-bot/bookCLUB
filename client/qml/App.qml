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
//      main.cpp. App.qml registers every service as a QML singleton so
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

    // =========================================================================
    //  Issue 9 + Issue 10 — Exit confirmation dialog + working Exit button.
    //
    //  We intercept the platform close event (the X button, Alt+F4, Cmd+Q on
    //  macOS) via `onClosing`. Instead of letting Qt close the window
    //  immediately, we show a ConfirmDialog and only close when the user
    //  confirms. Escape cancels; Enter confirms.
    //
    //  Issue 10 root cause: `Qt.quit()` triggers another `onClosing` event
    //  on the ApplicationWindow, which reopens the dialog → infinite loop.
    //  Fix: use a `_exiting` guard flag that is set to true right before
    //  calling `Qt.quit()`. When `onClosing` sees `_exiting == true`, it
    //  accepts the close immediately without showing the dialog.
    // =========================================================================
    property bool _exiting: false

    onClosing: function(close) {
        if (_app._exiting) {
            // We're already in the process of quitting — let it through.
            close.accepted = true
            return
        }
        close.accepted = false
        _exitDialog.open()
    }

    // The dialog itself. Defined here so it has access to the application
    // window's overlay. Uses the existing ConfirmDialog component so the
    // visual language matches the rest of the app.
    ConfirmDialog {
        id: _exitDialog
        title: "Exit BookClub?"
        message: "Are you sure you want to quit? Any in-progress reads will be saved and you can pick up where you left off next time."
        detail: "Press Enter to exit, Esc to stay."
        iconName: "logout"
        confirmLabel: "Exit"
        cancelLabel: "Cancel"
        confirmStyle: "danger"
        onConfirmed: {
            // Issue 10 — set the guard flag BEFORE calling Qt.quit() so the
            // resulting onClosing event doesn't reopen the dialog.
            _app._exiting = true
            Qt.quit()
        }
        // Escape is handled by the Popup's closePolicy (CloseOnEscape).
        // Enter / Return is handled by the Shortcut below (active only
        // while the dialog is open).
    }

    // v11: Sign-out confirmation dialog — shown when any role clicks "Sign out"
    ConfirmDialog {
        id: _signOutDialog
        title: "Sign out?"
        message: "Are you sure you want to sign out? You'll need to log in again to access your account."
        detail: "Press Enter to sign out, Esc to stay."
        iconName: "logout"
        confirmLabel: "Sign out"
        cancelLabel: "Cancel"
        confirmStyle: "danger"
        onConfirmed: _app._performLogout()
    }

    // Global keyboard shortcut for Enter / Return → confirm exit.
    // Only active while the exit dialog is visible, so it doesn't
    // interfere with normal typing on auth pages and input fields.
    // BUG FIX: previously this Shortcut was always active and consumed
    // every Enter/Return keypress before any TextField could fire its
    // `onAccepted` handler — so pressing Enter in any auth form did
    // nothing (the form's submit-on-Enter path was dead).
    // Also removed the double-fire: previously the Shortcut called
    // `_exitDialog.confirmed()` (which itself calls Qt.quit()) AND then
    // called `Qt.quit()` again. Now just sets the guard flag + quits.
    Shortcut {
        sequences: ["Enter", "Return"]
        enabled: _exitDialog.visible
        onActivated: {
            _app._exiting = true
            Qt.quit()
        }
    }

    // ----- View Models (auth) -----
    LoginViewModel           { id: _loginVM;           authService: AuthService }
    RegisterViewModel        { id: _registerVM;        authService: AuthService }
    ForgotPasswordViewModel  { id: _forgotPasswordVM;  authService: AuthService }
    ResetPasswordViewModel   { id: _resetPasswordVM;   authService: AuthService }
    GenreSelectionViewModel  { id: _genreSelectionVM;  authService: AuthService }

    // ----- Services are now backed by the real server via NetworkService -----
    // Each service talks to the backend directly via NetworkService.
    Component.onCompleted: {
        console.info("BookClub client ready.")
    }

    // v12: Real-time notification toast — when the server pushes an
    // EvtNotification, show a toast so the user sees it immediately
    // (not just the badge count updating).
    Connections {
        target: NotificationService
        ignoreUnknownSignals: true
        function onNotificationReceived(dto) {
            if (dto) {
                _app.toast("info",
                           dto.title || "New notification",
                           dto.message || dto.body || "")
            }
        }
    }

    // v21: Handle EvtUserBlocked — when an admin blocks a user, the
    // server pushes this event to the blocked user's client. We
    // immediately log them out and show a "blocked" message.
    Connections {
        target: AuthService
        ignoreUnknownSignals: true
        function onUserBlocked(reason) {
            _app.toast("error", "Account blocked", reason || "Your account has been blocked by an administrator.")
            _app._performLogout()
        }
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

    // ----- Global auth connection-failure handler (Issue 2) -----
    // AuthService emits `connectionFailed` whenever any auth operation
    // (login / register / reset-password) cannot reach the server. We
    // show a modern toast in the top-right corner with a close button
    // and auto-dismiss. The toast is non-blocking — the underlying
    // form-error banner still shows on the auth page for context, but
    // the UI never freezes because the QEventLoop inside sendRequest
    // times out quickly via the connectToServer(500ms) short-wait.
    Connections {
        target: AuthService
        ignoreUnknownSignals: true
        function onConnectionFailed(reason) {
            _app.toast("error", "Cannot connect to server", reason, 6000)
        }
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
            // BUG FIX: loadSavedCredentials() was being called on every LoginPage
            // creation, overwriting router-pre-filled values (e.g. the
            // post-registration username hand-off). The per-instance
            // `_credentialsLoaded` guard didn't help because each
            // StackView.push creates a fresh LoginPage with the flag reset.
            // We now only load saved credentials when the VM is empty (no
            // router pre-fill happened). This preserves both the "Remember
            // me" auto-fill on first launch AND the post-registration /
            // post-genre-selection username hand-off.
            Component.onCompleted: {
                if (_loginVM.username.length === 0 && _loginVM.password.length === 0) {
                    _loginVM.loadSavedCredentials()
                }
            }
            onLoginSuccess: {
                // The AuthService already holds the session state
                // (currentUsername / currentDisplayName / currentRole).
                // Services talk to the
                // server directly via NetworkService.

                if (AuthService.requiresGenreSetup(AuthService.currentUsername)) {
                    _genreSelectionVM.username = AuthService.currentUsername
                    _router.push(_genreSelectionPage)
                } else {
                    _app._enterRoleShell()
                }
            }
            onRegisterRequested: {
                // Reset the Register VM before navigating so the user
                // doesn't see state from a previous registration attempt.
                _registerVM.reset()
                _router.replace(_registerPage)
            }
            onForgotPasswordRequested: {
                _forgotPasswordVM.reset()
                _router.push(_forgotPasswordPage)
            }
        }
    }

    Component {
        id: _registerPage
        RegisterPage {
            viewModel: _registerVM
            onRegisterSuccess: {
                // BUG FIX: after a successful registration, navigate directly
                // to the Genre Selection page (per the project spec: "On the
                // first login, the user must pick 1-3 favourite genres"). The
                // previous flow sent the user to the Login page first, which
                // meant the user had to log in before seeing the genre
                // selection — and if the server didn't return
                // requiresGenreSetup=true on login, the genre page never
                // appeared at all.
                //
                // We now go directly to GenreSelection, pre-fill the username
                // from the registration form, and let the user pick their
                // genres. After they complete (or skip) the genre selection,
                // we route them to the Login page to sign in with their new
                // credentials.
                _genreSelectionVM.reset()
                _genreSelectionVM.username = _registerVM.username
                _app.toast("success", "Account created", "Pick your favourite genres to get started.")
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
            // BUG FIX: Qt 6 requires `function(params)` syntax for
            // parameterized signal handlers. The implicit form
            // `onResetPasswordRequested: { /* use username, resetToken */ }`
            // generates QML compiler warnings and will be removed in a
            // future Qt version.
            onResetPasswordRequested: function(username, resetToken) {
                // BUG FIX: reset the ResetPasswordVM before assigning new
                // username/token so stale password/confirm from a previous
                // attempt don't bleed into the new attempt.
                _resetPasswordVM.reset()
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
                // BUG FIX (Issue 13): after a successful password reset,
                // any "Remember me" credentials stored in QSettings are
                // now stale (they hold the OLD password). Clear them so
                // the next LoginPage doesn't pre-fill an invalid password.
                _loginVM.clearSavedCredentials()
                // BUG FIX: also clear the in-memory password so the next
                // LoginPage doesn't show the old (now-invalid) password
                // the user typed before clicking "Forgot password?".
                _loginVM.password = ""
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
                // BUG FIX: the VM calls AuthService::saveGenreSelection inside
                // _doSubmit() before emitting `completed()`. After the genre
                // selection is done, we need to route the user based on
                // whether they're logged in:
                //   - If logged in (came from Login → GenreSelection), enter
                //     the role shell.
                //   - If NOT logged in (came from Register → GenreSelection),
                //     route to the Login page so they can sign in with their
                //     new credentials. Pre-fill the username from the
                //     registration form.
                _app.toast("success", "All set!", "Your reading preferences have been saved.")
                if (AuthService.isLoggedIn) {
                    _app._enterRoleShell()
                } else {
                    _loginVM.username = _genreSelectionVM.username
                    _loginVM.password = ""
                    _router.clear(StackView.Immediate)
                    _router.push(_loginPage)
                }
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
        // BUG FIX: clear the LoginVM's in-memory password so the next
        // LoginPage doesn't show the previous account's password (privacy/
        // security regression). The username is kept so "Remember me"
        // can still pre-fill it on the next login attempt.
        _loginVM.password = ""
        _router.clear(StackView.Immediate)
        _router.push(_welcomePage)
        _app.toast("info", "Signed out", "See you soon!")
    }

    Component {
        id: _userShell
        UserShell {
            onLogoutRequested: _signOutDialog.open()
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
            onLogoutRequested: _signOutDialog.open()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }

    Component {
        id: _adminShell
        AdminShell {
            onLogoutRequested: _signOutDialog.open()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }

    Component {
        id: _serverShell
        ServerShell {
            onLogoutRequested: _signOutDialog.open()
            onThemeToggled: Theme.mode = Theme.isDark ? "light" : "dark"
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
        }
    }
}
