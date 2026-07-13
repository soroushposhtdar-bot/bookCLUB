// =============================================================================
//  SettingsPage.qml
// =============================================================================
//  Multi-section settings page with a left nav rail + right content area.
//
//  Sections (sidebar order):
//      0  General        — language, animations
//      1  Appearance     — theme, accent, font
//      2  Notifications  — per-event toggles
//      3  Privacy        — sharing + ads
//      4  Reading        — reader theme, font, sync, downloads
//      5  Account        — profile, password, sign out
//      6  Storage        — cache + storage usage
//      7  About          — version, license, help
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs as Dialogs
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/data"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // SettingsViewModel
    property var userService: null  // for the Account section
    property bool darkMode: false

    signal logoutRequested()
    signal themeToggled()
    signal themeChanged(string mode)
    signal accentChanged(string name)
    signal toastRequested(string variant, string title, string description)
    signal navigateToProfileRequested()   // emitted when "Change password" is clicked

    readonly property int _horizontalPadding: Theme.space.xxxl

    Rectangle { anchors.fill: parent; color: Theme.color.pageBackground }

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

            // ----- Header + save bar -----
            Row {
                width: parent.width
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.md

                Text {
                    text: "Settings"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { Layout.fillWidth: true; width: 1; height: 1 }

                // Save indicator
                Row {
                    spacing: Theme.space.sm
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        visible: root.viewModel && root.viewModel.saved
                        text: "✓ Saved"
                        color: Theme.color.success
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightMedium
                    }
                    Spinner {
                        visible: root.viewModel && root.viewModel.saving
                        running: root.viewModel && root.viewModel.saving
                        size: 18
                    }
                    PrimaryButton {
                        text: "Save changes"
                        iconName: "check"
                        iconPosition: "leading"
                        enabled: root.viewModel && !root.viewModel.saving
                        loading: root.viewModel && root.viewModel.saving
                        onClicked: if (root.viewModel) root.viewModel.save()
                    }
                }
            }

            // ----- Two-column body -----
            Row {
                width: parent.width
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.xl

                // ----- Settings sidebar -----
                Column {
                    id: _settingsNav
                    width: 220
                    spacing: 2

                    Repeater {
                        model: [
                            { idx: 0, icon: "settings",           label: "General" },
                            { idx: 1, icon: "palette",            label: "Appearance" },
                            { idx: 2, icon: "notifications",      label: "Notifications" },
                            { idx: 3, icon: "shield",             label: "Privacy" },
                            { idx: 4, icon: "menu_book",          label: "Reading" },
                            { idx: 5, icon: "account_circle",     label: "Account" },
                            { idx: 6, icon: "storage",            label: "Storage" },
                            { idx: 7, icon: "info",               label: "About" }
                        ]
                        delegate: NavItem {
                            width: parent.width
                            iconName: modelData.icon
                            label: modelData.label
                            active: root.viewModel && root.viewModel.activeSection === modelData.idx
                            onClicked: if (root.viewModel) root.viewModel.activeSection = modelData.idx
                        }
                    }
                }

                // ----- Content area -----
                Column {
                    width: parent.width - 220 - Theme.space.xl
                    spacing: Theme.space.lg

                    // ===== General =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 0

                        Text {
                            text: "General"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: Theme.space.lg

                                // Language
                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Language"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Row {
                                        spacing: Theme.space.sm
                                        Repeater {
                                            model: ["English", "فارسی", "Français", "Deutsch"]
                                            delegate: GenreChip {
                                                label: modelData
                                                selected: root.viewModel && root.viewModel.language === modelData
                                                onClicked: if (root.viewModel) root.viewModel.language = modelData
                                            }
                                        }
                                    }
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "animation"
                                    title: "Reduce animations"
                                    description: "Disable page transitions and hover effects for accessibility."
                                    checked: root.viewModel && root.viewModel.reduceAnimations
                                    onToggled: if (root.viewModel) root.viewModel.reduceAnimations = checked
                                }
                            }
                        }
                    }

                    // ===== Appearance =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 1

                        Text {
                            text: "Appearance"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: Theme.space.lg

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Theme"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Row {
                                        spacing: Theme.space.sm
                                        Repeater {
                                            model: [
                                                { v: "light", label: "Light", icon: "light_mode" },
                                                { v: "dark",  label: "Dark",  icon: "dark_mode" },
                                                { v: "auto",  label: "Auto",  icon: "contrast" }
                                            ]
                                            delegate: GenreChip {
                                                label: modelData.label
                                                iconName: modelData.icon
                                                selected: root.viewModel && root.viewModel.theme === modelData.v
                                                onClicked: {
                                                    if (root.viewModel) root.viewModel.theme = modelData.v
                                                    root.themeChanged(modelData.v)
                                                }
                                            }
                                        }
                                    }
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Accent color"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Row {
                                        spacing: Theme.space.sm
                                        Repeater {
                                            model: Theme.accentPalette
                                            delegate: Item {
                                                width: 36; height: 36
                                                Rectangle {
                                                    anchors.fill: parent
                                                    radius: 18
                                                    color: modelData.color
                                                    border.color: root.viewModel && root.viewModel.accentName === modelData.name ? Theme.color.textPrimary : "transparent"
                                                    border.width: 3
                                                    Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
                                                }
                                                MouseArea {
                                                    anchors.fill: parent
                                                    cursorShape: Qt.PointingHandCursor
                                                    onClicked: {
                                                        if (root.viewModel) root.viewModel.accentName = modelData.name
                                                        root.accentChanged(modelData.name)
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Font size: " + (root.viewModel ? root.viewModel.fontSize : 14) + "px"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Slider {
                                        width: parent.width
                                        from: 12; to: 20; stepSize: 1
                                        value: root.viewModel ? root.viewModel.fontSize : 14
                                        onMoved: if (root.viewModel) root.viewModel.fontSize = value
                                    }
                                }
                            }
                        }
                    }

                    // ===== Notifications =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 2

                        Text {
                            text: "Notifications"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: 0

                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "new_releases"
                                    title: "New books in favorite genres"
                                    description: "Get notified when a new release matches your taste."
                                    checked: root.viewModel && root.viewModel.notifNewBooks
                                    onToggled: if (root.viewModel) root.viewModel.notifNewBooks = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "local_offer"
                                    title: "Discounts on saved books"
                                    description: "Price drops on your wishlist."
                                    checked: root.viewModel && root.viewModel.notifDiscounts
                                    onToggled: if (root.viewModel) root.viewModel.notifDiscounts = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "shopping_bag"
                                    title: "Purchase confirmations"
                                    description: "Confirmations for completed orders."
                                    checked: root.viewModel && root.viewModel.notifSales
                                    onToggled: if (root.viewModel) root.viewModel.notifSales = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "rate_review"
                                    title: "Replies to your reviews"
                                    description: "When someone replies to or likes your review."
                                    checked: root.viewModel && root.viewModel.notifReviews
                                    onToggled: if (root.viewModel) root.viewModel.notifReviews = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "mail"
                                    title: "Weekly email digest"
                                    description: "A Monday summary of new releases and trends."
                                    checked: root.viewModel && root.viewModel.notifEmailDigest
                                    onToggled: if (root.viewModel) root.viewModel.notifEmailDigest = checked
                                }
                            }
                        }
                    }

                    // ===== Privacy =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 3

                        Text {
                            text: "Privacy"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: 0
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "visibility"
                                    title: "Share reading activity"
                                    description: "Let friends see what you're reading."
                                    checked: root.viewModel && root.viewModel.shareReading
                                    onToggled: if (root.viewModel) root.viewModel.shareReading = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "bookmark"
                                    title: "Public wishlist"
                                    description: "Anyone with the link can see your wishlist."
                                    checked: root.viewModel && root.viewModel.shareWishlist
                                    onToggled: if (root.viewModel) root.viewModel.shareWishlist = checked
                                }
                                Divider { width: parent.width; orientation: "horizontal" }
                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "campaign"
                                    title: "Personalized recommendations"
                                    description: "Use my reading history to suggest books."
                                    checked: root.viewModel && root.viewModel.personalAds
                                    onToggled: if (root.viewModel) root.viewModel.personalAds = checked
                                }
                            }
                        }
                    }

                    // ===== Reading =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 4

                        Text {
                            text: "Reading preferences"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: Theme.space.lg

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Reader theme"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Row {
                                        spacing: Theme.space.sm
                                        Repeater {
                                            model: [
                                                { v: "light", label: "Light" },
                                                { v: "sepia", label: "Sepia" },
                                                { v: "dark",  label: "Dark" }
                                            ]
                                            delegate: GenreChip {
                                                label: modelData.label
                                                selected: root.viewModel && root.viewModel.readerTheme === modelData.v
                                                onClicked: if (root.viewModel) root.viewModel.readerTheme = modelData.v
                                            }
                                        }
                                    }
                                }

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Font size: " + (root.viewModel ? root.viewModel.readerFontSize : 16) + "px"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Slider {
                                        width: parent.width
                                        from: 12; to: 24; stepSize: 1
                                        value: root.viewModel ? root.viewModel.readerFontSize : 16
                                        onMoved: if (root.viewModel) root.viewModel.readerFontSize = value
                                    }
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "sync"
                                    title: "Sync reading position"
                                    description: "Resume from the same page across devices."
                                    checked: root.viewModel && root.viewModel.readerSync
                                    onToggled: if (root.viewModel) root.viewModel.readerSync = checked
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm
                                    Text {
                                        text: "Download location"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Row {
                                        width: parent.width
                                        spacing: Theme.space.md
                                        InputField {
                                            width: parent.width - _browseBtn.width - Theme.space.md
                                            text: root.viewModel ? root.viewModel.downloadLocation : ""
                                            onTextEdited: if (root.viewModel) root.viewModel.downloadLocation = newText
                                        }
                                        SecondaryButton {
                                            id: _browseBtn
                                            text: "Browse"
                                            onClicked: {
                                                // Open a real folder picker via Qt.Dialogs.
                                                // Falls back to a toast if the dialog can't open.
                                                _folderDialog.open()
                                            }
                                        }
                                    }
                                }

                                SettingToggleRow {
                                    width: parent.width
                                    iconName: "download"
                                    title: "Auto-download purchased books"
                                    description: "Download to your device immediately after purchase."
                                    checked: root.viewModel && root.viewModel.autoDownload
                                    onToggled: if (root.viewModel) root.viewModel.autoDownload = checked
                                }
                            }
                        }
                    }

                    // ===== Account =====
                    Column {
                        width: parent.width
                        spacing: Theme.space.lg
                        visible: root.viewModel && root.viewModel.activeSection === 5

                        Text {
                            text: "Account"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.xl

                            Column {
                                width: parent.width
                                spacing: Theme.space.lg

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.md

                                        Rectangle {
                                            width: 64; height: 64; radius: 32
                                            color: Theme.color.primary
                                            Text {
                                                anchors.centerIn: parent
                                                text: root.userService ? root.userService.initials : "?"
                                                color: Theme.color.onPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeHeadline
                                                font.weight: Theme.font.weightBold
                                            }
                                        }

                                        Column {
                                            width: parent.width - 64 - Theme.space.md - _changeAvatarBtn.width - Theme.space.md
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: root.userService ? root.userService.displayName : ""
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBodyLarge
                                                font.weight: Theme.font.weightSemibold
                                            }
                                            Text {
                                                text: "@" + (root.userService ? root.userService.username : "")
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                        }

                                        SecondaryButton {
                                            id: _changeAvatarBtn
                                            text: "Change"
                                            onClicked: {
                                                // Open a real file picker for avatar image selection.
                                                _avatarDialog.open()
                                            }
                                        }
                                    }

                                    Divider { width: parent.width; orientation: "horizontal" }

                                    PrimaryButton {
                                        text: "Change password"
                                        iconName: "lock"
                                        iconPosition: "leading"
                                        onClicked: {
                                            // Route to the Profile page which has the real,
                                            // working change-password form (current/new/confirm
                                            // fields with validation + strength meter).
                                            root.navigateToProfileRequested()
                                        }
                                    }

                                    Divider { width: parent.width; orientation: "horizontal" }

                                    SettingToggleRow {
                                        width: parent.width
                                        iconName: "logout"
                                        title: "Sign out"
                                        description: "You'll need to log in again next time."
                                        destructive: true
                                        checked: false
                                        onToggled: root.logoutRequested()
                                    }
                                }
                            }
                        }

                        // ===== Storage =====
                        Column {
                            width: parent.width
                            spacing: Theme.space.lg
                            visible: root.viewModel && root.viewModel.activeSection === 6

                            Text {
                                text: "Storage"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                            }

                            Card {
                                width: parent.width
                                bordered: true
                                padding: Theme.space.xl

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.lg

                                    // Cache size
                                    Row {
                                        width: parent.width
                                        Text {
                                            text: "Cache size"
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                        }
                                        Item { Layout.fillWidth: true; width: 1; height: 1 }
                                        Text {
                                            text: root.viewModel ? root.viewModel.cacheSize : ""
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                        }
                                    }

                                    SecondaryButton {
                                        text: "Clear cache"
                                        iconName: "delete"
                                        iconPosition: "leading"
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.clearCache()
                                                root.toastRequested("success", "Cache cleared",
                                                                     "Download cache has been cleared.")
                                            }
                                        }
                                    }

                                    Divider { width: parent.width; orientation: "horizontal" }

                                    // Storage usage bar
                                    Column {
                                        width: parent.width
                                        spacing: Theme.space.sm
                                        Text {
                                            text: "Storage used"
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                        }
                                        Rectangle {
                                            width: parent.width; height: 8; radius: 4
                                            color: Theme.color.fieldFilled
                                            Rectangle {
                                                // Storage bar width bound to the VM's storagePct property (0..1).
                                                width: parent.width * (root.viewModel ? root.viewModel.storagePct : 0.28); height: parent.height; radius: parent.radius
                                                color: Theme.color.accent
                                                Behavior on width { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
                                            }
                                        }
                                        Text {
                                            text: root.viewModel ? root.viewModel.storageUsed : ""
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                }
                            }
                        }

                        // ===== About =====
                        Column {
                            width: parent.width
                            spacing: Theme.space.lg
                            visible: root.viewModel && root.viewModel.activeSection === 7

                            Text {
                                text: "About"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                            }

                            Card {
                                width: parent.width
                                bordered: true
                                padding: Theme.space.xl

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.lg

                                    Row {
                                        width: parent.width
                                        spacing: Theme.space.md
                                        Rectangle {
                                            width: 48; height: 48; radius: 12
                                            color: Theme.color.primary
                                            Text {
                                                anchors.centerIn: parent
                                                text: "B"
                                                color: Theme.color.onPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeHeadline
                                                font.weight: Theme.font.weightBold
                                            }
                                        }
                                        Column {
                                            spacing: 0
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: "BookClub"
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBodyLarge
                                                font.weight: Theme.font.weightSemibold
                                            }
                                            Text {
                                                text: "Version 1.0.0 (build 2025.07)"
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                        }
                                    }

                                    Divider { width: parent.width; orientation: "horizontal" }

                                    Repeater {
                                        model: [
                                            { icon: "help", label: "Help center", value: "" },
                                            { icon: "feedback", label: "Send feedback", value: "" },
                                            { icon: "description", label: "Open-source licenses", value: "" },
                                            { icon: "update", label: "Check for updates", value: "" }
                                        ]
                                        delegate: Item {
                                            width: parent.width
                                            height: 44
                                            Row {
                                                anchors.fill: parent
                                                spacing: Theme.space.md
                                                AppIcon {
                                                    name: modelData.icon
                                                    size: 20
                                                    color: Theme.color.textSecondary
                                                    anchors.verticalCenter: parent.verticalCenter
                                                }
                                                Text {
                                                    text: modelData.label
                                                    color: Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBody
                                                    font.weight: Theme.font.weightMedium
                                                    anchors.verticalCenter: parent.verticalCenter
                                                }
                                                Item { Layout.fillWidth: true; width: 1; height: 1 }
                                                AppIcon {
                                                    name: "chevron_right"
                                                    size: 18
                                                    color: Theme.color.textMuted
                                                    anchors.verticalCenter: parent.verticalCenter
                                                }
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    // Wire each About list item to a real action.
                                                    switch (modelData.label) {
                                                        case "Help center":
                                                            Qt.openUrlExternally("https://bookclub.app/help")
                                                            break
                                                        case "Send feedback":
                                                            Qt.openUrlExternally("mailto:support@bookclub.app?subject=BookClub%20Feedback")
                                                            break
                                                        case "Open-source licenses":
                                                            root.toastRequested("info", "Open-source licenses",
                                                                                 "This app uses Qt, Material Symbols, and other open-source software.")
                                                            break
                                                        case "Check for updates":
                                                            root.toastRequested("info", "Up to date",
                                                                                 "You're running the latest version of BookClub.")
                                                            break
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // ----- Folder picker for download location -----
    // Qt5-compatible: uses FileDialog with selectFolder: true.
    Dialogs.FileDialog {
        id: _folderDialog
        title: "Choose download folder"
        selectFolder: true
        onAccepted: {
            if (root.viewModel && _folderDialog.selectedFile) {
                root.viewModel.downloadLocation = _folderDialog.selectedFile.toString().replace("file://", "")
                root.toastRequested("success", "Folder set",
                                     "New purchases will download to the selected folder.")
            }
        }
        onRejected: {
            root.toastRequested("info", "Browse",
                                 "Folder picker cancelled — type a path above to set manually.")
        }
    }

    // ----- File picker for avatar image -----
    Dialogs.FileDialog {
        id: _avatarDialog
        title: "Choose avatar image"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: {
            // In a real build this would upload the image and update the
            // user's avatar. For the mock we just acknowledge the selection.
            root.toastRequested("success", "Avatar selected",
                                 "Your avatar will be updated on next sign-in.")
        }
        onRejected: {
            // Silent cancel
        }
    }
