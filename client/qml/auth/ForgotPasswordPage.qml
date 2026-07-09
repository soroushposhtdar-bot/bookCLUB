// =============================================================================
//  ForgotPasswordPage.qml
// =============================================================================
//  Two-step password recovery flow:
//    Step 1 — Username entry → ViewModel fetches the user's security question
//             (mocked). Validates username format and existence.
//    Step 2 — Security answer entry → ViewModel verifies the answer against
//             the stored hash (mocked). On success, navigates to ResetPassword.
//
//  States handled by UI:
//      • initial           — username field only
//      • loading           — fetching question (spinner overlay)
//      • questionLoaded    — show question + answer field
//      • error             — username not found / answer wrong (inline banner)
//      • success           — proceed to reset password page
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../layouts"
import "../components/buttons"
import "../components/inputs"
import "../components/surfaces"
import "../components/feedback"
import "../components/progress"

Item {
    id: root

    property var viewModel: null   // ForgotPasswordViewModel
    property bool isBusy: viewModel ? viewModel.isSubmitting : false

    signal backRequested()
    signal resetPasswordRequested(string username, string resetToken)

    AuthLayout {
        id: _layout
        anchors.fill: parent
        heroTitle: "Forgot Password"
        heroSubtitle: "Reset your password using your security question."
        heroBadgeLabel: "Account recovery"
        heroBadgeText: "Verify your identity to set a new password."

        // ----- Back button -----
        Row {
            width: parent.width
            IconButton {
                iconName: "arrow_back"
                iconColor: Theme.color.textSecondary
                onClicked: root.backRequested()
            }
            Item { width: 1; height: 1; Layout.fillWidth: true }
        }

        // ----- Title -----
        Column {
            width: parent.width
            spacing: Theme.space.xs

            Text {
                text: root.viewModel && root.viewModel.step === "answer" ? "Security question" : "Recover account"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightSemibold
            }

            Text {
                text: root.viewModel && root.viewModel.step === "answer"
                      ? "Answer the question to verify your identity."
                      : "Enter your username to begin password recovery."
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
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

        // ----- Step indicator -----
        Row {
            width: parent.width
            spacing: Theme.space.sm

            // Step 1
            Rectangle {
                width: (parent.width - Theme.space.sm) / 2
                height: 4
                radius: 2
                color: Theme.color.primary
            }
            // Step 2
            Rectangle {
                width: (parent.width - Theme.space.sm) / 2
                height: 4
                radius: 2
                color: root.viewModel && root.viewModel.step === "answer" ? Theme.color.primary : Theme.color.divider
                Behavior on color { ColorAnimation { duration: Theme.motion.durationBase } }
            }
        }

        // ----- Step 1: Username -----
        InputField {
            id: _username
            width: parent.width
            label: "Username"
            placeholder: "Enter your username"
            leadingIcon: "person"
            required: true
            text: root.viewModel ? root.viewModel.username : ""
            errorText: root.viewModel ? root.viewModel.usernameError : ""
            visible: !root.viewModel || root.viewModel.step === "username"
            maximumLength: 20
            onTextEdited: {
                if (root.viewModel) {
                    root.viewModel.username = newText
                    root.viewModel.validateUsername()
                }
            }
            onAccepted: _continueBtn.clicked()
            Component.onCompleted: if (root.viewModel && root.viewModel.step === "username") forceActiveFocus()
        }

        // ----- Step 2: Security question + answer -----
        Column {
            width: parent.width
            spacing: Theme.space.lg
            visible: root.viewModel && root.viewModel.step === "answer"

            // Security question display (read-only card)
            Rectangle {
                width: parent.width
                height: _qColumn.implicitHeight + 2 * Theme.space.md
                radius: Theme.radius.md
                color: Theme.color.fieldFilled
                border.color: Theme.color.border
                border.width: 1

                Column {
                    id: _qColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space.md
                    spacing: Theme.space.xs

                    Row {
                        spacing: Theme.space.xs

                        AppIcon {
                            name: "quiz"
                            size: Theme.size.iconSm
                            color: Theme.color.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: "Security question"
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                    }

                    Text {
                        text: root.viewModel ? root.viewModel.securityQuestion : ""
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBodyLarge
                        font.weight: Theme.font.weightMedium
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            InputField {
                id: _answer
                width: parent.width
                label: "Your answer"
                placeholder: "Type your answer"
                leadingIcon: "verified_user"
                required: true
                text: root.viewModel ? root.viewModel.securityAnswer : ""
                errorText: root.viewModel ? root.viewModel.securityAnswerError : ""
                maximumLength: 100
                onTextEdited: {
                    if (root.viewModel) {
                        root.viewModel.securityAnswer = newText
                        root.viewModel.validateSecurityAnswer()
                    }
                }
                onAccepted: _continueBtn.clicked()
                Component.onCompleted: if (root.viewModel && root.viewModel.step === "answer") forceActiveFocus()
            }
        }

        // ----- Continue / Submit -----
        PrimaryButton {
            id: _continueBtn
            width: parent.width
            text: root.viewModel && root.viewModel.step === "answer" ? "Verify answer" : "Continue"
            iconName: "arrow_forward"
            iconPosition: "trailing"
            loading: root.isBusy
            enabled: !root.isBusy && (root.viewModel ? root.viewModel.canSubmit : false)
            onClicked: if (root.viewModel) root.viewModel.submit()
        }

        // ----- Resend / Retry helper -----
        TextButton {
            text: "Back to login"
            iconName: "arrow_back"
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: root.backRequested()
        }
    }

    // Loading overlay on the form panel
    LoadingOverlay {
        active: root.isBusy
        label: root.viewModel && root.viewModel.step === "username" ? "Looking up your account…" : "Verifying your answer…"
    }

    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onRecoverySucceeded: {
            root.resetPasswordRequested(root.viewModel.username, root.viewModel.resetToken)
        }
    }
}
