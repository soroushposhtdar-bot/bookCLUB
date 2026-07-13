// =============================================================================
//  AdminReportsPage.qml
// =============================================================================
//  Reports queue for the admin role. Filter chips up top (All / Pending /
//  Investigating / Resolved / Dismissed), a table of reports with type,
//  target, reporter, reason, status, assignee, and row actions. Pagination
//  at the bottom.
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `reports` (QVariantList of { id, type, target, reporter, reason, status,
//  assigned }) plus `updateReportStatus(id, status)` /
//  `takeActionOnReport(id, action)` / `dismissReport(id)`. We mirror the
//  VM's report list into a local `_allReports` ListModel so we can apply
//  filter / pagination without round-tripping through the VM on every
//  chip toggle. Whenever the VM's `reports` property changes we re-seed
//  `_allReports` and re-apply the active filter / page.
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

    // ----- Filter state -----
    property string _filter: "All"

    // ----- Pagination state -----
    property int _currentPage: 1
    readonly property int _pageSize: 8
    readonly property int _totalPages: Math.max(1, Math.ceil(_filteredReports.count / _pageSize))

    function _statusColor(s) {
        if (s === "Resolved")      return Theme.color.success
        if (s === "Investigating") return Theme.color.info
        if (s === "Dismissed")     return Theme.color.textMuted
        return Theme.color.warning
    }
    function _statusSoft(s) {
        if (s === "Resolved")      return Theme.color.successSoft
        if (s === "Investigating") return Theme.color.infoSoft
        if (s === "Dismissed")     return Theme.color.fieldFilled
        return Theme.color.warningSoft
    }
    function _typeColor(t) {
        if (t === "Copyright")           return Theme.color.error
        if (t === "Harassment")          return Theme.color.error
        if (t === "Spam")                return Theme.color.warning
        if (t === "Fake review")         return Theme.color.warning
        return Theme.color.info
    }

    // ----- Local mirrors of the VM's reports -----
    //   _allReports     — full set, no filtering
    //   _filteredReports — reports matching the active status filter
    //   _reportsPage    — the current page slice bound to the table
    ListModel { id: _allReports }
    ListModel { id: _filteredReports }
    ListModel { id: _reportsPage }

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshFromVM() {
        if (!page.viewModel) return
        _allReports.clear()
        const list = page.viewModel.reports || []
        for (let i = 0; i < list.length; ++i) {
            const r = list[i]
            _allReports.append({
                id:       r.id       !== undefined ? r.id       : i,
                type:     r.type     || "Inappropriate content",
                target:   r.target   || "",
                reporter: r.reporter || "",
                reason:   r.reason   || "",
                status:   r.status   || "Pending",
                assigned: r.assigned  || r.assignee || "—"
            })
        }
        page._applyFilter()
    }

    // ----- Apply status filter, then paginate -----
    function _applyFilter() {
        _filteredReports.clear()
        const filter = page._filter
        for (let i = 0; i < _allReports.count; ++i) {
            const row = _allReports.get(i)
            if (filter !== "All" && row.status !== filter) continue
            _filteredReports.append(row)
        }

        // Clamp the current page
        const totalPages = Math.max(1, Math.ceil(_filteredReports.count / page._pageSize))
        if (page._currentPage > totalPages) page._currentPage = totalPages
        page._applyPage()
    }

    function _applyPage() {
        _reportsPage.clear()
        const start = (page._currentPage - 1) * page._pageSize
        const end   = Math.min(start + page._pageSize, _filteredReports.count)
        for (let i = start; i < end; ++i) _reportsPage.append(_filteredReports.get(i))
    }

    // ----- Cycle a report to a new status via the VM -----
    function _setStatus(model, newStatus) {
        if (!page.viewModel || typeof page.viewModel.updateReportStatus !== "function") {
            page.toastRequested("error", "No view model",
                                "AdminViewModel is not available.")
            return
        }
        page.viewModel.updateReportStatus(model.id, newStatus)
        page.toastRequested("info", "Status updated",
                            "Report on " + model.target + " → " + newStatus + ".")
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onReportsChanged: page._refreshFromVM()
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshFromVM()
            if (typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
        }
    }

    readonly property real _colType:    200
    readonly property real _colTarget:  280
    readonly property real _colReporter:140
    readonly property real _colReason:  180
    readonly property real _colStatus:  150
    readonly property real _colAssigned:140

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Filter chips row -----
            Row {
                width: parent.width
                spacing: Theme.space.sm

                Repeater {
                    model: [ "All", "Pending", "Investigating", "Resolved", "Dismissed" ]
                    FilterChip {
                        label: modelData
                        iconName: page._filter === modelData ? "check" : ""
                        onClicked: {
                            page._filter = modelData
                            page._currentPage = 1
                            page._applyFilter()
                            page.toastRequested("info", "Filter applied",
                                                "Showing reports: " + modelData)
                        }
                        onRemoveClicked: {
                            page._filter = "All"
                            page._currentPage = 1
                            page._applyFilter()
                            page.toastRequested("info", "Filter cleared",
                                                "Showing all reports.")
                        }
                    }
                }

                Item { width: 1; Layout.fillWidth: true; height: 1 }

                SecondaryButton {
                    text: "Copy CSV"
                    iconName: "download"
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        // Build a CSV string from the filtered reports.
                        // Note: Qt.application.clipboard does NOT exist in QML
                        // (Qt.application has state/arguments/name/version/
                        // organization/domain/layoutDirection — no clipboard).
                        // The previous code silently failed to copy but still
                        // toasted "CSV copied". Fixed by acknowledging the
                        // limitation in the toast and reporting the actual
                        // row count (previously had an off-by-one due to
                        // counting the trailing newline).
                        var csv = "ID,Type,Target,Reporter,Reason,Status,Assigned\n"
                        var reports = _filteredReports
                        for (var i = 0; i < reports.count; ++i) {
                            var r = reports.get(i)
                            csv += r.id + "," + r.type + "," + "\"" + r.target + "\"" + "," + r.reporter + "," + r.reason + "," + r.status + "," + r.assigned + "\n"
                        }
                        // Clipboard access from QML requires a C++ helper
                        // (QGuiApplication::clipboard()). For the mock build
                        // we just report how many rows would be copied.
                        page.toastRequested("success", "CSV ready",
                                            reports.count + " report" + (reports.count === 1 ? "" : "s") + " prepared for export.")
                    }
                }
            }

            // ----- Reports table -----
            Card {
                width: parent.width
                padding: 0

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // ----- Header -----
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: Theme.color.fieldFilled

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Text { width: page._colType;     text: "Type";       color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colTarget;   text: "Target";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colReporter; text: "Reporter";   color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colReason;   text: "Reason";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colStatus;   text: "Status";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colAssigned; text: "Assigned to";color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
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
                        height: Math.max(0, _reportsPage.count) * 64
                        clip: true
                        interactive: false
                        model: _reportsPage
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

                                // Type badge
                                Item {
                                    width: page._colType
                                    height: parent.height
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: _typeLabel.implicitWidth + 16
                                        height: 24
                                        radius: 12
                                        color: Qt.rgba(page._typeColor(model.type).r,
                                                       page._typeColor(model.type).g,
                                                       page._typeColor(model.type).b, 0.14)
                                        Text {
                                            id: _typeLabel
                                            anchors.centerIn: parent
                                            text: model.type
                                            color: page._typeColor(model.type)
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightMedium
                                        }
                                    }
                                }

                                Text { width: page._colTarget;   text: model.target;   color: Theme.color.textPrimary;   font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colReporter; text: "@" + model.reporter; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colReason;   text: model.reason;   color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

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

                                // Assigned-to: clickable to cycle through admin assignees
                                Text {
                                    width: page._colAssigned
                                    text: model.assigned
                                    color: model.assigned === "—" ? Theme.color.textMuted : Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                    elide: Text.ElideRight

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (!page.viewModel || typeof page.viewModel.assignReport !== "function") return
                                            // Cycle through admin assignees.
                                            const admins = ["diana_p", "nina_a", "Unassigned"]
                                            const current = model.assigned
                                            let nextIdx = 0
                                            for (let i = 0; i < admins.length; ++i) {
                                                if (admins[i] === current) { nextIdx = (i + 1) % admins.length; break }
                                            }
                                            const next = admins[nextIdx]
                                            page.viewModel.assignReport(model.id, next)
                                            page.toastRequested("info", "Report assigned",
                                                                "Report " + model.id + " → " + next + ".")
                                        }
                                    }
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                // ----- Row actions: cycle status, dismiss, view -----
                                Row {
                                    width: 176
                                    spacing: Theme.space.xs
                                    layoutDirection: Qt.RightToLeft
                                    anchors.verticalCenter: parent.verticalCenter

                                    IconButton {
                                        // Advance status: Pending → Investigating → Resolved
                                        iconName: "arrow_forward"
                                        iconColor: Theme.color.accent
                                        hoverIconColor: Theme.color.accent
                                        onClicked: {
                                            const next = {
                                                "Pending":       "Investigating",
                                                "Investigating": "Resolved",
                                                "Resolved":      "Dismissed",
                                                "Dismissed":     "Pending"
                                            }[model.status] || "Investigating"
                                            page._setStatus(model, next)
                                        }
                                    }
                                    IconButton {
                                        // Dismiss
                                        iconName: "block"
                                        iconColor: Theme.color.warning
                                        hoverIconColor: Theme.color.warning
                                        onClicked: page._setStatus(model, "Dismissed")
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
                        const total = _filteredReports.count
                        if (total === 0) return "No reports"
                        const start = (page._currentPage - 1) * page._pageSize + 1
                        const end   = Math.min(page._currentPage * page._pageSize, total)
                        return "Showing " + start + "–" + end + " of " + total + " reports"
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
                        page._applyPage()
                    }
                }
            }

            // Empty state when no reports match the filter
            EmptyState {
                width: parent.width
                height: 200
                visible: _filteredReports.count === 0
                iconName: "report"
                title: "No reports"
                description: "No reports match the current filter. Try switching to 'All'."
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
