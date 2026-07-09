// =============================================================================
//  App.qml
// =============================================================================
//  Root application window. Owns:
//      • A StackView-based router for the auth flow
//      • All auth view models (instantiated here, injected into pages)
//      • A single shared AuthService (mocked)
//      • A single ToastManager overlay
//
//  Auth flow:
//      SplashPage → WelcomePage
//                 → LoginPage       → (success) → GenreSelectionPage (if needed) → done
//                                  → ForgotPasswordPage → ResetPasswordPage → LoginPage
//                 → RegisterPage    → (success) → GenreSelectionPage → done
//
//  All page transitions use a subtle horizontal slide + fade.
//
//  ViewModels and AuthService are registered in main.cpp via
//  qmlRegisterType / qmlRegisterSingletonType and imported via the
//  `BookClub.ViewModels 1.0` and `BookClub.Services 1.0` URIs.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import BookClub.ViewModels 1.0
import BookClub.Services 1.0
import "./theme"
import "./auth"
import "./components/feedback"

ApplicationWindow {
    id: _app
    visible: true
    width: 1180
    height: 760
    minimumWidth: 960
    minimumHeight: 640
    title: "BookClub"
    color: Theme.color.pageBackground

    // ----- View Models -----
    // Each VM has authService injected via the `authService` property,
    // which is wired through the AuthService singleton instance.
    LoginViewModel           { id: _loginVM;           authService: AuthService }
    RegisterViewModel        { id: _registerVM;        authService: AuthService }
    ForgotPasswordViewModel  { id: _forgotPasswordVM;  authService: AuthService }
    ResetPasswordViewModel   { id: _resetPasswordVM;   authService: AuthService }
    GenreSelectionViewModel  { id: _genreSelectionVM;  authService: AuthService }

    // ----- Router (StackView) -----
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
    }

    // ----- Toast host (always on top) -----
    ToastManager {
        id: _toasts
        z: Theme.z.toast
    }

    // Helper: show a toast from anywhere
    function toast(variant, title, description, duration) {
        _toasts.show(variant, title, description, "", duration || 4000)
    }

    // ========================================================================
    //  Page definitions
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
                if (AuthService.requiresGenreSetup(_loginVM.username)) {
                    _genreSelectionVM.username = _loginVM.username
                    _router.push(_genreSelectionPage)
                } else {
                    _app.toast("success", "Welcome back!", "You're now signed in.")
                    // TODO: navigate to main dashboard once implemented
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
                // Pop the entire stack back to nothing, then show login as the fresh root.
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
                _app.toast("success", "All set!", "Your reading preferences have been saved.")
                // TODO: navigate to main dashboard once implemented
            }
            onBackRequested: _router.pop()
        }
    }

    // ----- Demo accounts hint -----
    Component.onCompleted: {
        console.info("BookClub client ready. Demo accounts:")
        console.info("  username: alice   password: password123")
        console.info("  username: bob     password: password123")
    }
}
