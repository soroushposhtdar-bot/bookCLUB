// LAYOUT FIX:
//   - Replaced outer ScrollView > Column with plain Column { anchors.fill: parent }
//     (same pattern as AdminUsersPage). The ScrollView+Column combo collapses
//     to zero height in Qt 6 because the Column has no explicit height and
//     ScrollView's contentHeight is unresolved on the first paint pass.
//   - Filter bar is now an Item row (no RowLayout dependency).
//   - Log Card fills remaining height via math (same as fixed admin pages).
//   - ListView inside Card always has a computed height = card height minus header.
//   - EmptyState shown when no entries match the filter.
//   - Auto-scroll to bottom on model change.

// =============================================================================
//  ServerLogsPage.qml
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

import BookClub.Services 1.0

Item {
    id: page

    property var viewModel: null   // ServerViewModel

    signal toastRequested(string variant, string title, string description)

    property string _filter: "ALL"
    property string _query: ""
    property var _filtered: []

    function _levelColor(level) {
        if (level === "ERROR")   return Theme.color.error
        if (level === "WARN" || level === "WARNING") return Theme.color.warning
        return Theme.color.info
    }

    function _levelBg(level) {
        if (level === "ERROR")   return Theme.color.errorSoft
        if (level === "WARN" || level === "WARNING") return Theme.color.warningSoft
        return Theme.color.infoSoft
    }

    // Normalize level strings that come from the log file parser
    // (server emits lowercase "info"/"warning"/"error", filter chips use uppercase)
    function _normalizeLevel(raw) {
        if (!raw) return "INFO"
        const s = raw.toUpperCase()
        if (s === "WARNING" || s === "WARN") return "WARN"
        if (s === "ERROR" || s === "ERR" || s === "CRITICAL") return "ERROR"
        return "INFO"
    }

    function _refresh() {
        if (!page.viewModel) { page._filtered = []; return }
        const raw = page.viewModel.filterLogs(page._filter, page._query) || []
        // Normalize level field so color functions work correctly
        const out = []
        for (let i = 0; i < raw.length; ++i) {
            const e = raw[i]
            if (!e) continue
            const entry = Object.assign({}, e)
            entry.level = page._normalizeLevel(e.level)
            out.push(entry)
        }
        page._filtered = out
        // Auto-scroll to most recent entry
        Qt.callLater(function() {
            if (page && _logView && page._filtered.length > 0)
                _logView.positionViewAtEnd()
        })
    }

    Timer {
        interval: 5000
        repeat: true
        running: page.visible
        onTriggered: if (page.viewModel) { page.viewModel.refresh() }
    }

    Timer {
        id: _initialFetchRetry
        interval: 800
        repeat: false
        onTriggered: {
            if (page.viewModel && typeof page.viewModel.refresh === "function")
                page.viewModel.refresh()
            page._refresh()
        }
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onLogsChanged()  { page._refresh() }
        function onRefreshed()    { page._refresh() }
        function onDataChanged()  { page._refresh() }
        function onDataReady()    { page._refresh() }
    }

    onVisibleChanged: {
        if (page.visible && page.viewModel) {
            page.viewModel.refresh()
            page._refresh()
        }
    }

    Component.onCompleted: {
        if (page.viewModel) page.viewModel.refresh()
        page._refresh()
        if (page._filtered.length === 0) _initialFetchRetry.start()
    }

    // -------------------------------------------------------------------------
    //  LAYOUT FIX: plain Column anchors.fill — same pattern as AdminUsersPage.
    // -------------------------------------------------------------------------
    Column {
        id: _mainColumn
        anchors.fill: parent
        anchors.margins: Theme.space.xl
        spacing: Theme.space.lg

        // ----- Filter / action bar -----
        Item {
            width: _mainColumn.width
            height: 44

            Row {
                anchors.fill: parent
                spacing: Theme.space.md

                SearchField {
                    width: Math.min(360, _mainColumn.width * 0.38)
                    height: parent.height
                    placeholder: "Search logs…"
                    onTextEdited: function(newText) { page._query = newText; page._refresh() }
                }

                // Level filter chips
                Row {
                    spacing: Theme.space.sm
                    height: parent.height

                    Repeater {
                        model: [
                            { key: "ALL",   label: "All"     },
                            { key: "INFO",  label: "INFO"    },
                            { key: "WARN",  label: "WARNING" },
                            { key: "ERROR", label: "ERROR"   }
                        ]
                        FilterChip {
                            label: modelData.label
                            iconName: page._filter === modelData.key ? "check" : "filter_alt"
                            onClicked: { page._filter = modelData.key; page._refresh() }
                        }
                    }
                }

                // Spacer
                Item { width: _mainColumn.width - 360 - 4 * 80 - 80 - 80 - Theme.space.md * 6; height: 1 }

                PrimaryButton {
                    text: "Copy CSV"; iconName: "download"
                    height: parent.height
                    onClicked: {
                        var csv = "Timestamp,Level,Source,Message\n"
                        for (var i = 0; i < page._filtered.length; ++i) {
                            var e = page._filtered[i]
                            csv += (e.timestamp||"") + "," + (e.level||"") + "," +
                                   (e.source||"") + ",\"" +
                                   ((e.message||"").replace(/"/g, '""')) + "\"\n"
                        }
                        try {
                            if (typeof Qt.application !== "undefined" && Qt.application.clipboard)
                                Qt.application.clipboard.setText(csv)
                        } catch(err) {}
                        page.toastRequested("success", "CSV copied",
                            page._filtered.length + " log entries copied.")
                    }
                }
                SecondaryButton {
                    text: "Clear"; iconName: "delete_outline"
                    height: parent.height
                    onClicked: {
                        if (page.viewModel && typeof page.viewModel.clearLogs === "function") {
                            page.viewModel.clearLogs()
                            page._refresh()
                            page.toastRequested("info", "Logs cleared",
                                "All log entries have been removed.")
                        }
                    }
                }
            }
        }

        // ----- Logs Card — fills remaining space -----
        Card {
            width: _mainColumn.width
            height: _mainColumn.height - 44 - Theme.space.lg
            padding: 0

            Column {
                anchors.fill: parent
                spacing: 0

                // Card header
                Item {
                    id: _cardHeader
                    width: parent.width
                    height: 52

                    SectionHeader {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.xl
                        anchors.rightMargin: Theme.space.xl
                        anchors.topMargin: Theme.space.sm
                        title: "Server logs"
                        subtitle: page._filtered.length + " entries"
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Theme.color.divider
                    }
                }

                // Table header row
                Rectangle {
                    id: _tableHeader
                    width: parent.width
                    height: 36
                    color: Theme.color.fieldFilled

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.xl
                        anchors.rightMargin: Theme.space.xl
                        spacing: Theme.space.md

                        Text { width: 160; text: "Timestamp";  color: Theme.color.textMuted; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: 72;  text: "Level";      color: Theme.color.textMuted; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: 160; text: "Source";     color: Theme.color.textMuted; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter }
                        Text {             text: "Message";    color: Theme.color.textMuted; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter }
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1; color: Theme.color.divider
                    }
                }

                // Empty state
                EmptyState {
                    width: parent.width
                    height: page._filtered.length === 0
                            ? (parent.height - _cardHeader.height - _tableHeader.height)
                            : 0
                    visible: page._filtered.length === 0
                    iconName: "terminal"
                    title: "No log entries"
                    description: "Server logs will stream here as events occur."
                }

                // Log rows ListView — fills remaining card height
                ListView {
                    id: _logView
                    width: parent.width
                    height: page._filtered.length > 0
                            ? (parent.height - _cardHeader.height - _tableHeader.height)
                            : 0
                    visible: page._filtered.length > 0
                    clip: true
                    interactive: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: page._filtered
                    spacing: 0
                    ScrollBar.vertical: ScrollBar {}

                    onCountChanged: Qt.callLater(function() {
                        if (page && _logView) _logView.positionViewAtEnd()
                    })

                    delegate: Rectangle {
                        width: _logView.width
                        height: _delRow.implicitHeight + Theme.space.sm * 2
                        color: index % 2 === 0 ? "transparent" : Qt.rgba(
                            Theme.color.fieldFilled.r,
                            Theme.color.fieldFilled.g,
                            Theme.color.fieldFilled.b, 0.5)

                        Rectangle {
                            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                            height: 1; color: Theme.color.divider; opacity: 0.5
                        }

                        // Left accent bar by level
                        Rectangle {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                            width: 3
                            color: page._levelColor(modelData.level)
                            opacity: 0.7
                        }

                        Row {
                            id: _delRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: Theme.space.xl + 3   // +3 for accent bar
                            anchors.rightMargin: Theme.space.xl
                            spacing: Theme.space.md
                            topPadding: Theme.space.sm
                            bottomPadding: Theme.space.sm

                            // Timestamp
                            Text {
                                width: 160
                                text: modelData.timestamp || ""
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight
                            }

                            // Level badge
                            Rectangle {
                                width: 64; height: 20; radius: 4
                                color: page._levelBg(modelData.level)
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.level || "INFO"
                                    color: page._levelColor(modelData.level)
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }

                            // Source
                            Text {
                                width: 160
                                text: modelData.source || "server"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightSemibold
                                anchors.verticalCenter: parent.verticalCenter
                                elide: Text.ElideRight
                            }

                            // Message — takes remaining width
                            Text {
                                width: _delRow.width - 160 - 64 - 160 - Theme.space.md * 3
                                text: modelData.message || ""
                                color: Theme.color.textPrimary
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                                wrapMode: Text.WordWrap
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
