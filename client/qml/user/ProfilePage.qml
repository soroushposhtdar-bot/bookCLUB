// =============================================================================
//  ProfilePage.qml
// =============================================================================
//  Profile + settings page.
//
//  Sections:
//      • Identity card (avatar + display name + edit form)
//      • Favorite genres management
//      • Change password
//      • Purchase history
//      • Settings (theme toggle, sign out)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../layouts"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/navigation"
import "../components/data"
import "../components/feedback"
import "../components/progress"
import "../components/book"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // ProfileViewModel
    property var bookService: null  // for the available-genres catalog
    property bool darkMode: false

    signal logoutRequested()
    signal themeToggled()

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property bool _isBusy: root.viewModel && root.viewModel.isBusy

    // ----- Password strength evaluator -----
    //   Returns 0..4 based on length + character-class diversity.
    //   0 = empty/too short, 1 = weak, 2 = fair, 3 = good, 4 = strong.
    function _strengthScore(pw) {
        if (pw.length < 6) return 0
        let score = 0
        if (pw.length >= 6) ++score
        if (pw.length >= 10) ++score
        if (/[a-z]/.test(pw) && /[A-Z]/.test(pw)) ++score
        if (/[0-9]/.test(pw)) ++score
        if (/[^a-zA-Z0-9]/.test(pw)) ++score
        return Math.min(4, score)
    }
    function _strengthLabel(pw) {
        const s = root._strengthScore(pw)
        if (s === 0) return pw.length === 0 ? "" : "Too short"
        if (s === 1) return "Weak"
        if (s === 2) return "Fair"
        if (s === 3) return "Good"
        return "Strong"
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.xl

            Item { width: 1; height: Theme.space.sm }

            // ----- Identity card -----
            Card {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "sm"
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.lg

                    // Header
                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        Avatar {
                            size: 72
                            initials: root.viewModel ? root.viewModel.initials : "?"
                            online: true
                        }

                        Column {
                            spacing: Theme.space.xs
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: root.viewModel ? root.viewModel.displayName : ""
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeHeadline
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: "@" + (root.viewModel ? root.viewModel.username : "guest")
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Row {
                                spacing: Theme.space.sm
                                Rectangle {
                                    width: _gText.implicitWidth + 16
                                    height: 22
                                    radius: 11
                                    color: Theme.color.fieldFilled
                                    Text {
                                        id: _gText
                                        anchors.centerIn: parent
                                        text: root.viewModel ? root.viewModel.favoriteGenresSummary : ""
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeMicro2
                                        font.weight: Theme.font.weightMedium
                                    }
                                }
                            }
                        }

                        Item { width: 1; height: 1; Layout.fillWidth: true }

                        // Stats column
                        Column {
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: root.viewModel ? root.viewModel.purchaseCount : 0
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeDisplay
                                font.weight: Theme.font.weightBold
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: "Purchases"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    Divider {
                        width: parent.width
                        orientation: "horizontal"
                    }

                    // Edit display name form
                    Column {
                        width: parent.width
                        spacing: Theme.space.md

                        Text {
                            text: "Edit display name"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightSemibold
                        }

                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                            InputField {
                                width: parent.width - _saveProfileBtn.width - Theme.space.md
                                label: "Display name"
                                placeholder: "How should we call you?"
                                text: root.viewModel ? root.viewModel.displayName : ""
                                maximumLength: 50
                                onTextEdited: {
                                    if (root.viewModel) root.viewModel.displayName = newText
                                }
                            }

                            PrimaryButton {
                                id: _saveProfileBtn
                                text: "Save"
                                iconPosition: "leading"
                                enabled: !root._isBusy
                                loading: root._isBusy
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: {
                                    if (root.viewModel) root.viewModel.saveProfile()
                                }
                            }
                        }
                    }
                }
            }

            // ----- Favorite genres -----
            Card {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Row {
                        width: parent.width
                        Text {
                            text: "Favorite genres"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightSemibold
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item { width: 1; height: 1; Layout.fillWidth: true }
                        Text {
                            text: (root.viewModel ? root.viewModel.selectedGenreCount : 0) + " / 3 selected"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Text {
                        text: "We use these to personalize your home feed. Pick between 1 and 3."
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    Grid {
                        width: parent.width
                        columns: root.width < 760 ? 2 : 4
                        spacing: Theme.space.sm

                        Repeater {
                            model: root.bookService ? root.bookService.availableGenres() : []
                            delegate: GenreChip {
                                label: modelData
                                selected: root.viewModel && root.viewModel.isGenreSelected(modelData)
                                width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                                onClicked: {
                                    if (root.viewModel) root.viewModel.toggleGenre(modelData)
                                }
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        PrimaryButton {
                            text: "Save preferences"
                            iconName: "check"
                            iconPosition: "leading"
                            enabled: root.viewModel && root.viewModel.canSaveGenres && !root._isBusy
                            loading: root._isBusy
                            onClicked: {
                                if (root.viewModel) root.viewModel.saveGenres()
                            }
                        }

                        SecondaryButton {
                            text: "Reset to current"
                            onClicked: {
                                if (root.viewModel) root.viewModel.loadGenresFromUser()
                            }
                        }
                    }
                }
            }

            // ----- Change password -----
            Card {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text {
                        text: "Change password"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightSemibold
                    }

                    PasswordField {
                        width: parent.width
                        label: "Current password"
                        placeholder: "Enter your current password"
                        leadingIcon: "lock"
                        text: root.viewModel ? root.viewModel.currentPassword : ""
                        onTextEdited: {
                            if (root.viewModel) root.viewModel.currentPassword = newText
                        }
                    }

                    PasswordField {
                        width: parent.width
                        label: "New password"
                        placeholder: "At least 6 characters"
                        leadingIcon: "lock"
                        showStrengthMeter: true
                        // Real strength evaluator — computed from the current
                        // password text. Score is 0..4, label is Weak/Fair/Good/Strong.
                        strengthScore: root._strengthScore(root.viewModel ? root.viewModel.newPassword : "")
                        strengthLabel: root._strengthLabel(root.viewModel ? root.viewModel.newPassword : "")
                        text: root.viewModel ? root.viewModel.newPassword : ""
                        onTextEdited: {
                            if (root.viewModel) root.viewModel.newPassword = newText
                        }
                    }

                    PasswordField {
                        width: parent.width
                        label: "Confirm new password"
                        placeholder: "Re-enter your new password"
                        leadingIcon: "lock"
                        text: root.viewModel ? root.viewModel.confirmPassword : ""
                        errorText: root.viewModel ? root.viewModel.passwordError : ""
                        successText: root.viewModel && root.viewModel.canChangePassword ? "Passwords match" : ""
                        onTextEdited: {
                            if (root.viewModel) root.viewModel.confirmPassword = newText
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        PrimaryButton {
                            text: "Update password"
                            iconName: "lock"
                            iconPosition: "leading"
                            enabled: root.viewModel && root.viewModel.canChangePassword && !root._isBusy
                            loading: root._isBusy
                            onClicked: {
                                if (root.viewModel) root.viewModel.changePassword()
                            }
                        }

                        SecondaryButton {
                            text: "Clear"
                            onClicked: {
                                if (root.viewModel) root.viewModel.clearPasswordFields()
                            }
                        }
                    }
                }
            }

            // ----- Purchase history -----
            Card {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text {
                        text: "Purchase history"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightSemibold
                    }

                    EmptyState {
                        width: parent.width
                        height: 160
                        visible: root.viewModel && root.viewModel.purchaseCount === 0
                        iconName: "history"
                        title: "No purchases yet"
                        description: "Your past orders will appear here."
                    }

                    Repeater {
                        model: root.viewModel ? root.viewModel.purchaseHistory : []

                        delegate: Column {
                            width: parent.width
                            spacing: Theme.space.xs

                            Rectangle {
                                width: parent.width
                                height: _phRow.height + 2 * Theme.space.md
                                radius: Theme.radius.md
                                color: "transparent"
                                border.color: Theme.color.divider
                                border.width: 1

                                Row {
                                    id: _phRow
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.md

                                    Rectangle {
                                        width: 40; height: 40; radius: 10
                                        color: Theme.color.successSoft
                                        anchors.verticalCenter: parent.verticalCenter
                                        AppIcon {
                                            anchors.centerIn: parent
                                            name: "shopping_bag"
                                            size: 20
                                            color: Theme.color.success
                                        }
                                    }

                                    Column {
                                        width: parent.width - 40 - Theme.space.md - _phTotal.width - Theme.space.md
                                        spacing: 2
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: modelData.titlesSummary
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightSemibold
                                            elide: Text.ElideRight
                                            width: parent.width
                                        }
                                        Text {
                                            text: modelData.relativeDate + " · " + modelData.itemCount + " item(s) · " + modelData.discountText
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }

                                    Text {
                                        id: _phTotal
                                        text: modelData.totalText
                                        color: Theme.color.primary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBodyLarge
                                        font.weight: Theme.font.weightBold
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Settings -----
            Card {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text {
                        text: "Settings"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightSemibold
                    }

                    // Theme toggle
                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        AppIcon {
                            name: "dark_mode"
                            size: Theme.size.iconMd
                            color: Theme.color.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            width: parent.width - _themeToggle.width - Theme.space.md - Theme.size.iconMd - Theme.space.md
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "Dark mode"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightMedium
                            }
                            Text {
                                text: "Easier on the eyes at night"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }

                        AppToggleButton {
                            id: _themeToggle
                            checked: root.darkMode
                            onToggled: {
                                root.themeToggled()
                            }
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Divider {
                        width: parent.width
                        orientation: "horizontal"
                    }

                    // Sign out
                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        AppIcon {
                            name: "logout"
                            size: Theme.size.iconMd
                            color: Theme.color.error
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            width: parent.width - _signOutBtn.width - Theme.space.md - Theme.size.iconMd - Theme.space.md
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: "Sign out"
                                color: Theme.color.error
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightMedium
                            }
                            Text {
                                text: "You'll need to log in again next time."
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }

                        SecondaryButton {
                            id: _signOutBtn
                            text: "Sign out"
                            onClicked: root.logoutRequested()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }

    Component.onCompleted: {
        if (root.viewModel) root.viewModel.loadGenresFromUser()
    }
}
