// =============================================================================
//  LoginPage.qml
// =============================================================================
//  Primary authentication screen — username + password.
//
//  Mirrors the reference design language:
//      • Split-screen card with hero panel on the left
//      • Single-column form on the right (label → input → label → input)
//      • Primary button full-width
//      • Text links for "Forgot password?" and "Create account"
//      • Loading / error / validation states wired to LoginViewModel
//
//  MVVM bindings:
//      • username ↔ viewModel.username
//      • password ↔ viewModel.password
//      • rememberMe ↔ viewModel.rememberMe
//      • isBusy ↔ viewModel.isSubmitting
//      • errorText ← viewModel.formError
//      • usernameError ← viewModel.usernameError
//      • passwordError ← viewModel.passwordError
//      • submit() → viewModel.submit() (async)
//      • on loginSuccess → parent router goes to GenreSelection or dashboard
//
//  No social login, no email sign-in — only the project's defined auth method
//  (username + password), per the project requirements.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../layouts"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/progress"
import "../components/feedback"
import "../components/surfaces"

Item {
    id: root

    // ----- Public API -----
    property var viewModel: null   // LoginViewModel instance
    property bool isBusy: viewModel ? viewModel.isSubmitting : false

    signal loginSuccess()
    signal backRequested()
    signal registerRequested()
    signal forgotPasswordRequested()

    AuthLayout {
        id: _layout
        anchors.fill: parent
        heroTitle: "Welcome Back"
        heroSubtitle: "Sign in to continue to your reading journey."
        heroBadgeLabel: "Secure & Private"
        heroBadgeText: "Your credentials are encrypted and never shared."

        // ----- Form content -----
        Column {
            width: parent.width
            spacing: Theme.space.xl

            // Title block
            Column {
                width: parent.width
                spacing: Theme.space.xs

                Text {
                    text: "Login"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeDisplay
                    font.weight: Theme.font.weightSemibold
                }

                Text {
                    text: "Please sign in to your account"
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightRegular
                }
            }

            // ----- Form-level error banner -----
            ValidationMessage {
                type: "error"
                text: root.viewModel && root.viewModel.formError.length > 0 ? root.viewModel.formError : ""
                width: parent.width
                visible: root.viewModel && root.viewModel.formError.length > 0

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -Theme.space.sm
                    radius: Theme.radius.md
                    color: Theme.color.errorSoft
                    z: -1
                    visible: parent.visible
                }
            }

            // ----- Username field -----
            InputField {
                id: _username
                width: parent.width
                label: "Username"
                placeholder: "Enter your username"
                leadingIcon: "person"
                required: true
                text: root.viewModel ? root.viewModel.username : ""
                errorText: root.viewModel ? root.viewModel.usernameError : ""
                maximumLength: 20
                onTextEdited: {
                    if (root.viewModel) {
                        root.viewModel.username = newText
                        root.viewModel.validateUsername()
                    }
                }
                onAccepted: _password.forceActiveFocus()
                Component.onCompleted: {
                    if (root.viewModel && root.viewModel.username.length === 0) {
                        forceActiveFocus()
                    }
                }
            }

            // ----- Password field -----
            PasswordField {
                id: _password
                width: parent.width
                label: "Password"
                placeholder: "Enter your password"
                leadingIcon: "lock"
                required: true
                text: root.viewModel ? root.viewModel.password : ""
                errorText: root.viewModel ? root.viewModel.passwordError : ""
                onTextEdited: {
                    if (root.viewModel) {
                        root.viewModel.password = newText
                        root.viewModel.validatePassword()
                    }
                }
                onAccepted: _loginBtn.clicked()
            }

            // ----- Remember me + Forgot password -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                AppCheckbox {
                    checked: root.viewModel ? root.viewModel.rememberMe : false
                    label: "Remember me"
                    onToggled: {
                        if (root.viewModel) root.viewModel.rememberMe = checked
                    }
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                TextButton {
                    text: "Forgot password?"
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.forgotPasswordRequested()
                }
            }

            // ----- Primary submit -----
            PrimaryButton {
                id: _loginBtn
                width: parent.width
                text: "Login"
                iconName: "arrow_forward"
                iconPosition: "trailing"
                loading: root.isBusy
                enabled: !root.isBusy && (root.viewModel ? root.viewModel.canSubmit : false)
                onClicked: {
                    if (root.viewModel) {
                        root.viewModel.submit()
                    }
                }
            }

            // ----- Footer: Create account link -----
            Row {
                width: parent.width
                spacing: Theme.space.xs
                layoutDirection: Qt.LeftToRight
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text: "Don't have an account?"
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    anchors.verticalCenter: parent.verticalCenter
                }

                TextButton {
                    text: "Create account"
                    onClicked: root.registerRequested()
                }
            }
        }
    }

    // ----- Watch for login success -----
    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onLoginSucceeded: {
            root.loginSuccess()
        }
    }
}
