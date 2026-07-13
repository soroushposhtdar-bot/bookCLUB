// =============================================================================
//  ServerLogsPage.qml
// =============================================================================
//  Real-time server log viewer. Filter chips for log level (All / INFO / WARN
//  / ERROR), search, export and clear buttons, and a ListView that auto-
//  scrolls to the most recent entry. Timestamps and messages use the
//  monospace font; level badges are color-coded. Sourced from
//  ServerViewModel.logs and ServerViewModel.filterLogs(), with a 5-second
//  refresh timer simulating live log streaming.
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
    id: page

    property var viewModel: null   // ServerViewModel

    signal toastRequested(string variant, string title, string description)

    property string _filter: "ALL"
    property string _query: ""
    property var _filtered: []

    function _levelColor(level) {
        if (level === "ERROR") return Theme.color.error
        if (level === "WARN")  return Theme.color.warning
        return Theme.color.info
    }

    function _levelBg(level) {
        if (level === "ERROR") return Theme.color.errorSoft
        if (level === "WARN")  return Theme.color.warningSoft
        return Theme.color.infoSoft
    }

    // Pulls the filtered set from the view model. The VM's filterLogs(level,
    // search) returns a QVariantList; we just store it.
    function _refresh() {
        if (!page.viewModel) {
            page._filtered = []
            return
        }
        var result = page.viewModel.filterLogs(page._filter, page._query)
        page._filtered = result || []
    }

    // ----- Real-time refresh timer (every 5 seconds while page is visible) -----
    Timer {
        interval: 5000
        repeat: true
        running: page.visible
        onTriggered: if (page.viewModel) page.viewModel.refresh()
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onLogsChanged: page._refresh()
    }

    Component.onCompleted: {
        page._refresh()
        _logView.positionViewAtEnd()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Filter row -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                SearchField {
                    width: parent.width * 0.45
                    placeholder: "Search logs by message or source…"
                    onTextEdited: { page._query = newText; page._refresh() }
                }

                Row {
                    spacing: Theme.space.sm
                    anchors.verticalCenter: parent.verticalCenter

                    Repeater {
                        model: [
                            { key: "ALL",   label: "All"    },
                            { key: "INFO",  label: "INFO"   },
                            { key: "WARN",  label: "WARNING" },
                            { key: "ERROR", label: "ERROR"  }
                        ]
                        FilterChip {
                            label: modelData.label
                            iconName: page._filter === modelData.key ? "check" : "filter_alt"
                            onClicked: { page._filter = modelData.key; page._refresh() }
                        }
                    }
                }

                Item { width: 1; Layout.fillWidth: true; height: 1 }

                PrimaryButton {
                    text: "Copy CSV"
                    iconName: "download"
                    onClicked: {
                        var csv = "Timestamp,Level,Source,Message\n"
                        for (var i = 0; i < page._filtered.length; ++i) {
                            var e = page._filtered[i]
                            csv += e.timestamp + "," + e.level + "," + e.source + ",\"" + (e.message || "").replace(/"/g, '""') + "\"\n"
                        }
                        try { if (typeof Qt.application !== "undefined" && Qt.application.clipboard) Qt.application.clipboard.setText(csv) } catch(e) {}
                        page.toastRequested("success", "CSV copied", page._filtered.length + " log entries copied to clipboard.")
                    }
                }
                SecondaryButton {
                    text: "Clear"
                    iconName: "delete_outline"
                    onClicked: {
                        if (page.viewModel && typeof page.viewModel.clearLogs === "function") {
                            page.viewModel.clearLogs()
                            page.toastRequested("info", "Logs cleared", "All log entries have been removed.")
                        }
                    }
                }
            }

            // ----- Logs card -----
            Card {
                width: parent.width
                padding: Theme.space.lg

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.sm

                    SectionHeader {
                        width: parent.width
                        title: "Server logs"
                        subtitle: "%1 entries (filtered)".arg(page._filtered.length)
                    }

                    ListView {
                        id: _logView
                        width: parent.width
                        height: 520
                        clip: true
                        spacing: 0
                        model: page._filtered

                        delegate: Column {
                            width: parent.width

                            Row {
                                width: parent.width
                                spacing: Theme.space.md
                                topPadding: Theme.space.sm
                                bottomPadding: Theme.space.sm

                                Text {
                                    width: 150
                                    text: modelData.timestamp
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                // Level badge
                                Rectangle {
                                    width: _lvlTxt.implicitWidth + 16
                                    height: 20
                                    radius: 4
                                    color: page._levelBg(modelData.level)
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        id: _lvlTxt
                                        anchors.centerIn: parent
                                        text: modelData.level
                                        color: page._levelColor(modelData.level)
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                Text {
                                    width: 160
                                    text: modelData.source
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightSemibold
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: parent.width - 150 - (_lvlTxt.implicitWidth + 16) - 160 - 3 * Theme.space.md
                                    text: modelData.message
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    wrapMode: Text.WordWrap
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Rectangle { width: parent.width; height: 1; color: Theme.color.divider }
                        }
                    }

                    // Load more
                    Row {
                        width: parent.width
                        layoutDirection: Qt.RightToLeft
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
