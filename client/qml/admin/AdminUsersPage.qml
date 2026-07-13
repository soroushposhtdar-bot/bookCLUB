// =============================================================================
//  AdminUsersPage.qml
// =============================================================================
//  User management table for the admin role. Search + sort + add-user affordance
//  up top, a table of users (with avatar initials, role badge, status badge,
//  row actions: block / unblock / delete), pagination at the bottom, and an
//  empty state when the search returns nothing.
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `users` (QVariantList of { username, displayName, role, joined, status,
//  initials, avatarColor }) plus blockUser / unblockUser / deleteUser /
//  toggleUserStatus. We mirror the VM's user list into a local `_allUsers`
//  ListModel so we can apply search / sort filtering without round-tripping
//  through the VM on every keystroke. Whenever the VM's `users` property
//  changes we re-seed `_allUsers` and re-apply the active filter/sort.
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
import "../components/book"

import BookClub.Services 1.0

Item {
    id: page

    // ----- AdminViewModel (injected by AdminShell) -----
    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal openUserDetail(string username)   // emitted when a row is clicked

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

    // ----- Search / sort / pagination state -----
    property string _search: ""
    property string _sortValue: "newest"
    property string _statusFilter: "all"   // all | Active | Blocked
    property int _currentPage: 1
    readonly property int _pageSize: 8
    readonly property int _totalPages: Math.max(1, Math.ceil(_allUsers.count / _pageSize))

    // ----- Sort dropdown options -----
    readonly property var _sortOptions: [
        { label: "Newest first",     value: "newest" },
        { label: "Oldest first",     value: "oldest" },
        { label: "Username A→Z",     value: "username_asc" },
        { label: "Role",             value: "role" }
    ]

    // ----- Local mirrors of the VM's users -----
    //   _allUsers     — full set, no filtering (sorted in place)
    //   _filteredPage — the current page slice bound to the table
    ListModel { id: _allUsers }
    ListModel { id: _filteredPage }

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshFromVM() {
        if (!page.viewModel) return
        _allUsers.clear()
        const users = page.viewModel.users || []
        for (let i = 0; i < users.length; ++i) {
            const u = users[i]
            _allUsers.append({
                username:     u.username     || "",
                displayName:  u.displayName  || "",
                role:         u.role         || "user",
                joined:       u.joined       || "",
                status:       u.status       || "Active",
                initials:     u.initials     || "",
                avatarColor:  u.avatarColor  || Theme.color.accent
            })
        }
        page._applyFilterAndSort()
    }

    // ----- Apply search + sort + pagination, then refresh the page slice -----
    function _applyFilterAndSort() {
        // 1) Collect + filter into a temp array
        const q = page._search.trim().toLowerCase()
        const statusFilter = page._statusFilter
        const rows = []
        for (let i = 0; i < _allUsers.count; ++i) {
            const row = _allUsers.get(i)
            if (q.length > 0) {
                const hay = (row.username + " " + row.displayName).toLowerCase()
                if (hay.indexOf(q) < 0) continue
            }
            if (statusFilter !== "all" && row.status !== statusFilter) continue
            rows.push(row)
        }

        // 2) Sort the filtered set
        const sv = page._sortValue
        rows.sort(function(a, b) {
            if (sv === "username_asc") return a.username.localeCompare(b.username)
            if (sv === "role")         return a.role.localeCompare(b.role)
            // "newest" / "oldest" — VM order is assumed to be newest-first
            return 0
        })
        if (sv === "oldest") rows.reverse()

        // 3) Clamp current page to the new total
        const totalPages = Math.max(1, Math.ceil(rows.length / page._pageSize))
        if (page._currentPage > totalPages) page._currentPage = totalPages

        // 4) Rebuild the page slice
        _filteredPage.clear()
        const start = (page._currentPage - 1) * page._pageSize
        const end   = Math.min(start + page._pageSize, rows.length)
        for (let j = start; j < end; ++j) _filteredPage.append(rows[j])
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onUsersChanged: page._refreshFromVM()
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshFromVM()
            if (typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
        }
    }

    // ----- Column widths for the table -----
    readonly property real _colUsername: 180
    readonly property real _colDisplay:  200
    readonly property real _colRole:     140
    readonly property real _colJoined:   140
    readonly property real _colStatus:   140

    // ----- Confirmation dialog for destructive actions -----
    ConfirmDialog {
        id: _confirmDialog
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Search / sort / add-user row -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                SearchField {
                    width: Math.min(420, parent.width * 0.45)
                    placeholder: "Search users by username or name…"
                    text: page._search
                    onTextEdited: {
                        page._search = newText
                        page._currentPage = 1
                        page._applyFilterAndSort()
                    }
                    onAccepted: page.toastRequested("info", "Search",
                                                    "Filtering users for \"" + text + "\".")
                }

                SortDropdown {
                    width: 220
                    options: page._sortOptions
                    onChanged: {
                        page._sortValue = value
                        page._applyFilterAndSort()
                        page.toastRequested("info", "Sort applied",
                                            "Users are now sorted by " + value + ".")
                    }
                }

                // Status filter chips
                Repeater {
                    model: [
                        { key: "all",     label: "All" },
                        { key: "Active",  label: "Active" },
                        { key: "Blocked", label: "Blocked" }
                    ]
                    FilterChip {
                        label: modelData.label
                        iconName: page._statusFilter === modelData.key ? "check" : ""
                        onClicked: {
                            page._statusFilter = modelData.key
                            page._currentPage = 1
                            page._applyFilterAndSort()
                        }
                    }
                }

                Item { width: 1; Layout.fillWidth: true; height: 1 }

                PrimaryButton {
                    text: "Add user"
                    iconName: "person_add"
                    onClicked: page.toastRequested("info", "Add user",
                                                    "Open the add-user dialog to invite a new member.")
                }
            }

            // ----- Users table -----
            Card {
                width: parent.width
                padding: 0

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // ----- Header row -----
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: Theme.color.fieldFilled

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Text { width: page._colUsername; text: "Username";    color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colDisplay;  text: "Display name"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colRole;     text: "Role";        color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colJoined;   text: "Joined";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colStatus;   text: "Status";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            Text { width: 176; text: "Actions"; horizontalAlignment: Text.AlignRight; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.color.divider
                        }
                    }

                    // ----- Body -----
                    ListView {
                        width: parent.width
                        height: Math.max(0, _filteredPage.count) * 64
                        clip: true
                        interactive: false
                        model: _filteredPage
                        spacing: 0

                        delegate: Rectangle {
                            width: parent.width
                            height: 64
                            color: _rowHover1.hovered ? Theme.color.fieldFilled
                                 : (index % 2 === 0 ? "transparent" : Theme.color.fieldFilled)

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover1
                                cursorShape: Qt.PointingHandCursor
                            }

                            // Click anywhere on the row (except the action
                            // buttons) opens the user-detail drawer.
                            MouseArea {
                                anchors.fill: parent
                                onClicked: page.openUserDetail(model.username)
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: Theme.color.divider
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.xl
                                anchors.rightMargin: Theme.space.xl
                                spacing: 0

                                // Username + avatar
                                Row {
                                    width: page._colUsername
                                    spacing: Theme.space.md
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        width: 32; height: 32; radius: 16
                                        color: model.avatarColor
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.initials
                                            color: Theme.color.textOnAccent
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                    Text {
                                        text: model.username
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                    }
                                }

                                Text { width: page._colDisplay;  text: model.displayName; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

                                // Role badge
                                Item {
                                    width: page._colRole
                                    height: parent.height
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: _roleLabel.implicitWidth + 16
                                        height: 24
                                        radius: 12
                                        color: page._roleSoft(model.role)
                                        Text {
                                            id: _roleLabel
                                            anchors.centerIn: parent
                                            text: model.role
                                            color: page._roleColor(model.role)
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                            font.capitalization: Font.Capitalize
                                        }
                                    }
                                }

                                Text { width: page._colJoined;   text: model.joined; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

                                // Status badge
                                Item {
                                    width: page._colStatus
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: Theme.space.xs
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: model.status === "Active" ? Theme.color.success : Theme.color.error
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: model.status
                                            color: model.status === "Active" ? Theme.color.success : Theme.color.error
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                // ----- Row actions: block/unblock toggle, edit, delete -----
                                Row {
                                    width: 176
                                    spacing: Theme.space.xs
                                    layoutDirection: Qt.RightToLeft
                                    anchors.verticalCenter: parent.verticalCenter

                                    IconButton {
                                        // Delete (with confirmation)
                                        iconName: "delete"
                                        iconColor: Theme.color.error
                                        hoverIconColor: Theme.color.error
                                        onClicked: {
                                            _confirmDialog.openDialog({
                                                title: "Delete user?",
                                                message: "Permanently delete @" + model.username + " (" + model.displayName + ").",
                                                detail: "This action cannot be undone.",
                                                iconName: "delete_forever",
                                                confirmLabel: "Delete",
                                                confirmStyle: "danger",
                                                onConfirmed: function() {
                                                    if (page.viewModel && typeof page.viewModel.deleteUser === "function") {
                                                        page.viewModel.deleteUser(model.username)
                                                        page.toastRequested("success", "User deleted",
                                                                            "@" + model.username + " was removed.")
                                                    } else {
                                                        page.toastRequested("error", "No view model",
                                                                            "AdminViewModel is not available.")
                                                    }
                                                }
                                            })
                                        }
                                    }
                                    IconButton {
                                        iconName: "edit"
                                        onClicked: page.openUserDetail(model.username)
                                    }
                                    IconButton {
                                        // Block / Unblock toggle
                                        iconName: model.status === "Active" ? "lock" : "lock_open"
                                        iconColor: model.status === "Active" ? Theme.color.warning : Theme.color.success
                                        hoverIconColor: model.status === "Active" ? Theme.color.warning : Theme.color.success
                                        onClicked: {
                                            if (!page.viewModel) {
                                                page.toastRequested("error", "No view model",
                                                                    "AdminViewModel is not available.")
                                                return
                                            }
                                            if (model.status === "Active") {
                                                if (typeof page.viewModel.blockUser === "function") {
                                                    page.viewModel.blockUser(model.username)
                                                    page.toastRequested("warning", "User blocked",
                                                                        "@" + model.username + " has been blocked.")
                                                }
                                            } else {
                                                if (typeof page.viewModel.unblockUser === "function") {
                                                    page.viewModel.unblockUser(model.username)
                                                    page.toastRequested("success", "User unblocked",
                                                                        "@" + model.username + " is active again.")
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

            // ----- Pagination + count -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                Text {
                    text: {
                        const total = _allUsers.count
                        if (total === 0) return "No users"
                        const start = (page._currentPage - 1) * page._pageSize + 1
                        const end   = Math.min(page._currentPage * page._pageSize, total)
                        return "Showing " + start + "–" + end + " of " + total.toLocaleString(Qt.locale(), "f", 0) + " users"
                    }
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { width: 1; Layout.fillWidth: true; height: 1 }
                Pagination {
                    currentPage: page._currentPage
                    totalPages: page._totalPages
                    onPageRequested: function(pageNum) {
                        page._currentPage = pageNum
                        page._applyFilterAndSort()
                    }
                }
            }

            // ----- Empty state (shown when search returns nothing) -----
            Card {
                width: parent.width
                height: 200
                visible: page._search.length > 0 && _allUsers.count > 0 && _filteredPage.count === 0
                padding: Theme.space.xxl

                EmptyState {
                    anchors.fill: parent
                    iconName: "search_off"
                    title: "No users found"
                    description: "Try a different username or clear the search."
                    actionLabel: "Clear search"
                    onActionTriggered: {
                        page._search = ""
                        page._currentPage = 1
                        page._applyFilterAndSort()
                    }
                }
            }
        }
    }
}
