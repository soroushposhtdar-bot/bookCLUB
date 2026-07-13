// =============================================================================
//  AdminUserDetailDrawer.qml
// =============================================================================
//  Slide-in drawer for the admin role showing the full profile of a single
//  user. Implements spec §4-1 (user management) and §4-2 (access management):
//    • Account summary (avatar / display name / username / role / status)
//    • Email + last-active timestamp
//    • Activity stats (books owned / reviews posted / reports filed)
//    • Login history table (device / IP / time / success flag)
//    • Memberships table (plan / since / status / price)
//    • Access-management actions: block / unblock / delete + role switcher
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `userDetails(username)` → QVariantMap, `userLoginHistory(username)`, and
//  `userMemberships(username)`. The drawer calls these once when opened and
//  again whenever the underlying user record changes (so a block / unblock /
//  role change from within the drawer reflects immediately).
//
//  Usage:
//    AdminUserDetailDrawer {
//        id: _userDrawer
//        viewModel: _adminVM
//        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
//    }
//    _userDrawer.openForUser("alice_w")
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"

import BookClub.Services 1.0

Item {
    id: drawer

    // ----- AdminViewModel (injected by parent) -----
    property var viewModel: null

    // ----- Currently displayed username -----
    property string username: ""
    property var _detail: ({})

    signal toastRequested(string variant, string title, string description)
    signal closed()

    // ----- Visual state -----
    visible: false
    width: 460

    // ----- Role → color map -----
    function _roleColor(role) {
        if (role === "admin")     return Theme.color.error
        if (role === "publisher") return Theme.color.warning
        if (role === "server")    return Theme.color.success
        return Theme.color.accent
    }
    function _roleSoft(role) {
        if (role === "admin")     return Theme.color.errorSoft
        if (role === "publisher") return Theme.color.warningSoft
        if (role === "server")    return Theme.color.successSoft
        return Theme.color.accentSoft
    }

    function openForUser(uname) {
        drawer.username = uname
        drawer._reload()
        drawer.visible = true
        _slideIn.from = drawer.width
        _slideIn.start()
    }

    function _reload() {
        if (!drawer.viewModel || drawer.username.length === 0) {
            drawer._detail = {}
            return
        }
        const d = drawer.viewModel.userDetails(drawer.username)
        drawer._detail = d || {}
    }

    function close() {
        _slideOut.from = 0
        _slideOut.to = drawer.width
        _slideOut.start()
        // Hide after the slide-out finishes.
        _hideTimer.start()
    }

    Timer {
        id: _hideTimer
        // Drive from Theme.motion.durationBase so the hide fires exactly
        // when the slide-out animation finishes (previously hardcoded to
        // 260ms, which could drift if durationBase changed).
        interval: Theme.motion.durationBase
        repeat: false
        onTriggered: {
            drawer.visible = false
            // Emit the closed() signal so parents can react (e.g. clear the
            // pending username). Previously this signal was declared but
            // never emitted, so subscribers never fired.
            drawer.closed()
        }
    }

    // Reload whenever the VM signals usersChanged (covers block/unblock/role/delete).
    Connections {
        target: drawer.viewModel
        ignoreUnknownSignals: true
        onUsersChanged: drawer._reload()
    }

    // Sliding animations
    NumberAnimation {
        id: _slideIn
        target: _content
        property: "x"
        from: drawer.width
        to: 0
        duration: Theme.motion.durationSlow
        easing.type: Easing.OutCubic
    }
    NumberAnimation {
        id: _slideOut
        target: _content
        property: "x"
        from: 0
        to: drawer.width
        duration: Theme.motion.durationBase
        easing.type: Easing.InCubic
    }

    // Scrim (click outside to close)
    Item {
        id: _scrim
        anchors.fill: parent
        visible: drawer.visible
        Rectangle {
            anchors.fill: parent
            color: Theme.color.overlayScrim
            MouseArea {
                anchors.fill: parent
                onClicked: drawer.close()
            }
        }
    }

    // Drawer content
    Rectangle {
        id: _content
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: drawer.width
        color: Theme.color.cardBackground

        // Left edge shadow
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }

        Column {
            anchors.fill: parent
            spacing: 0

            // ----- Header -----
            Rectangle {
                width: parent.width
                height: 72
                color: "transparent"

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.color.divider
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.lg
                    anchors.rightMargin: Theme.space.lg
                    spacing: Theme.space.md

                    Text {
                        text: "User details"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    IconButton {
                        iconName: "close"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: drawer.close()
                    }
                }
            }

            // ----- Body (scrollable) -----
            ScrollView {
                width: parent.width
                height: parent.height - 72
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: Theme.space.lg

                    // ----- Profile header (avatar + name + role + status) -----
                    Item {
                        width: parent.width
                        height: 100

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: Theme.space.lg

                            Rectangle {
                                width: 64; height: 64; radius: 32
                                color: drawer._detail.avatarColor || Theme.color.accent
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    anchors.centerIn: parent
                                    text: drawer._detail.initials || "?"
                                    color: Theme.color.textOnAccent
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeTitle
                                    font.weight: Theme.font.weightBold
                                }
                            }

                            Column {
                                spacing: 4
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: drawer._detail.displayName || drawer.username
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeTitle
                                    font.weight: Theme.font.weightBold
                                }
                                Text {
                                    text: "@" + drawer.username
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeBody
                                }
                                Row {
                                    spacing: Theme.space.sm
                                    // Role badge
                                    Rectangle {
                                        width: _roleLbl.implicitWidth + 16
                                        height: 24
                                        radius: 12
                                        color: drawer._roleSoft(drawer._detail.role || "user")
                                        Text {
                                            id: _roleLbl
                                            anchors.centerIn: parent
                                            text: drawer._detail.role || "user"
                                            color: drawer._roleColor(drawer._detail.role || "user")
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                            font.capitalization: Font.Capitalize
                                        }
                                    }
                                    // Status badge
                                    Rectangle {
                                        width: _statLbl.implicitWidth + 16
                                        height: 24
                                        radius: 12
                                        color: drawer._detail.status === "Active"
                                               ? Theme.color.successSoft : Theme.color.errorSoft
                                        Text {
                                            id: _statLbl
                                            anchors.centerIn: parent
                                            text: drawer._detail.status || "Active"
                                            color: drawer._detail.status === "Active"
                                                   ? Theme.color.success : Theme.color.error
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ----- Account info (email + last active) -----
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Account"
                                subtitle: "Identity & activity"
                            }

                            // Email
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "mail"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: drawer._detail.email || "—"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            // Joined
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "event"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: "Joined " + (drawer._detail.joined || "—")
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            // Last active
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "schedule"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: "Last active: " + (drawer._detail.lastActive || "—")
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    // ----- Activity stats -----
                    Row {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.space.md

                        Repeater {
                            model: [
                                { label: "Books owned",    value: drawer._detail.booksOwned    || 0, icon: "library_books", color: Theme.color.accent },
                                { label: "Reviews posted", value: drawer._detail.reviewsPosted || 0, icon: "rate_review",   color: Theme.color.info },
                                { label: "Reports filed",  value: drawer._detail.reportsFiled  || 0, icon: "report",        color: Theme.color.warning }
                            ]
                            Rectangle {
                                width: (parent.width - 2 * Theme.space.md) / 3
                                height: 88
                                radius: Theme.radius.md
                                color: Theme.color.fieldFilled
                                border.color: Theme.color.divider
                                border.width: 1

                                Column {
                                    anchors.centerIn: parent
                                    spacing: Theme.space.xs

                                    AppIcon {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        name: modelData.icon
                                        size: 20
                                        color: modelData.color
                                    }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: (modelData.value).toLocaleString(Qt.locale(), "f", 0)
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightBold
                                    }
                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: modelData.label
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                            }
                        }
                    }

                    // ----- Login history -----
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Login history"
                                subtitle: "Recent sign-in attempts"
                            }

                            // Header row
                            Row {
                                width: parent.width
                                spacing: 0
                                Text { width: parent.width * 0.42; text: "Device"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; elide: Text.ElideRight }
                                Text { width: parent.width * 0.24; text: "IP";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; elide: Text.ElideRight }
                                Text { width: parent.width * 0.22; text: "Time";   color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; elide: Text.ElideRight }
                                Text { width: parent.width * 0.12; text: "Status"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; horizontalAlignment: Text.AlignRight; elide: Text.ElideRight }
                            }

                            Rectangle { width: parent.width; height: 1; color: Theme.color.divider }

                            ListView {
                                width: parent.width
                                height: Math.max(0, (drawer._detail.logins || []).length) * 36
                                clip: true
                                interactive: false
                                model: drawer._detail.logins || []
                                spacing: 0

                                delegate: Row {
                                    width: parent.width
                                    height: 36
                                    spacing: 0

                                    Text { width: parent.width * 0.42; text: modelData.device; color: Theme.color.textPrimary;   font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                    Text { width: parent.width * 0.24; text: modelData.ip;     color: Theme.color.textSecondary; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                    Text { width: parent.width * 0.22; text: modelData.time;   color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                    Row {
                                        width: parent.width * 0.12
                                        layoutDirection: Qt.RightToLeft
                                        anchors.verticalCenter: parent.verticalCenter
                                        Rectangle {
                                            width: 6; height: 6; radius: 3
                                            color: modelData.success ? Theme.color.success : Theme.color.error
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ----- Memberships -----
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Memberships"
                                subtitle: "Active plans"
                            }

                            ListView {
                                width: parent.width
                                height: Math.max(0, (drawer._detail.memberships || []).length) * 72
                                clip: true
                                interactive: false
                                model: drawer._detail.memberships || []
                                spacing: Theme.space.sm

                                delegate: Rectangle {
                                    width: parent.width
                                    height: 64
                                    radius: Theme.radius.md
                                    color: Theme.color.fieldFilled
                                    border.color: Theme.color.divider

                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: Theme.space.md
                                        anchors.rightMargin: Theme.space.md
                                        spacing: Theme.space.sm

                                        AppIcon {
                                            name: "card_membership"
                                            size: 18
                                            color: modelData.status === "Active" ? Theme.color.accent : Theme.color.textMuted
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Column {
                                            spacing: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: modelData.plan
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                font.weight: Theme.font.weightMedium
                                            }
                                            Row {
                                                spacing: Theme.space.xs
                                                Text {
                                                    text: "Since " + (modelData.since || "—")
                                                    color: Theme.color.textMuted
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeCaption
                                                }
                                                Text { text: "·"; color: Theme.color.textMuted; font.pixelSize: Theme.font.sizeCaption }
                                                Text {
                                                    text: "$" + Number(modelData.price || 0).toFixed(2) + "/mo"
                                                    color: Theme.color.textSecondary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeCaption
                                                }
                                            }
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }

                                        // Status badge
                                        Rectangle {
                                            width: _memStatusLbl.implicitWidth + 12; height: 20; radius: 10
                                            color: modelData.status === "Active" ? Theme.color.successSoft
                                                 : modelData.status === "Suspended" ? Theme.color.warningSoft
                                                 : Theme.color.errorSoft
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                id: _memStatusLbl
                                                anchors.centerIn: parent
                                                text: modelData.status || "Active"
                                                color: modelData.status === "Active" ? Theme.color.success
                                                     : modelData.status === "Suspended" ? Theme.color.warning
                                                     : Theme.color.error
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeMicro2
                                                font.weight: Theme.font.weightBold
                                            }
                                        }

                                        // Action buttons (spec §4-2: manage active/inactive memberships)
                                        IconButton {
                                            iconName: modelData.status === "Active" ? "pause" : "play_arrow"
                                            iconColor: modelData.status === "Active" ? Theme.color.warning : Theme.color.success
                                            anchors.verticalCenter: parent.verticalCenter
                                            onClicked: {
                                                if (!drawer.viewModel) return
                                                if (modelData.status === "Active") {
                                                    drawer.viewModel.suspendMembership(drawer.username, index)
                                                    drawer._reload()
                                                    drawer.toastRequested("warning", "Membership suspended",
                                                                         "Suspended " + modelData.plan + " for @" + drawer.username + ".")
                                                } else {
                                                    drawer.viewModel.reactivateMembership(drawer.username, index)
                                                    drawer._reload()
                                                    drawer.toastRequested("success", "Membership reactivated",
                                                                         "Reactivated " + modelData.plan + " for @" + drawer.username + ".")
                                                }
                                            }
                                        }
                                        IconButton {
                                            iconName: "delete"
                                            iconColor: Theme.color.error
                                            anchors.verticalCenter: parent.verticalCenter
                                            onClicked: {
                                                if (drawer.viewModel) {
                                                    drawer.viewModel.cancelMembership(drawer.username, index)
                                                    drawer._reload()
                                                    drawer.toastRequested("warning", "Membership cancelled",
                                                                         "Cancelled " + modelData.plan + " for @" + drawer.username + ".")
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            EmptyState {
                                width: parent.width
                                height: 100
                                visible: (drawer._detail.memberships || []).length === 0
                                iconName: "card_membership"
                                title: "No memberships"
                                description: "This user has no active plans."
                            }
                        }
                    }

                    // ----- Access management actions -----
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Access management"
                                subtitle: "Block / unblock / delete · change role"
                            }

                            // Role switcher
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: "Role:"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Repeater {
                                    model: ["user", "publisher", "admin", "server"]
                                    FilterChip {
                                        label: modelData
                                        iconName: (drawer._detail.role || "user") === modelData ? "check" : ""
                                        onClicked: {
                                            if (drawer.viewModel && typeof drawer.viewModel.setUserRole === "function") {
                                                drawer.viewModel.setUserRole(drawer.username, modelData)
                                                drawer.toastRequested("info", "Role updated",
                                                                      "@" + drawer.username + " is now a " + modelData + ".")
                                            }
                                        }
                                    }
                                }
                            }

                            // Action buttons
                            Row {
                                width: parent.width
                                spacing: Theme.space.md

                                PrimaryButton {
                                    text: drawer._detail.status === "Active" ? "Block user" : "Unblock user"
                                    iconName: drawer._detail.status === "Active" ? "lock" : "lock_open"
                                    enabled: drawer.username.length > 0
                                    onClicked: {
                                        if (!drawer.viewModel) return
                                        if (drawer._detail.status === "Active") {
                                            drawer.viewModel.blockUser(drawer.username)
                                            drawer.toastRequested("warning", "User blocked",
                                                                  "@" + drawer.username + " can no longer sign in.")
                                        } else {
                                            drawer.viewModel.unblockUser(drawer.username)
                                            drawer.toastRequested("success", "User unblocked",
                                                                  "@" + drawer.username + " access restored.")
                                        }
                                    }
                                }
                                SecondaryButton {
                                    text: "Delete user"
                                    iconName: "delete"
                                    enabled: drawer.username.length > 0
                                    onClicked: {
                                        _confirmDelete.open()
                                    }
                                }
                            }
                        }
                    }

                    // Bottom spacer
                    Item { width: 1; height: Theme.space.xl }
                }
            }
        }
    }

    // ----- Delete confirmation dialog -----
    Dialog {
        id: _confirmDelete
        anchors.centerIn: parent
        modal: true
        title: "Delete user?"
        width: 360
        standardButtons: Dialog.Cancel | Dialog.Ok

        contentItem: Column {
            spacing: Theme.space.sm
            Text {
                width: parent.width
                text: "Permanently delete @" + drawer.username + "?"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: "This action cannot be undone. The user's account, login history, and memberships will all be removed."
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                wrapMode: Text.WordWrap
            }
        }

        onAccepted: {
            if (drawer.viewModel && typeof drawer.viewModel.deleteUser === "function") {
                drawer.viewModel.deleteUser(drawer.username)
                drawer.toastRequested("warning", "User deleted",
                                      "@" + drawer.username + " has been permanently removed.")
            }
            drawer.close()
        }
    }
}
