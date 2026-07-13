// =============================================================================
//  ResetPasswordPage.qml
// =============================================================================
//  Final step of the recovery flow — user enters a new password + confirm.
//  Requires a valid reset token from ForgotPasswordViewModel.
//
//  Validation (driven by ResetPasswordViewModel):
//      • password:    ≥6 chars (≥8 strong recommended); strength meter shown
//      • confirm:     must equal password
//  On success → emits resetSucceeded(), router navigates to LoginPage with a
//  "password changed" toast.
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

    property var viewModel: null   // ResetPasswordViewModel
    property bool isBusy: viewModel ? viewModel.isSubmitting : false

    signal backRequested()
    signal resetSuccess()

    AuthLayout {
        id: _layout
        anchors.fill: parent
        heroTitle: "Reset Password"
        heroSubtitle: "Choose a new password for your account."
        heroBadgeLabel: "Almost there"
        heroBadgeText: "After resetting, you'll be redirected to login."

        // ----- Back -----
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
                text: "Set new password"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightSemibold
            }

            Text {
                text: "Enter and confirm your new password below."
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
            }
        }

        // ----- Form-level error -----
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

        // ----- Requirements card -----
        Rectangle {
            width: parent.width
            height: _reqCol.implicitHeight + 2 * Theme.space.md
            radius: Theme.radius.md
            color: Theme.color.infoSoft
            border.color: "transparent"

            Column {
                id: _reqCol
                anchors.fill: parent
                anchors.margins: Theme.space.md
                spacing: Theme.space.xs

                Row {
                    spacing: Theme.space.xs
                    AppIcon { name: "info"; size: Theme.size.iconSm; color: Theme.color.info; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "Password requirements"
                        color: Theme.color.info
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightSemibold
                    }
                }

                Repeater {
                    model: [
                        { label: "At least 6 characters",     key: "minLength" },
                        { label: "Upper and lowercase letters", key: "caseMix" },
                        { label: "At least one digit",         key: "digit" },
                        { label: "At least one special character", key: "special" }
                    ]
                    delegate: Row {
                        spacing: Theme.space.xs
                        property bool met: root.viewModel ? root.viewModel.requirementsStatus[modelData.key] : false
                        AppIcon {
                            name: met ? "check_circle" : "radio_button_unchecked"
                            size: Theme.size.iconSm
                            color: met ? Theme.color.success : Theme.color.textMuted
                            anchors.verticalCenter: parent.verticalCenter
                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                        }
                        Text {
                            text: modelData.label
                            color: met ? Theme.color.success : Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: met ? Theme.font.weightMedium : Theme.font.weightRegular
                            anchors.verticalCenter: parent.verticalCenter
                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                        }
                    }
                }
            }
        }

        // ----- New password -----
        PasswordField {
            id: _newPass
            width: parent.width
            label: "New password"
            placeholder: "Enter your new password"
            leadingIcon: "lock"
            required: true
            text: root.viewModel ? root.viewModel.password : ""
            errorText: root.viewModel ? root.viewModel.passwordError : ""
            showStrengthMeter: true
            strengthScore: root.viewModel ? root.viewModel.passwordStrength : 0
            strengthLabel: root.viewModel ? root.viewModel.strengthLabel : ""
            onTextEdited: {
                if (root.viewModel) {
                    root.viewModel.password = newText
                    root.viewModel.validatePassword()
                }
            }
            onAccepted: _confirmPass.forceActiveFocus()
            Component.onCompleted: forceActiveFocus()
        }

        // ----- Confirm password -----
        PasswordField {
            id: _confirmPass
            width: parent.width
            label: "Confirm new password"
            placeholder: "Re-enter your new password"
            leadingIcon: "lock"
            required: true
            text: root.viewModel ? root.viewModel.confirmPassword : ""
            errorText: root.viewModel ? root.viewModel.confirmPasswordError : ""
            successText: root.viewModel && root.viewModel.confirmPassword.length > 0
                         && root.viewModel.confirmPassword === root.viewModel.password
                         ? "Passwords match" : ""
            onTextEdited: {
                if (root.viewModel) {
                    root.viewModel.confirmPassword = newText
                    root.viewModel.validateConfirmPassword()
                }
            }
            onAccepted: _resetBtn.clicked()
        }

        // ----- Submit -----
        PrimaryButton {
            id: _resetBtn
            width: parent.width
            text: "Reset password"
            iconName: "key"
            iconPosition: "leading"
            loading: root.isBusy
            enabled: !root.isBusy && (root.viewModel ? root.viewModel.canSubmit : false)
            onClicked: if (root.viewModel) root.viewModel.submit()
        }
    }

    LoadingOverlay {
        active: root.isBusy
        label: "Resetting your password…"
    }

    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onResetSucceeded: root.resetSuccess()
    }
}
