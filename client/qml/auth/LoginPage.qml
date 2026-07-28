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

    // Issue 5: load saved username/password from QSettings so the
    // "Remember me" checkbox actually fills the fields on next launch.
    Component.onCompleted: {
        if (root.viewModel) root.viewModel.loadSavedCredentials()
    }

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
                onTextEdited: function(newText) {
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
                onTextEdited: function(newText) {
                    if (root.viewModel) {
                        root.viewModel.password = newText
                        root.viewModel.validatePassword()
                    }
                }
                // BUG FIX (Issue 20): `clicked()` is a signal emitter that
                // fires `onClicked` regardless of `enabled`. Guard with
                // `enabled` so Enter doesn't submit when the button is
                // disabled (e.g. during a busy state or invalid form).
                onAccepted: if (_loginBtn.enabled) _loginBtn.clicked()
            }

            // ----- Remember me + Forgot password -----
            // BUG FIX (Issue 1): the previous Row used
            // `Item { Layout.fillWidth: true }` as a spacer, but Layout
            // attached properties only function inside RowLayout/ColumnLayout.
            // Inside a plain Row, the spacer stayed 1px wide and the
            // "Forgot password?" link was never pushed to the right edge.
            // Switched to RowLayout so the spacer actually fills.
            RowLayout {
                width: parent.width
                spacing: Theme.space.md

                AppCheckbox {
                    checked: root.viewModel ? root.viewModel.rememberMe : false
                    label: "Remember me"
                    onToggled: function(checked) {
                        if (root.viewModel) root.viewModel.rememberMe = checked
                    }
                }

                Item { Layout.fillWidth: true }

                TextButton {
                    text: "Forgot password?"
                    Layout.alignment: Qt.AlignVCenter
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
            // BUG FIX (Issue 26): removed redundant `anchors.horizontalCenter`
            // (no-op when `width: parent.width` is set).
            Row {
                width: parent.width
                spacing: Theme.space.xs
                layoutDirection: Qt.LeftToRight

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
