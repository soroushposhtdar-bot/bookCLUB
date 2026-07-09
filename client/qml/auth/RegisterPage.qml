// =============================================================================
//  RegisterPage.qml
// =============================================================================
//  New account registration. Fields per the project's UserAccount model:
//      • username (3-20 alphanumeric + underscore)
//      • displayName (1-50 chars)
//      • password (≥6 chars; UI also shows strength meter)
//      • confirmPassword (must equal password)
//      • securityQuestion (dropdown of 5 predefined questions)
//      • securityAnswer (≥2 chars; stored hashed)
//      • acceptTerms (required)
//
//  Validation is bound to RegisterViewModel; each field shows inline error
//  text and the submit button is disabled until the whole form is valid.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../layouts"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/surfaces"
import "../components/feedback"
import "../components/progress"

Item {
    id: root

    property var viewModel: null   // RegisterViewModel
    property bool isBusy: viewModel ? viewModel.isSubmitting : false

    signal registerSuccess()
    signal backRequested()
    signal loginRequested()

    AuthLayout {
        id: _layout
        anchors.fill: parent
        heroTitle: "Join BookClub"
        heroSubtitle: "Create your account and start your reading journey today."
        heroBadgeLabel: "Your data stays yours"
        heroBadgeText: "We never share your information with anyone."

        // ----- Back button (top-left of form) -----
        Row {
            width: parent.width
            layoutDirection: Qt.LeftToRight

            IconButton {
                iconName: "arrow_back"
                iconColor: Theme.color.textSecondary
                onClicked: root.backRequested()
            }

            Item { width: 1; height: 1; Layout.fillWidth: true }
        }

        // ----- Title block -----
        Column {
            width: parent.width
            spacing: Theme.space.xs

            Text {
                text: "Create account"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightSemibold
            }

            Text {
                text: "Fill in your details to register"
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

        // ----- Username -----
        InputField {
            width: parent.width
            label: "Username"
            placeholder: "3-20 chars, letters and digits"
            leadingIcon: "person"
            required: true
            text: root.viewModel ? root.viewModel.username : ""
            errorText: root.viewModel ? root.viewModel.usernameError : ""
            helperText: root.viewModel && root.viewModel.usernameError.length === 0 && root.viewModel.username.length > 0
                        ? "Available" : ""
            successText: root.viewModel && root.viewModel.usernameAvailable ? "Username is available" : ""
            maximumLength: 20
            onTextEdited: {
                if (root.viewModel) {
                    root.viewModel.username = newText
                    root.viewModel.validateUsername()
                }
            }
            onAccepted: _displayName.forceActiveFocus()
        }

        // ----- Display name -----
        InputField {
            id: _displayName
            width: parent.width
            label: "Display name"
            placeholder: "How should we call you?"
            leadingIcon: "badge"
            required: true
            text: root.viewModel ? root.viewModel.displayName : ""
            errorText: root.viewModel ? root.viewModel.displayNameError : ""
            maximumLength: 50
            onTextEdited: {
                if (root.viewModel) {
                    root.viewModel.displayName = newText
                    root.viewModel.validateDisplayName()
                }
            }
            onAccepted: _password.forceActiveFocus()
        }

        // ----- Password (with strength meter) -----
        PasswordField {
            id: _password
            width: parent.width
            label: "Password"
            placeholder: "At least 6 characters"
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
            onAccepted: _confirmPassword.forceActiveFocus()
        }

        // ----- Confirm password -----
        PasswordField {
            id: _confirmPassword
            width: parent.width
            label: "Confirm password"
            placeholder: "Re-enter your password"
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
            onAccepted: _securityQuestionCombo.forceActiveFocus()
        }

        // ----- Security question -----
        Column {
            width: parent.width
            spacing: Theme.space.sm

            Row {
                spacing: 2
                Text {
                    text: "Security question"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightMedium
                }
                Text {
                    text: "*"
                    color: Theme.color.error
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightMedium
                }
            }

            // Custom combobox-styled field (renders the same as InputField)
            InputField {
                id: _securityQuestionCombo
                width: parent.width
                leadingIcon: "quiz"
                placeholder: "Choose a security question"
                text: root.viewModel ? root.viewModel.securityQuestion : ""
                showClearButton: false
                trailingIcon: "expand_more"
                readOnly: true
                onTrailingClicked: _securityQuestionsPopup.open()

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: _securityQuestionsPopup.open()
                }
            }

            // Popup with question list
            Popup {
                id: _securityQuestionsPopup
                width: _securityQuestionCombo.width
                y: _securityQuestionCombo.y + _securityQuestionCombo.height + Theme.space.xs
                padding: Theme.space.sm
                modal: false
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                background: Rectangle {
                    radius: Theme.radius.md
                    color: Theme.color.cardBackground
                    border.color: Theme.color.border
                    border.width: 1
                    layer.enabled: true
                    layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: root.viewModel ? root.viewModel.availableSecurityQuestions : []
                        delegate: ItemDelegate {
                            width: parent.width
                            height: 44
                            text: modelData
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.securityQuestion = modelData
                                    root.viewModel.validateSecurityQuestion()
                                }
                                _securityQuestionsPopup.close()
                            }

                            contentItem: Text {
                                text: modelData
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: Theme.space.md
                                rightPadding: Theme.space.md
                            }

                            background: Rectangle {
                                color: parent.hovered ? Theme.color.fieldFilled : "transparent"
                                radius: Theme.radius.sm
                            }
                        }
                    }
                }
            }
        }

        // ----- Security answer -----
        InputField {
            id: _securityAnswer
            width: parent.width
            label: "Security answer"
            placeholder: "Your answer (case-insensitive)"
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
            onAccepted: _registerBtn.clicked()
        }

        // ----- Terms checkbox -----
        AppCheckbox {
            checked: root.viewModel ? root.viewModel.acceptTerms : false
            label: "I agree to the Terms of Service and Privacy Policy"
            onToggled: {
                if (root.viewModel) {
                    root.viewModel.acceptTerms = checked
                    root.viewModel.validateAcceptTerms()
                }
            }
        }

        // ----- Submit -----
        PrimaryButton {
            id: _registerBtn
            width: parent.width
            text: "Create account"
            iconName: "how_to_reg"
            iconPosition: "leading"
            loading: root.isBusy
            enabled: !root.isBusy && (root.viewModel ? root.viewModel.canSubmit : false)
            onClicked: if (root.viewModel) root.viewModel.submit()
        }

        // ----- Footer -----
        Row {
            width: parent.width
            spacing: Theme.space.xs
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                text: "Already have an account?"
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                anchors.verticalCenter: parent.verticalCenter
            }

            TextButton {
                text: "Login"
                onClicked: root.loginRequested()
            }
        }
    }

    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onRegisterSucceeded: root.registerSuccess()
    }
}
