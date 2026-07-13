// =============================================================================
//  AdminPublishersPage.qml
// =============================================================================
//  Publisher management for the admin role. Three KPI cards up top, a pending-
//  approvals list (Approve / Reject / View catalog actions), and an active-
//  publishers table (Name | Catalog | Revenue 30d | Status | Actions).
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `pendingPublishers` and `activePublishers` (QVariantList) plus
//  `approvePublisher(username)` / `rejectPublisher(username)`. We mirror the
//  VM lists into local ListModels (`_pending` / `_active`) so the existing
//  table layout / delegates continue to work without modification.
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

    // ----- KPI cards — values bound to the AdminViewModel -----
    readonly property var _kpis: [
        { icon: "hourglass_empty", value: (page.viewModel && page.viewModel.pendingPublishers ? page.viewModel.pendingPublishers.length : 0).toString(),                                     label: "Pending approvals", delta: "+2 today",          deltaUp: true,  accent: Theme.color.warning },
        { icon: "business",        value: (page.viewModel && page.viewModel.activePublishers ? page.viewModel.activePublishers.length : 0).toString(),                                       label: "Active publishers", delta: "+2 this week",      deltaUp: true,  accent: Theme.color.accent  },
        { icon: "attach_money",    value: "$" + page._totalRevenue().toLocaleString(Qt.locale(), "f", 0),                                                                                  label: "Revenue share (mo)", delta: "+8.1% vs last mo", deltaUp: true,  accent: Theme.color.success }
    ]

    // ----- Sum the 30-day revenue across all active publishers for the
    //       third KPI. Returns 0 if no VM / no data. -----
    function _totalRevenue() {
        if (!page.viewModel || !page.viewModel.activePublishers) return 0
        const list = page.viewModel.activePublishers
        let total = 0
        for (let i = 0; i < list.length; ++i) {
            const r = list[i].revenue
            if (typeof r === "number") total += r
            else if (typeof r === "string") {
                const n = parseFloat(r.replace(/[^0-9.]/g, ""))
                if (!isNaN(n)) total += n
            }
        }
        return total
    }

    // ----- Local mirrors of the VM's pending + active publisher lists -----
    ListModel { id: _pending }
    ListModel { id: _active }

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshPendingFromVM() {
        if (!page.viewModel) return
        _pending.clear()
        const list = page.viewModel.pendingPublishers || []
        for (let i = 0; i < list.length; ++i) {
            const p = list[i]
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
            // revenue may arrive as a number (8420) or formatted string ("$8,420")
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
        // Both local ListModels (_pending + _active) derive from the VM's
        // pendingPublishers / activePublishers properties, which both NOTIFY
        // on publishersChanged. Previously this block had TWO
        // onPublishersChanged handlers — QML only attaches one, so the
        // second refresh was silently dropped. Fixed by calling both
        // refreshes from a single handler.
        onPublishersChanged: {
            page._refreshPendingFromVM()
            page._refreshActiveFromVM()
        }
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshPendingFromVM()
            page._refreshActiveFromVM()
            if (typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
        }
    }

    readonly property real _colName:    260
    readonly property real _colCatalog: 140
    readonly property real _colRevenue: 160
    readonly property real _colStatus:  140

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                Repeater {
                    model: page._kpis
                    StatCard {
                        width: (parent.width - 2 * Theme.space.lg) / 3
                        iconName: modelData.icon
                        value:    modelData.value
                        label:    modelData.label
                        delta:    modelData.delta
                        deltaUp:  modelData.deltaUp
                        accent:   modelData.accent
                    }
                }
            }

            // ----- Pending approvals card -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Pending publisher approvals"
                        subtitle: "Review catalog size and requested date"
                    }

                    ListView {
                        width: parent.width
                        height: Math.max(0, _pending.count) * 84
                        clip: true
                        interactive: false
                        model: _pending
                        spacing: Theme.space.sm

                        delegate: Rectangle {
                            width: parent.width
                            height: 76
                            radius: Theme.radius.md
                            color: _rowHover1.hovered ? Theme.color.sidebarItemHover : Theme.color.fieldFilled
                            border.color: Theme.color.divider
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover1
                                cursorShape: Qt.PointingHandCursor
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.lg
                                anchors.rightMargin: Theme.space.lg
                                spacing: Theme.space.md

                                // Avatar + name + requested
                                Row {
                                    width: page._colName
                                    spacing: Theme.space.md
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        width: 40; height: 40; radius: 12
                                        color: model.avatarColor
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.initials
                                            color: Theme.color.textOnAccent
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                    Column {
                                        spacing: 2
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            text: model.name
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightSemibold
                                            elide: Text.ElideRight
                                            width: page._colName - 40 - Theme.space.md
                                        }
                                        Text {
                                            text: "Requested " + model.requested
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                }

                                // Catalog size
                                Column {
                                    width: 160
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: model.catalog + " titles"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                    }
                                    Text {
                                        text: "Catalog size"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                // Actions
                                Row {
                                    spacing: Theme.space.sm
                                    anchors.verticalCenter: parent.verticalCenter
                                    TextButton {
                                        text: "View catalog"
                                        iconName: "library_books"
                                        anchors.verticalCenter: parent.verticalCenter
                                        onClicked: page.toastRequested("info", "View catalog",
                                                                        "Opening catalog preview for " + model.name + ".")
                                    }
                                    SecondaryButton {
                                        text: "Reject"
                                        anchors.verticalCenter: parent.verticalCenter
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.rejectPublisher === "function") {
                                                page.viewModel.rejectPublisher(model.username)
                                                page.toastRequested("warning", "Rejected",
                                                                    "Approval request from " + model.name + " has been rejected.")
                                            } else {
                                                page.toastRequested("error", "No view model",
                                                                    "AdminViewModel is not available.")
                                            }
                                        }
                                    }
                                    PrimaryButton {
                                        text: "Approve"
                                        iconName: "check"
                                        anchors.verticalCenter: parent.verticalCenter
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.approvePublisher === "function") {
                                                page.viewModel.approvePublisher(model.username)
                                                page.toastRequested("success", "Approved",
                                                                    model.name + " is now a publisher on BookClub.")
                                            } else {
                                                page.toastRequested("error", "No view model",
                                                                    "AdminViewModel is not available.")
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
            Card {
                width: parent.width
                padding: 0

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // Header
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: Theme.color.fieldFilled

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Text { width: page._colName;    text: "Name";    color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colCatalog; text: "Catalog"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colRevenue; text: "Revenue (30d)"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colStatus;  text: "Status";  color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            Text { width: 96; text: "Actions"; horizontalAlignment: Text.AlignRight; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.color.divider
                        }
                    }

                    // Body
                    ListView {
                        width: parent.width
                        height: Math.max(0, _active.count) * 60
                        clip: true
                        interactive: false
                        model: _active
                        spacing: 0

                        delegate: Rectangle {
                            width: parent.width
                            height: 60
                            color: _rowHover2.hovered ? Theme.color.fieldFilled
                                 : (index % 2 === 0 ? "transparent" : Theme.color.fieldFilled)

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover2
                                cursorShape: Qt.PointingHandCursor
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

                                // Name + avatar
                                Row {
                                    width: page._colName
                                    spacing: Theme.space.md
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        width: 32; height: 32; radius: 8
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
                                        text: model.name
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                        width: page._colName - 32 - Theme.space.md
                                    }
                                }

                                Text { width: page._colCatalog; text: model.catalog + " titles"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colRevenue; text: model.revenue; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightSemibold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

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

                                // Actions
                                Row {
                                    width: 96
                                    spacing: Theme.space.xs
                                    layoutDirection: Qt.RightToLeft
                                    anchors.verticalCenter: parent.verticalCenter

                                }
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
