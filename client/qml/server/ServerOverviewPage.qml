// =============================================================================
//  ServerOverviewPage.qml
// =============================================================================
//  Cluster health overview: KPI cards, a per-minute request bar chart, a
//  service status panel, and a live activity feed. Data is sourced from the
//  ServerViewModel passed in by the ServerShell.
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
    signal navigateToRequested(string route)

    // -------------------------------------------------------------------------
    //  Health helpers (CPU + RAM based)
    // -------------------------------------------------------------------------
    function _healthLabel() {
        var cpu = page.viewModel ? page.viewModel.cpuLoad : 0
        var ram = page.viewModel ? page.viewModel.ramUsage : 0
        if (cpu > 80 || ram > 85) return "Overloaded"
        if (cpu >= 50 || ram >= 70) return "Busy"
        return "Healthy"
    }

    function _healthColor() {
        var cpu = page.viewModel ? page.viewModel.cpuLoad : 0
        var ram = page.viewModel ? page.viewModel.ramUsage : 0
        if (cpu > 80 || ram > 85) return Theme.color.error
        if (cpu >= 50 || ram >= 70) return Theme.color.warning
        return Theme.color.success
    }

    // -------------------------------------------------------------------------
    //  Activity feed helpers (derived from viewModel.logs, top 8 most recent)
    // -------------------------------------------------------------------------
    function _activityTone(level) {
        if (level === "ERROR") return "error"
        if (level === "WARN")  return "warning"
        if (level === "SUCCESS") return "success"
        return "info"
    }

    function _activityIcon(level) {
        if (level === "ERROR") return "error"
        if (level === "WARN")  return "warning_amber"
        if (level === "SUCCESS") return "check_circle"
        return "info"
    }

    function _activityText(entry) {
        if (!entry) return ""
        return "<b>" + (entry.source || "Server") + "</b>: " + (entry.message || "")
    }

    property var _activity: []

    function _refreshActivity() {
        var logs = page.viewModel ? page.viewModel.logs : []
        var result = []
        var n = Math.min(logs.length, 8)
        for (var i = 0; i < n; ++i) {
            var e = logs[i]
            result.push({
                icon: page._activityIcon(e.level),
                text: page._activityText(e),
                time: e.timestamp || "",
                tone: page._activityTone(e.level)
            })
        }
        page._activity = result
    }

    Component.onCompleted: page._refreshActivity()

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onLogsChanged: page._refreshActivity()
        onClientsChanged: _bars.requestPaint()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards row + server health pill -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 4 * Theme.space.lg) / 5
                    iconName: "group"
                    value:    page.viewModel ? page.viewModel.connectedClientCount : 0
                    label:    "Connected clients"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
                StatCard {
                    width: (parent.width - 4 * Theme.space.lg) / 5
                    iconName: "verified"
                    value:    page.viewModel ? page.viewModel.activeSessionCount : 0
                    label:    "Active sessions"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 4 * Theme.space.lg) / 5
                    iconName: "speed"
                    value:    (page.viewModel ? page.viewModel.dbQueryRate : 0) + "/min"
                    label:    "DB query rate"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    width: (parent.width - 4 * Theme.space.lg) / 5
                    iconName: "monitor_heart"
                    value:    (page.viewModel ? page.viewModel.cpuLoad : 0) + "%"
                    label:    "CPU load"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.warning
                }

                // Server health pill
                Card {
                    width: (parent.width - 4 * Theme.space.lg) / 5
                    bordered: true
                    elevation: "none"
                    padding: Theme.space.lg

                    Row {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        Rectangle {
                            width: 44; height: 44; radius: 12
                            color: Qt.rgba(page._healthColor().r,
                                           page._healthColor().g,
                                           page._healthColor().b, 0.14)
                            anchors.verticalCenter: parent.verticalCenter
                            AppIcon {
                                anchors.centerIn: parent
                                name: "monitor_heart"
                                size: 22
                                color: page._healthColor()
                            }
                        }

                        Column {
                            width: parent.width - 44 - Theme.space.md
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: page._healthLabel()
                                color: page._healthColor()
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeHeadline
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: "Server health"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                            Text {
                                text: "CPU %1% · RAM %2%"
                                    .arg(page.viewModel ? page.viewModel.cpuLoad : 0)
                                    .arg(page.viewModel ? page.viewModel.ramUsage : 0)
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }
                    }
                }
            }

            // ----- Bar chart + Service status -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // Bar chart
                Card {
                    width: parent.width * 0.60 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Requests / min"
                            subtitle: "Last %1 minutes".arg(_bars._series.length)
                        }

                        Canvas {
                            id: _bars
                            width: parent.width
                            height: 180
                            property var _series: page.viewModel ? page.viewModel.requestSeries : []
                            property real _max: {
                                var m = 0
                                for (var i = 0; i < _series.length; ++i) {
                                    if (_series[i] > m) m = _series[i]
                                }
                                return m > 0 ? m * 1.1 : 1
                            }
                            property real _min: 0

                            on_SeriesChanged: requestPaint()
                            on_MaxChanged: requestPaint()
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                const w = width, h = height, pad = 8
                                const innerW = w - 2 * pad
                                const innerH = h - 2 * pad
                                if (_series.length === 0 || _max <= 0) return
                                const slot = innerW / _series.length
                                const barW = slot * 0.62

                                // Gridlines
                                ctx.strokeStyle = Qt.rgba(Theme.color.divider.r, Theme.color.divider.g, Theme.color.divider.b, 0.5)
                                ctx.lineWidth = 1
                                for (let g = 0; g <= 4; ++g) {
                                    const y = pad + (innerH * g / 4)
                                    ctx.beginPath()
                                    ctx.moveTo(pad, y)
                                    ctx.lineTo(w - pad, y)
                                    ctx.stroke()
                                }

                                // Bars
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * slot + (slot - barW) / 2
                                    const barH = (_series[i] / _max) * innerH
                                    const y = h - pad - barH
                                    ctx.fillStyle = Theme.color.accent
                                    ctx.beginPath()
                                    ctx.moveTo(x, y)
                                    ctx.lineTo(x + barW, y)
                                    ctx.lineTo(x + barW, h - pad)
                                    ctx.lineTo(x, h - pad)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                            }
                        }
                    }
                }

                // Service status
                Card {
                    width: parent.width * 0.40 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Service status"
                            subtitle: "%1 microservices".arg((page.viewModel ? page.viewModel.services : []).length)
                        }

                        ListView {
                            width: parent.width
                            height: parent.height - 40
                            clip: true
                            interactive: false
                            spacing: Theme.space.sm
                            model: page.viewModel ? page.viewModel.services : []

                            delegate: Row {
                                width: parent.width
                                spacing: Theme.space.md

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: modelData.status === "Operational" ? Theme.color.success
                                         : modelData.status === "Degraded"    ? Theme.color.warning
                                         :                                       Theme.color.error
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: 110
                                    text: modelData.name
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: 100
                                    text: modelData.status
                                    color: modelData.status === "Operational" ? Theme.color.success
                                         : modelData.status === "Degraded"    ? Theme.color.warning
                                         :                                       Theme.color.error
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                Text {
                                    text: modelData.latency
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }

            // ----- Live activity feed -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Live activity feed"
                        subtitle: "Most recent server events"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("logs")
                    }

                    ListView {
                        width: parent.width
                        height: 360
                        clip: true
                        interactive: false
                        spacing: Theme.space.sm
                        model: page._activity

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Rectangle {
                                width: 32; height: 32; radius: 8
                                color: {
                                    if (modelData.tone === "success") return Theme.color.successSoft
                                    if (modelData.tone === "warning") return Theme.color.warningSoft
                                    if (modelData.tone === "error")   return Theme.color.errorSoft
                                    return Theme.color.infoSoft
                                }
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: modelData.icon
                                    size: 18
                                    color: {
                                        if (modelData.tone === "success") return Theme.color.success
                                        if (modelData.tone === "warning") return Theme.color.warning
                                        if (modelData.tone === "error")   return Theme.color.error
                                        return Theme.color.info
                                    }
                                }
                            }

                            Column {
                                width: parent.width - 32 - Theme.space.md
                                spacing: 2

                                Text {
                                    width: parent.width
                                    text: modelData.text
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    wrapMode: Text.WordWrap
                                    textFormat: Text.RichText
                                }
                                Text {
                                    text: modelData.time
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
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
