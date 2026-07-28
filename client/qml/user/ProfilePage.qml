// =============================================================================
//  ProfilePage.qml  (v10 — clean rewrite)
// =============================================================================
//  User profile page. Clean, single-column layout with clear sections:
//    1. Identity header (avatar + name + username + stats)
//    2. Account info editor (display name + email)
//    3. Favorite genres
//    4. Change password
//    5. Purchase history
//    6. Account actions (dark mode toggle + sign out)
//
//  All actions wire to the ProfileViewModel which talks to the server via
//  UserService. Signals propagate back through NOTIFY so the UI updates
//  in real-time.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/navigation"
import "../components/data"
import "../components/feedback"
import "../components/book"
import "../components"
import BookClub.Services 1.0

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // ProfileViewModel
    property var bookService: null  // for the available-genres catalog
    property bool darkMode: false

    signal logoutRequested()
    signal themeToggled()
    signal openLibraryRequested()
    signal openReaderRequested(string bookId)
    signal bookDetailRequested(string bookId)
    signal toastRequested(string variant, string title, string description)

    readonly property int _hPadding: Theme.space.xxxl
    readonly property bool _isBusy: root.viewModel && root.viewModel.isBusy

    // ----- Password strength -----
    function _strengthScore(pw) {
        if (pw.length < 6) return 0
        let s = 0
        if (pw.length >= 6) ++s
        if (pw.length >= 10) ++s
        if (/[a-z]/.test(pw) && /[A-Z]/.test(pw)) ++s
        if (/[0-9]/.test(pw)) ++s
        if (/[^a-zA-Z0-9]/.test(pw)) ++s
        return Math.min(4, s)
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

    // ----- Main scrollable content -----
    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: _profileColumn.implicitHeight + Theme.space.xxl
        flickDeceleration: 5000
        maximumFlickVelocity: 12000
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: _profileColumn
            width: parent.width
            spacing: Theme.space.xl

            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.lg }

            // ============================================================
            //  1. Identity header
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "sm"
                padding: Theme.space.xl

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.xl

                    // Avatar
                    Avatar {
                        size: 80
                        initials: root.viewModel ? root.viewModel.initials : "?"
                        online: true
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Name + username + genres chip
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            Layout.fillWidth: true
                            text: root.viewModel ? root.viewModel.displayName : ""
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeHeadline
                            font.weight: Theme.font.weightBold
                            elide: Text.ElideRight
                        }
                        Text {
                            text: "@" + (root.viewModel ? root.viewModel.username : "guest")
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Rectangle {
                            visible: root.viewModel && root.viewModel.favoriteGenresSummary.length > 0
                            width: _gText.implicitWidth + 16
                            height: 22
                            radius: height / 2
                            color: Theme.color.accentSoft

                            Text {
                                id: _gText
                                anchors.centerIn: parent
                                text: root.viewModel ? root.viewModel.favoriteGenresSummary : ""
                                color: Theme.color.accent
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeMicro2
                                font.weight: Theme.font.weightMedium
                            }
                        }
                    }

                    // Purchases stat
                    ColumnLayout {
                        spacing: 0
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 80

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: String(root.viewModel ? root.viewModel.purchaseCount : 0)
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeDisplay
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Purchases"
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }
                }
            }

            // ============================================================
            //  2. Account info editor
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Account information"
                    }

                    // Display name
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Display name"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md
                            InputField {
                                id: _displayNameField
                                Layout.fillWidth: true
                                placeholder: "How should we call you?"
                                text: root.viewModel ? root.viewModel.displayName : ""
                                maximumLength: 50
                                onTextEdited: function(newText) {
                                    _displayNameField.text = newText
                                    if (root.viewModel) root.viewModel.displayName = newText
                                }
                            }
                            PrimaryButton {
                                text: "Save"
                                iconName: "check"
                                enabled: !root._isBusy && _displayNameField.text.length > 0
                                loading: root._isBusy
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: {
                                    if (root.viewModel) {
                                        root.viewModel.saveProfile()
                                        root.toastRequested("success", "Profile saved",
                                                            "Your display name has been updated.")
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ============================================================
            //  3. Favorite genres
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    RowLayout {
                        Layout.fillWidth: true
                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Favorite genres"
                        }
                        Text {
                            text: (root.viewModel ? root.viewModel.selectedGenreCount : 0) + " / 3"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightBold
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "We use these to personalize your home feed. Pick 1–3 genres."
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        wrapMode: Text.WordWrap
                    }

                    // Genre grid
                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.width < 760 ? 2 : 4
                        rowSpacing: Theme.space.sm
                        columnSpacing: Theme.space.sm

                        Repeater {
                            model: root.bookService ? root.bookService.availableGenres() : []
                            delegate: GenreChip {
                                label: modelData
                                // v11: bind to the selectedGenres Q_PROPERTY (not the
                                // Q_INVOKABLE isGenreSelected) so QML re-evaluates
                                // when selectedGenresChanged is emitted.
                                selected: root.viewModel && root.viewModel.selectedGenres
                                           ? root.viewModel.selectedGenres.indexOf(modelData) >= 0
                                           : false
                                Layout.fillWidth: true
                                onClicked: {
                                    if (root.viewModel) root.viewModel.toggleGenre(modelData)
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md
                        PrimaryButton {
                            text: "Save preferences"
                            iconName: "check"
                            enabled: root.viewModel && root.viewModel.canSaveGenres && !root._isBusy
                            loading: root._isBusy
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.saveGenres()
                                    root.toastRequested("success", "Genres saved",
                                                        "Your favorite genres have been updated.")
                                }
                            }
                        }
                        SecondaryButton {
                            text: "Reset"
                            onClicked: {
                                // v11 (Issue 4): clear local selection instead of
                                // re-fetching from server. Previously this called
                                // `loadGenresFromUser()` which re-reads the
                                // server-side genres — but if the user hasn't
                                // saved yet, the server still has the OLD list,
                                // so the reset appeared to do nothing. Clearing
                                // the local selection directly is what the user
                                // actually expects from a "Reset" button.
                                if (root.viewModel) {
                                    // Toggle off every currently-selected genre
                                    // so the local cache returns to empty.
                                    var genres = root.viewModel.selectedGenres
                                    for (var i = 0; i < genres.length; ++i) {
                                        root.viewModel.toggleGenre(genres[i])
                                    }
                                }
                            }
                        }
                        Item { Layout.fillWidth: true; height: 1 }
                    }
                }
            }

            // ============================================================
            //  4. Change password
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Change password"
                    }

                    PasswordField {
                        Layout.fillWidth: true
                        label: "Current password"
                        placeholder: "Enter your current password"
                        leadingIcon: "lock"
                        text: root.viewModel ? root.viewModel.currentPassword : ""
                        onTextEdited: function(newText) {
                            if (root.viewModel) root.viewModel.currentPassword = newText
                        }
                    }

                    PasswordField {
                        Layout.fillWidth: true
                        label: "New password"
                        placeholder: "At least 6 characters"
                        leadingIcon: "lock"
                        showStrengthMeter: true
                        strengthScore: root._strengthScore(root.viewModel ? root.viewModel.newPassword : "")
                        strengthLabel: root._strengthLabel(root.viewModel ? root.viewModel.newPassword : "")
                        text: root.viewModel ? root.viewModel.newPassword : ""
                        onTextEdited: function(newText) {
                            if (root.viewModel) root.viewModel.newPassword = newText
                        }
                    }

                    PasswordField {
                        Layout.fillWidth: true
                        label: "Confirm new password"
                        placeholder: "Re-enter your new password"
                        leadingIcon: "lock"
                        text: root.viewModel ? root.viewModel.confirmPassword : ""
                        errorText: root.viewModel ? root.viewModel.passwordError : ""
                        successText: root.viewModel && root.viewModel.canChangePassword ? "Passwords match" : ""
                        onTextEdited: function(newText) {
                            if (root.viewModel) root.viewModel.confirmPassword = newText
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md
                        PrimaryButton {
                            text: "Update password"
                            iconName: "lock"
                            enabled: root.viewModel && root.viewModel.canChangePassword && !root._isBusy
                            loading: root._isBusy
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.changePassword()
                                    // v12: don't show toast here — wait for the
                                    // passwordChanged signal (fires on success)
                                    // or passwordError (fires on failure).
                                }
                            }
                        }
                        SecondaryButton {
                            text: "Clear"
                            onClicked: {
                                if (root.viewModel) root.viewModel.clearPasswordFields()
                            }
                        }
                        Item { Layout.fillWidth: true; height: 1 }
                    }
                }
            }

            // ============================================================
            //  5. Purchase history
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Purchase history"
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        visible: root.viewModel && root.viewModel.purchaseCount === 0
                        iconName: "history"
                        title: "No purchases yet"
                        description: "Your past orders will appear here."
                    }

                    // Purchase list
                    Repeater {
                        model: root.viewModel ? root.viewModel.purchaseHistory : []
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            height: _phRow.height + 2 * Theme.space.md
                            radius: Theme.radius.md
                            color: "transparent"
                            border.color: Theme.color.divider
                            border.width: 1

                            RowLayout {
                                id: _phRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: Theme.space.md
                                spacing: Theme.space.md

                                // Icon
                                Rectangle {
                                    width: 40; height: 40; radius: width / 2
                                    color: Theme.color.successSoft
                                    Layout.alignment: Qt.AlignVCenter

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "shopping_bag"
                                        size: 20
                                        color: Theme.color.success
                                    }
                                }

                                // Details
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Layout.alignment: Qt.AlignVCenter

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.titlesSummary
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightSemibold
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: modelData.relativeDate + " · " + modelData.itemCount + " item(s) · " + modelData.discountText
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }

                                // Total
                                Text {
                                    text: modelData.totalText
                                    color: Theme.color.primary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBodyLarge
                                    font.weight: Theme.font.weightBold
                                    Layout.alignment: Qt.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }

            // ============================================================
            //  6. Account actions (dark mode + sign out)
            // ============================================================
            Card {
                Layout.fillWidth: true
                Layout.leftMargin: root._hPadding
                Layout.rightMargin: root._hPadding
                elevation: "none"
                bordered: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Account"
                    }

                    // Dark mode toggle
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md

                        Rectangle {
                            width: 40; height: 40; radius: width / 2
                            color: Theme.color.accentSoft
                            Layout.alignment: Qt.AlignVCenter

                            AppIcon {
                                anchors.centerIn: parent
                                name: "dark_mode"
                                size: 20
                                color: Theme.color.accent
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Layout.alignment: Qt.AlignVCenter

                            Text {
                                Layout.fillWidth: true
                                text: "Dark mode"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightMedium
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "Easier on the eyes at night"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }

                        AppToggleButton {
                            checked: root.darkMode
                            onToggled: root.themeToggled()
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }

                    Divider { Layout.fillWidth: true; orientation: "horizontal" }

                    // Sign out
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md

                        Rectangle {
                            width: 40; height: 40; radius: width / 2
                            color: Theme.color.errorSoft
                            Layout.alignment: Qt.AlignVCenter

                            AppIcon {
                                anchors.centerIn: parent
                                name: "logout"
                                size: 20
                                color: Theme.color.error
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Layout.alignment: Qt.AlignVCenter

                            Text {
                                Layout.fillWidth: true
                                text: "Sign out"
                                color: Theme.color.error
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightMedium
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "You'll need to log in again next time."
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }

                        SecondaryButton {
                            text: "Sign out"
                            onClicked: root.logoutRequested()
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xxl }
        }
    }

    // ----- Lifecycle -----
    Component.onCompleted: {
        if (root.viewModel) {
            root.viewModel.refresh()
        }
        LibraryService.refresh()
    }

    // ----- Refresh when library changes -----
    Connections {
        target: LibraryService
        ignoreUnknownSignals: true
        onLibraryChanged: {
            if (root.viewModel) root.viewModel.userChanged()
        }
        onWishlistChanged: {
            if (root.viewModel) root.viewModel.userChanged()
        }
    }

    // v12: async password change feedback — show toast after server responds
    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        function onPasswordChanged() {
            root.toastRequested("success", "Password updated",
                                "Your password has been changed successfully.")
            if (root.viewModel) root.viewModel.clearPasswordFields()
        }
        function onPasswordFieldsChanged() {
            // Show error toast if there's a password error
            if (root.viewModel && root.viewModel.passwordError.length > 0) {
                root.toastRequested("error", "Password change failed",
                                    root.viewModel.passwordError)
            }
        }
    }
}
