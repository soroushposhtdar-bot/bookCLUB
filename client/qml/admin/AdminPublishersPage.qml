// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Added fallback Connections handlers (onRefreshed, onDataChanged, onDataReady)
//   - Added _delayedRefresh Timer to re-sync after async VM.refresh() completes
//   - Defensive null/undefined checks in _refreshPendingFromVM / _refreshActiveFromVM
//
// LAYOUT FIXES:
//   - BUG A: Added EmptyState for pending approvals when _pending.count === 0
//   - BUG B: Active publishers ListView fills Card height; EmptyState when empty
//   - BUG C: Page header icon changed from "business" → "store" (broken glyph)
//   - LAYOUT FIX (this pass): replaced ScrollView > ColumnLayout with a plain
//     Column that anchors.fill: parent — same pattern as AdminUsersPage.
//     The ScrollView + ColumnLayout combo collapses to zero height in Qt 6
//     because ColumnLayout.implicitHeight is 0 on the first paint pass,
//     making the entire page invisible until the window is resized.

// =============================================================================
//  AdminPublishersPage.qml
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

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    readonly property var _kpis: [
        { icon: "hourglass_empty", value: (page.viewModel && page.viewModel.pendingPublishers ? page.viewModel.pendingPublishers.length : 0).toString(), label: "Pending approvals", delta: "live", deltaUp: true,  accent: Theme.color.warning },
        { icon: "business",        value: (page.viewModel && page.viewModel.activePublishers  ? page.viewModel.activePublishers.length  : 0).toString(), label: "Active publishers", delta: "live", deltaUp: true,  accent: Theme.color.accent  },
        { icon: "attach_money",    value: "$" + page._totalRevenue().toLocaleString(Qt.locale(), "f", 0),                                               label: "Revenue share (mo)", delta: "live", deltaUp: true,  accent: Theme.color.success }
    ]

    function _totalRevenue() {
        if (!page.viewModel || !page.viewModel.activePublishers) return 0
        const list = page.viewModel.activePublishers
        let total = 0
        for (let i = 0; i < list.length; ++i) {
            const r = list[i].revenue
            if (typeof r === "number") total += r
            else if (typeof r === "string") { const n = parseFloat(r.replace(/[^0-9.]/g, "")); if (!isNaN(n)) total += n }
        }
        return total
    }

    ListModel { id: _pending }
    ListModel { id: _active }

    function _refreshPendingFromVM() {
        if (!page.viewModel) return
        _pending.clear()
        const list = page.viewModel.pendingPublishers || []
        for (let i = 0; i < list.length; ++i) {
            const p = list[i]
            if (!p) continue
            _pending.append({
                username:    p.username    || p.name || "",
                name:        p.name        || p.displayName || p.username || "",
                requested:   p.requested   || p.requestedDate || "",
                catalog:     p.catalog     || p.catalogSize || 0,
                initials:    p.initials    || "",
                avatarColor: p.avatarColor || Theme.color.accent
            })
        }
    }

    function _refreshActiveFromVM() {
        if (!page.viewModel) return
        _active.clear()
        const list = page.viewModel.activePublishers || []
        for (let i = 0; i < list.length; ++i) {
            const p = list[i]
            if (!p) continue
            const rawRevenue = p.revenue !== undefined ? p.revenue : 0
            const revenueText = (typeof rawRevenue === "number")
                                ? "$" + rawRevenue.toLocaleString(Qt.locale(), "f", 0)
                                : String(rawRevenue)
            _active.append({
                username:    p.username    || p.name || "",
                name:        p.name        || p.displayName || p.username || "",
                catalog:     p.catalog     || p.catalogSize || 0,
                revenue:     revenueText,
                status:      p.status      || "Active",
                initials:    p.initials    || "",
                avatarColor: p.avatarColor || Theme.color.accent
            })
        }
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onPublishersChanged() { page._refreshPendingFromVM(); page._refreshActiveFromVM() }
        function onRefreshed()         { page._refreshPendingFromVM(); page._refreshActiveFromVM() }
        function onDataChanged()       { page._refreshPendingFromVM(); page._refreshActiveFromVM() }
        function onDataReady()         { page._refreshPendingFromVM(); page._refreshActiveFromVM() }
    }

    Timer {
        id: _delayedRefresh
        interval: 700
        repeat: true
        running: false
        onTriggered: {
            if (!page.viewModel) { _delayedRefresh.stop(); return }
            var hasPending = page.viewModel.pendingPublishers && page.viewModel.pendingPublishers.length > 0
            var hasActive  = page.viewModel.activePublishers  && page.viewModel.activePublishers.length > 0
            if (hasPending || hasActive) {
                page._refreshPendingFromVM(); page._refreshActiveFromVM(); _delayedRefresh.stop()
            } else {
                if (typeof page.viewModel.refresh === "function") page.viewModel.refresh()
            }
        }
    }

    onVisibleChanged: {
        if (page.visible && page.viewModel) {
            page._refreshPendingFromVM()
            page._refreshActiveFromVM()
        }
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshPendingFromVM()
            page._refreshActiveFromVM()
            if (typeof page.viewModel.refresh === "function") page.viewModel.refresh()
            _delayedRefresh.start()
        }
    }

    readonly property real _colName:    260
    readonly property real _colCatalog: 140
    readonly property real _colRevenue: 160
    readonly property real _colStatus:  140

    // -------------------------------------------------------------------------
    //  LAYOUT FIX: plain Column with anchors.fill, same as AdminUsersPage.
    //  The ScrollView > ColumnLayout combo produced a zero-height viewport in
    //  Qt 6 until a window resize event triggered a re-layout.
    // -------------------------------------------------------------------------
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
                    AppIcon { anchors.centerIn: parent; name: "store"; size: 22; color: Theme.color.accent }
                }
                Column {
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter
                    Text { text: "Publishers"; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold }
                    Text { text: "Approvals and publisher accounts"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                }
            }
        }

        // ----- KPI cards row -----
        Row {
            width: _mainColumn.width
            spacing: Theme.space.lg

            Repeater {
                model: page._kpis
                StatCard {
                    width: (_mainColumn.width - Theme.space.lg * (page._kpis.length - 1)) / page._kpis.length
                    height: Theme.size.kpiCardHeight
                    iconName: modelData.icon; value: modelData.value; label: modelData.label
                    delta: modelData.delta; deltaUp: modelData.deltaUp; accent: modelData.accent
                }
            }
        }

        // ----- Pending approvals card -----
        // Height: clamp between a min and enough rows. If the list is taller
        // than 50% of the remaining space, cap it so the active table is visible too.
        Card {
            width: _mainColumn.width
            height: _pending.count === 0
                    ? 220    // EmptyState minimum
                    : Math.min(Math.max(220, _pending.count * 84 + 80),
                               (_mainColumn.height - 56 - Theme.size.kpiCardHeight - Theme.space.lg * 3) * 0.45)
            padding: Theme.space.xl

            Column {
                width: parent.width
                spacing: Theme.space.md

                SectionHeader {
                    width: parent.width
                    title: "Pending publisher approvals"
                    subtitle: "Review catalog size and requested date"
                }

                // BUG A — EmptyState when no pending approvals
                EmptyState {
                    width: parent.width
                    height: _pending.count === 0 ? 120 : 0
                    visible: _pending.count === 0
                    iconName: "hourglass_empty"
                    title: "No pending approvals"
                    description: "New publisher applications will appear here."
                }

                ListView {
                    id: _pendingListView
                    width: parent.width
                    height: _pending.count > 0 ? Math.min(_pending.count * 84, parent.height - 80) : 0
                    visible: _pending.count > 0
                    clip: true
                    interactive: true
                    model: _pending
                    spacing: Theme.space.sm

                    delegate: Rectangle {
                        width: _pendingListView.width
                        height: 76
                        radius: Theme.radius.md
                        color: _rowHover1.hovered ? Theme.color.sidebarItemHover : Theme.color.fieldFilled
                        border.color: Theme.color.divider; border.width: 1

                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                        HoverHandler { id: _rowHover1; cursorShape: Qt.PointingHandCursor }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.lg
                            anchors.rightMargin: Theme.space.lg
                            spacing: Theme.space.md

                            Row {
                                width: page._colName
                                spacing: Theme.space.md
                                anchors.verticalCenter: parent.verticalCenter

                                Rectangle {
                                    width: 40; height: 40; radius: 12; color: model.avatarColor; anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: model.initials; color: Theme.color.textOnAccent; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold }
                                }
                                Column {
                                    spacing: 2; anchors.verticalCenter: parent.verticalCenter
                                    Text { text: model.name; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightSemibold; elide: Text.ElideRight; width: page._colName - 40 - Theme.space.md }
                                    Text { text: "Requested " + model.requested; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                                }
                            }

                            Column {
                                width: 160; spacing: 2; anchors.verticalCenter: parent.verticalCenter
                                Text { text: model.catalog + " titles"; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold }
                                Text { text: "Catalog size"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                            }

                            Item { width: 1; height: 1 }

                            Row {
                                spacing: Theme.space.sm; anchors.verticalCenter: parent.verticalCenter
                                TextButton {
                                    text: "View catalog"; iconName: "library_books"; anchors.verticalCenter: parent.verticalCenter
                                    onClicked: page.toastRequested("info", "View catalog", "Opening catalog preview for " + model.name + ".")
                                }
                                SecondaryButton {
                                    text: "Reject"; anchors.verticalCenter: parent.verticalCenter
                                    onClicked: {
                                        if (page.viewModel && typeof page.viewModel.rejectPublisher === "function") {
                                            page.viewModel.rejectPublisher(model.username)
                                            page.toastRequested("warning", "Rejected", "Approval request from " + model.name + " has been rejected.")
                                        } else {
                                            page.toastRequested("error", "No view model", "AdminViewModel is not available.")
                                        }
                                    }
                                }
                                PrimaryButton {
                                    text: "Approve"; iconName: "check"; anchors.verticalCenter: parent.verticalCenter
                                    onClicked: {
                                        if (page.viewModel && typeof page.viewModel.approvePublisher === "function") {
                                            page.viewModel.approvePublisher(model.username)
                                            page.toastRequested("success", "Approved", model.name + " is now a publisher on BookClub.")
                                        } else {
                                            page.toastRequested("error", "No view model", "AdminViewModel is not available.")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ----- Active publishers table -----
        // LAYOUT FIX: Card fills the remaining space so the table is always visible.
        Card {
            width: _mainColumn.width
            // Fill everything below: header(56) + KPI(kpiCardHeight) + pending card + gaps
            height: _mainColumn.height
                    - 56
                    - Theme.size.kpiCardHeight
                    - (_pending.count === 0
                       ? 220
                       : Math.min(Math.max(220, _pending.count * 84 + 80),
                                  (_mainColumn.height - 56 - Theme.size.kpiCardHeight - Theme.space.lg * 3) * 0.45))
                    - Theme.space.lg * 3
                    - Theme.space.xl * 2
            padding: 0

            Column {
                anchors.fill: parent
                spacing: 0

                // Header
                Rectangle {
                    id: _activeTableHeader
                    width: parent.width
                    height: 44
                    color: Theme.color.fieldFilled

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.xl
                        anchors.rightMargin: Theme.space.xl
                        spacing: 0

                        Text { width: page._colName;    text: "Name";         color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colCatalog; text: "Catalog";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colRevenue; text: "Revenue (30d)";color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colStatus;  text: "Status";       color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                    }

                    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }
                }

                // Body
                EmptyState {
                    width: parent.width
                    height: _active.count === 0 ? (parent.height - _activeTableHeader.height) : 0
                    visible: _active.count === 0
                    iconName: "business"
                    title: "No active publishers"
                    description: "Approved publishers will appear in this table."
                }

                ListView {
                    id: _activeListView
                    width: parent.width
                    height: _active.count > 0 ? (parent.height - _activeTableHeader.height) : 0
                    visible: _active.count > 0
                    clip: true
                    interactive: true
                    model: _active
                    spacing: 0

                    delegate: Rectangle {
                        width: _activeListView.width
                        height: Theme.size.tableRowHeight
                        color: _rowHover2.hovered ? Theme.color.fieldFilled
                             : (index % 2 === 0 ? "transparent" : Theme.color.fieldFilled)

                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                        HoverHandler { id: _rowHover2; cursorShape: Qt.PointingHandCursor }

                        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Row {
                                width: page._colName
                                spacing: Theme.space.md
                                anchors.verticalCenter: parent.verticalCenter

                                Rectangle {
                                    width: 32; height: 32; radius: 8; color: model.avatarColor; anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: model.initials; color: Theme.color.textOnAccent; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold }
                                }
                                Text {
                                    text: model.name; color: Theme.color.textPrimary; font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight
                                    width: page._colName - 32 - Theme.space.md
                                }
                            }

                            Text { width: page._colCatalog; text: model.catalog + " titles"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colRevenue; text: model.revenue; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightSemibold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

                            Item {
                                width: page._colStatus
                                height: parent.height
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: Theme.space.xs
                                    Rectangle { width: 8; height: 8; radius: 4; color: model.status === "Active" ? Theme.color.success : Theme.color.error; anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: model.status; color: model.status === "Active" ? Theme.color.success : Theme.color.error; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; anchors.verticalCenter: parent.verticalCenter }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
