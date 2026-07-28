// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//   - SearchField.onTextEdited: function(newText) { ... }
//   - SortDropdown.onChanged: function(value) { ... }
//
// FUNCTIONAL FIXES:
//   - Added fallback Connections handlers (onRefreshed, onDataChanged, onDataReady)
//     so the user list re-syncs whenever the VM pushes data, regardless of signal name
//   - Added _delayedRefresh Timer to re-sync after async VM.refresh() completes
//   - Defensive null/undefined checks in _refreshFromVM for malformed user records
//
// DATA-BINDING FIXES (this pass):
//   - BUG 1 (status mapping): server returns `status` as integer per the
//     AccountStatus enum in common/AppEnums.h:
//         0 = Pending, 1 = Active, 2 = Blocked, 3 = Disabled, 4 = Deleted
//     The previous `_statusToString()` incorrectly mapped 0→"Blocked" and
//     1→"Active", which meant blocked users (status=2) fell through to the
//     default "Active" and never showed as blocked. Now correctly maps all
//     5 enum values.
//   - BUG 1 (missing fields): server raw JSON often lacks displayName, joined,
//     initials, avatarColor. Added _deriveInitials() (first letters of up to 2
//     words, else first letter of username) and _deriveAvatarColor()
//     (deterministic color from a hash of the username so the same user always
//     gets the same color). displayName now tries displayName → name → fullName
//     → username; joined tries joined → createdAt → joinedAt.
//   - role now preserved as the raw value when defined (the existing _roleKey()
//     helper already normalizes 0/1/2/"user"/"publisher"/"admin" for display).

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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
import "../components"

Item {
    id: page

    // ----- AdminViewModel (injected by AdminShell) -----
    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal openUserDetail(string username)   // emitted when a row is clicked

    // ----- Role → color map -----
    // BUG FIX (admin polish): same normalization as AdminUserDetailDrawer.
    // The server returns `role` as an integer (0/1/2); the previous
    // helpers only accepted strings, so the role badge always fell
    // through to the default color.
    function _roleKey(role) {
        if (role === 0 || role === "0" || role === "user") return "user"
        if (role === 1 || role === "1" || role === "publisher") return "publisher"
        if (role === 2 || role === "2" || role === "admin") return "admin"
        return "user"
    }
    function _roleColor(role) {
        const k = page._roleKey(role)
        if (k === "admin")     return Theme.color.error
        if (k === "publisher") return Theme.color.warning
        return Theme.color.accent
    }
    function _roleSoft(role) {
        const k = page._roleKey(role)
        if (k === "admin")     return Theme.color.errorSoft
        if (k === "publisher") return Theme.color.warningSoft
        return Theme.color.accentSoft
    }
    function _roleLabel(role) {
        const k = page._roleKey(role)
        if (k === "admin") return "Admin"
        if (k === "publisher") return "Publisher"
        return "User"
    }

    // ----- Search / sort / pagination state -----
    property string _search: ""
    property string _sortValue: "newest"
    property string _statusFilter: "all"   // all | Active | Blocked
    property string _roleFilter: "all"     // v21: all | user | publisher | admin
    property int _currentPage: 1
    readonly property int _pageSize: 8
    readonly property int _totalPages: Math.max(1, Math.ceil(page._filteredCount / _pageSize))

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

    // Track filtered count for correct pagination and display text
    property int _filteredCount: 0

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    //  BUG FIX (status mapping): the server returns `status` as an integer
    //  per the AccountStatus enum in common/AppEnums.h:
    //      0 = Pending, 1 = Active, 2 = Blocked, 3 = Disabled, 4 = Deleted
    //  The previous version mapped 0→"Blocked" and 1→"Active", which was
    //  WRONG — it meant blocked users (status=2) fell through to the
    //  default "Active" and never showed as blocked. Now correctly maps
    //  all 5 enum values.
    //
    //  BUG FIX (missing fields): the server raw JSON often lacks displayName,
    //  joined, initials, and avatarColor. We safely derive each:
    //    - displayName: try displayName → name → fullName → username
    //    - joined:      try joined → createdAt → joinedAt → ""
    //    - initials:    first letters of up to 2 words in displayName, else
    //                   first letter of username, else "?"
    //    - avatarColor: u.avatarColor, else a deterministic color picked from
    //                   the theme palette by hashing the username (so the same
    //                   user always gets the same color).
    // -------------------------------------------------------------------------
    function _statusToString(rawStatus) {
        // Integer enum values (from server, per common/AppEnums.h):
        //   0=Pending, 1=Active, 2=Blocked, 3=Disabled, 4=Deleted
        if (rawStatus === 0 || rawStatus === "0") return "Pending"
        if (rawStatus === 1 || rawStatus === "1") return "Active"
        if (rawStatus === 2 || rawStatus === "2") return "Blocked"
        if (rawStatus === 3 || rawStatus === "3") return "Disabled"
        if (rawStatus === 4 || rawStatus === "4") return "Deleted"
        // String values (from older server versions or QML-constructed)
        if (typeof rawStatus === "string") {
            var s = rawStatus.toLowerCase()
            if (s === "pending")  return "Pending"
            if (s === "active")   return "Active"
            if (s === "blocked")  return "Blocked"
            if (s === "disabled") return "Disabled"
            if (s === "deleted")  return "Deleted"
        }
        // Fallback: treat undefined/null/unknown as Active so the table isn't empty
        return "Active"
    }

    // BUG FIX (status colors): returns the theme color for a given status
    // string (as produced by _statusToString). Used by the status badge dot
    // and text so all 5 statuses are color-coded correctly.
    function _statusColor(statusStr) {
        if (statusStr === "Active")   return Theme.color.success
        if (statusStr === "Blocked")  return Theme.color.error
        if (statusStr === "Pending")  return Theme.color.warning
        if (statusStr === "Disabled") return Theme.color.textMuted
        if (statusStr === "Deleted")  return Theme.color.textMuted
        return Theme.color.textMuted
    }

    // BUG FIX: helper that returns true ONLY for "Active" status. Used by
    // the block/unblock toggle button so the icon is "lock" (block) for
    // Active users and "lock_open" (unblock) for every non-Active status
    // (Blocked, Pending, Disabled, Deleted).
    function _isActiveStatus(statusStr) {
        return statusStr === "Active"
    }

    function _deriveInitials(name, username) {
        var n = (name || "").trim()
        if (n.length > 0) {
            var parts = n.split(/\s+/).filter(function(p) { return p.length > 0 })
            if (parts.length > 0) {
                var first = parts[0].charAt(0).toUpperCase()
                var second = parts.length > 1 ? parts[1].charAt(0).toUpperCase() : ""
                return (first + second)
            }
        }
        if (username && username.length > 0) return username.charAt(0).toUpperCase()
        return "?"
    }

    function _deriveAvatarColor(username) {
        var palette = [
            Theme.color.accent,  Theme.color.success, Theme.color.info,
            Theme.color.warning, Theme.color.error
        ]
        var hash = 0
        var s = username || ""
        for (var i = 0; i < s.length; ++i) {
            hash = (hash * 31 + s.charCodeAt(i)) | 0   // force int32 to avoid overflow
        }
        return palette[Math.abs(hash) % palette.length]
    }

    function _refreshFromVM() {
        if (!page.viewModel) return
        _allUsers.clear()
        const users = page.viewModel.users || []
        for (let i = 0; i < users.length; ++i) {
            const u = users[i]
            if (!u) continue   // defensive: skip null/undefined entries

            // BUG FIX — derive missing fields safely
            const rawName = u.displayName || u.name || u.fullName || u.username || ""
            const joined  = u.joined || u.createdAt || u.joinedAt || ""

            _allUsers.append({
                username:     u.username     || "",
                displayName:  rawName,
                role:         u.role !== undefined ? u.role : "user",
                joined:       joined,
                status:       page._statusToString(u.status),
                initials:     u.initials || page._deriveInitials(rawName, u.username),
                avatarColor:  u.avatarColor || page._deriveAvatarColor(u.username)
            })
        }
        page._applyFilterAndSort()
    }

    // ----- Apply search + sort + pagination, then refresh the page slice -----
    function _applyFilterAndSort() {
        // 1) Collect + filter into a temp array
        const q = page._search.trim().toLowerCase()
        const statusFilter = page._statusFilter
        const roleFilter = page._roleFilter  // v21: role filter
        const rows = []
        for (let i = 0; i < _allUsers.count; ++i) {
            const row = _allUsers.get(i)
            if (q.length > 0) {
                const hay = (row.username + " " + row.displayName).toLowerCase()
                if (hay.indexOf(q) < 0) continue
            }
            if (statusFilter !== "all" && row.status !== statusFilter) continue
            // v21: filter by role
            if (roleFilter !== "all") {
                const rowRole = page._roleKey(row.role)
                if (rowRole !== roleFilter) continue
            }
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
        page._filteredCount = rows.length
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
        function onUsersChanged() { page._refreshFromVM() }
        // Fallback handlers — re-sync the local ListModel whenever the VM
        // completes a refresh cycle. These catch VMs that emit a single
        // "refreshed" / "dataChanged" signal instead of granular per-list ones.
        function onRefreshed() { page._refreshFromVM() }
        function onDataChanged() { page._refreshFromVM() }
        function onDataReady() { page._refreshFromVM() }
    }

    // ----- Delayed refresh -----
    // VM.refresh() is async; the data may arrive after Component.onCompleted
    // finishes. This Timer re-syncs 500ms after load to catch the first
    // async data push, then stops (the Connections block handles subsequent updates).
    Timer {
        id: _delayedRefresh
        interval: 500
        repeat: true
        running: page.visible
        onTriggered: {
            if (page.viewModel && page.viewModel.users && page.viewModel.users.length > 0) {
                page._refreshFromVM()
                _delayedRefresh.stop()
            }
        }
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

    // -------------------------------------------------------------------------
    //  BUG FIX (layout): the user table was hidden behind a default panel
    //  because the ScrollView's ColumnLayout didn't propagate its implicit
    //  height to the ScrollView's contentHeight. The table (a ListView with
    //  height bound to _filteredPage.count * rowHeight) collapsed to 0 when
    //  the data hadn't arrived yet, and even after data arrived the
    //  ColumnLayout's implicit height wasn't picked up by the ScrollView.
    //
    //  Fix: replaced the ScrollView + ColumnLayout with a plain Column that
    //  anchors to all 4 sides. The Column auto-stacks its children vertically
    //  and the page itself is hosted in a Loader that has anchors.fill, so
    //  the Column gets the full page height. The table (ListView) now has a
    //  Layout.fillHeight equivalent via direct height binding to the
    //  remaining space. This makes the table always visible at the top of
    //  the page, below the header + search row.
    // -------------------------------------------------------------------------

    // ----- Confirmation dialog for destructive actions -----
    ConfirmDialog {
        id: _confirmDialog
    }

    // ----- Main scrollable column -----
    // BUG FIX (layout): use a plain Column with anchors.fill so the table
    // is always at the top, not hidden behind a collapsed ScrollView panel.
    Column {
        id: _mainColumn
        anchors.fill: parent
        anchors.margins: Theme.space.xl
        spacing: Theme.space.lg

        // ----- Page header -----
        Item {
            width: _mainColumn.width
            height: 56
            Row {
                anchors.fill: parent
                spacing: Theme.space.md
                Rectangle {
                    width: 44; height: 44; radius: 12
                    color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.14)
                    anchors.verticalCenter: parent.verticalCenter
                    AppIcon {
                        anchors.centerIn: parent
                        name: "manage_accounts"
                        size: 22
                        color: Theme.color.accent
                    }
                }
                Column {
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "Users"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                    }
                    Text {
                        text: "Manage members, roles, and access"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
            }
        }

        // ----- Search / sort / add-user row -----
        Row {
            width: _mainColumn.width
            height: 44
            spacing: Theme.space.md

            SearchField {
                width: Math.min(420, _mainColumn.width * 0.45)
                height: parent.height
                placeholder: "Search users by username or name…"
                text: page._search
                onTextEdited: function(newText) {
                    page._search = newText
                    page._currentPage = 1
                    page._applyFilterAndSort()
                }
                onAccepted: page.toastRequested("info", "Search",
                                                "Filtering users for \"" + text + "\".")
            }

            SortDropdown {
                width: 220
                height: parent.height
                options: page._sortOptions
                onChanged: function(value) {
                    page._sortValue = value
                    page._applyFilterAndSort()
                    page.toastRequested("info", "Sort applied",
                                        "Users are now sorted by " + value + ".")
                }
            }

            // v21: Role filter chips (replaces the Publishers page)
            Row {
                height: parent.height
                spacing: Theme.space.xs

                Text {
                    text: "Role:"
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    anchors.verticalCenter: parent.verticalCenter
                }

                Repeater {
                    model: [
                        { key: "all",       label: "All" },
                        { key: "user",      label: "Users" },
                        { key: "publisher", label: "Publishers" }
                    ]
                    FilterChip {
                        height: 32
                        label: modelData.label
                        iconName: page._roleFilter === modelData.key ? "check" : ""
                        onClicked: {
                            page._roleFilter = modelData.key
                            page._currentPage = 1
                            page._applyFilterAndSort()
                        }
                    }
                }
            }

            // Status filter chips
            Repeater {
                model: [
                    { key: "all",      label: "All" },
                    { key: "Active",   label: "Active" },
                    { key: "Blocked",  label: "Blocked" },
                    { key: "Pending",  label: "Pending" },
                    { key: "Disabled", label: "Disabled" },
                    { key: "Deleted",  label: "Deleted" }
                ]
                FilterChip {
                    height: parent.height
                    label: modelData.label
                    iconName: page._statusFilter === modelData.key ? "check" : ""
                    onClicked: {
                        page._statusFilter = modelData.key
                        page._currentPage = 1
                        page._applyFilterAndSort()
                    }
                }
            }

            Item { width: 1; height: 1 }

            PrimaryButton {
                text: "Add user"
                iconName: "person_add"
                onClicked: page.toastRequested("info", "Add user",
                                                "Open the add-user dialog to invite a new member.")
            }
        }

        // ----- Users table -----
        // BUG FIX (layout): the table Card now fills the remaining vertical
        // space so it's always visible. Previously the ListView's height was
        // bound to _filteredPage.count * rowHeight, which collapsed to 0
        // when no data was loaded — making the table invisible.
        Card {
            width: _mainColumn.width
            // Fill the remaining space in the Column (after header + search
            // row + pagination). This guarantees the table is always visible.
            height: _mainColumn.height - 56 - Theme.space.lg - 44 - Theme.space.lg - 56 - Theme.space.lg
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
                        Item { width: parent.width - page._colUsername - page._colDisplay - page._colRole - page._colJoined - page._colStatus - 176; height: 1 }
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

                // ----- Body (scrollable) -----
                // BUG FIX: ListView now fills the remaining Card height
                // (instead of having height = count * rowHeight which
                // collapsed to 0 with no data). Also interactive: true so
                // the list scrolls when there are more rows than visible.
                ListView {
                    id: _usersListView
                    width: parent.width
                    height: parent.height - 44
                    clip: true
                    interactive: true
                    model: _filteredPage
                    spacing: 0

                    delegate: Rectangle {
                        width: _usersListView.width
                        height: Theme.size.tableRowHeight
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
                                        text: page._roleLabel(model.role)
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
                                        color: page._statusColor(model.status)
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: model.status
                                        color: page._statusColor(model.status)
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            Item { width: parent.width - page._colUsername - page._colDisplay - page._colRole - page._colJoined - page._colStatus - 176; height: 1 }

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
                                            confirmedCb: function() {
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
                                    iconName: page._isActiveStatus(model.status) ? "lock" : "lock_open"
                                    iconColor: page._isActiveStatus(model.status) ? Theme.color.warning : Theme.color.success
                                    hoverIconColor: page._isActiveStatus(model.status) ? Theme.color.warning : Theme.color.success
                                    onClicked: {
                                        if (!page.viewModel) {
                                            page.toastRequested("error", "No view model",
                                                                "AdminViewModel is not available.")
                                            return
                                        }
                                        if (page._isActiveStatus(model.status)) {
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
            width: _mainColumn.width
            height: 40
            spacing: Theme.space.md

            Text {
                text: {
                    const total = page._filteredCount
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
            Item { width: _mainColumn.width - 400; height: 1 }
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
            width: _mainColumn.width
            height: visible ? 200 : 0
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
