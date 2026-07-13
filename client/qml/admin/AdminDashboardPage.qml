// =============================================================================
//  AdminDashboardPage.qml
// =============================================================================
//  Overview screen for the admin role. Shows KPI stat cards, a 14-day user
//  growth sparkline, system health bars, and a recent moderation activity feed.
//  Data is synthesized locally — in production this would come from an
//  AdminViewModel backed by the socket protocol.
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
    signal navigateToRequested(string route)

    // ----- KPI cards — values bound to the AdminViewModel -----
    readonly property var _kpis: [
        { icon: "group",          value: (page.viewModel ? page.viewModel.totalUsers : 0).toLocaleString(Qt.locale(), "f", 0),            label: "Total users",        delta: "+4.2% vs last month", deltaUp: true,  accent: Theme.color.accent  },
        { icon: "business",       value: (page.viewModel ? page.viewModel.activePublishersCount : 0).toString(),                          label: "Active publishers",  delta: "+2 this week",         deltaUp: true,  accent: Theme.color.success },
        { icon: "report",         value: (page.viewModel ? page.viewModel.pendingReports : 0).toString(),                                 label: "Pending reports",    delta: "+3 today",             deltaUp: false, accent: Theme.color.warning },
        { icon: "monitor_heart",  value: page.viewModel ? (page.viewModel.systemUptime || "—") : "—",                                     label: "System uptime",      delta: "+0.01% this week",     deltaUp: true,  accent: Theme.color.info    }
    ]

    // ----- System health bars — bound to viewModel.systemHealth -----
    readonly property var _healthData: page.viewModel ? page.viewModel.systemHealth : {}
    readonly property var _health: [
        { label: "CPU",     value: page._healthData.cpu || 0,    text: page._healthData.cpuText || "0%",    color: (page._healthData.cpu || 0) > 0.8 ? Theme.color.error : (page._healthData.cpu || 0) > 0.5 ? Theme.color.warning : Theme.color.success },
        { label: "Memory",  value: page._healthData.memory || 0, text: page._healthData.memoryText || "0%", color: (page._healthData.memory || 0) > 0.85 ? Theme.color.error : (page._healthData.memory || 0) > 0.7 ? Theme.color.warning : Theme.color.success },
        { label: "Disk",    value: page._healthData.disk || 0,   text: page._healthData.diskText || "0%",   color: Theme.color.accent  }
    ]

    // ----- Recent moderation activity — sourced from the VM's auditLog
    //       (QVariantList of { timestamp, action, user, details, severity }) -----
    readonly property var _activity: page.viewModel ? (page.viewModel.auditLog || []) : []

    // ----- Mapping from VM audit-log severity → UI tone + icon -----
    function _severityIcon(s) {
        if (s === "error")   return "report"
        if (s === "warning") return "warning"
        if (s === "success") return "check_circle"
        return "gavel"
    }
    function _severityTone(s) {
        if (s === "error")   return "error"
        if (s === "warning") return "warning"
        if (s === "success") return "success"
        return "info"
    }

    // ----- Re-paint the user-growth canvas when the VM pushes a new series.
    //       KPI cards + audit-log ListView re-evaluate automatically through
    //       their property bindings (they read page.viewModel.* directly). -----
    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onUsersChanged: _spark.requestPaint()
    }

    Component.onCompleted: {
        if (page.viewModel && typeof page.viewModel.refresh === "function") {
            page.viewModel.refresh()
        }
    }

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
                        width: (parent.width - 3 * Theme.space.lg) / 4
                        iconName: modelData.icon
                        value:    modelData.value
                        label:    modelData.label
                        delta:    modelData.delta
                        deltaUp:  modelData.deltaUp
                        accent:   modelData.accent
                    }
                }
            }

            // ----- User growth + System health row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // User growth sparkline card (60%)
                Card {
                    width: parent.width * 0.60 - Theme.space.lg / 2
                    height: 300
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "User growth (last 14 days)"
                            subtitle: "New signups per day"
                        }

                        Canvas {
                            id: _spark
                            width: parent.width
                            height: 200
                            // ----- User-growth series sourced from the VM -----
                            readonly property var _series: page.viewModel ? (page.viewModel.userGrowthSeries || []) : []
                            readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                            readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                if (_series.length === 0) return
                                const w = width, h = height, pad = 8
                                const stepX = _series.length > 1 ? (w - 2 * pad) / (_series.length - 1) : 0
                                const range = Math.max(1, _max - _min)

                                // Area fill
                                ctx.beginPath()
                                ctx.moveTo(pad, h - pad)
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                    ctx.lineTo(x, y)
                                }
                                ctx.lineTo(w - pad, h - pad)
                                ctx.closePath()
                                ctx.fillStyle = Qt.rgba(Theme.color.accent.r,
                                                        Theme.color.accent.g,
                                                        Theme.color.accent.b, 0.16)
                                ctx.fill()

                                // Line stroke
                                ctx.beginPath()
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                    if (i === 0) ctx.moveTo(x, y)
                                    else         ctx.lineTo(x, y)
                                }
                                ctx.lineWidth = 2
                                ctx.strokeStyle = Theme.color.accent
                                ctx.stroke()
                            }
                        }

                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Text {
                                text: "Total: " + (page.viewModel && page.viewModel.userGrowthSeries
                                                   ? page.viewModel.userGrowthSeries.reduce(function(a, b){ return a + b; }, 0).toLocaleString(Qt.locale(), "f", 0)
                                                   : "0") + " new users"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: {
                                    if (!page.viewModel || !page.viewModel.userGrowthSeries || page.viewModel.userGrowthSeries.length === 0) return "—"
                                    var first = page.viewModel.userGrowthSeries[0]
                                    var last = page.viewModel.userGrowthSeries[page.viewModel.userGrowthSeries.length-1]
                                    if (first === 0) return last > 0 ? "▲ 100%" : "—"
                                    var pct = Math.round((last - first) / first * 100)
                                    return (pct >= 0 ? "▲ " : "▼ ") + Math.abs(pct) + "%"
                                }
                                color: {
                                    if (!page.viewModel || !page.viewModel.userGrowthSeries || page.viewModel.userGrowthSeries.length === 0) return Theme.color.textMuted
                                    var first = page.viewModel.userGrowthSeries[0]
                                    var last = page.viewModel.userGrowthSeries[page.viewModel.userGrowthSeries.length-1]
                                    return last >= first ? Theme.color.success : Theme.color.error
                                }
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            TextButton {
                                text: "View analytics"
                                iconName: "arrow_forward"
                                onClicked: page.navigateToRequested("analytics")
                            }
                        }
                    }
                }

                // System health card (40%)
                Card {
                    width: parent.width * 0.40 - Theme.space.lg / 2
                    height: 300
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "System health"
                            subtitle: "Live infrastructure metrics"
                        }

                        Repeater {
                            model: page._health
                            Column {
                                width: parent.width
                                spacing: Theme.space.xs

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Text {
                                        text: modelData.label
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: Math.round(modelData.value * 100) + "%"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                ProgressBar {
                                    width: parent.width
                                    barHeight: 8
                                    value: modelData.value
                                    color: modelData.color
                                }
                            }
                        }

                        Item { width: 1; Layout.fillHeight: true; height: 1 }

                        Row {
                            width: parent.width
                            spacing: Theme.space.sm

                            AppIcon {
                                name: "check_circle"
                                size: Theme.size.iconSm
                                color: Theme.color.success
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: page._healthData.status === "Healthy" ? "All systems operational" : page._healthData.status === "Busy" ? "System under heavy load" : "System overloaded — action required"
                                color: page._healthData.status === "Healthy" ? Theme.color.success : page._healthData.status === "Busy" ? Theme.color.warning : Theme.color.error
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                        }
                    }
                }
            }

            // ----- Recent moderation activity -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Recent moderation activity"
                        subtitle: "Last 24 hours of admin actions"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("reports")
                    }

                    ListView {
                        width: parent.width
                        height: 280
                        clip: true
                        model: page._activity
                        spacing: Theme.space.sm
                        interactive: false

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Rectangle {
                                width: 32; height: 32; radius: 8
                                color: {
                                    const tone = page._severityTone(modelData.severity)
                                    if (tone === "success") return Theme.color.successSoft
                                    if (tone === "warning") return Theme.color.warningSoft
                                    if (tone === "error")   return Theme.color.errorSoft
                                    return Theme.color.infoSoft
                                }
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: page._severityIcon(modelData.severity)
                                    size: 18
                                    color: {
                                        const tone = page._severityTone(modelData.severity)
                                        if (tone === "success") return Theme.color.success
                                        if (tone === "warning") return Theme.color.warning
                                        if (tone === "error")   return Theme.color.error
                                        return Theme.color.info
                                    }
                                }
                            }

                            Column {
                                width: parent.width - 32 - Theme.space.md
                                spacing: 2

                                Text {
                                    width: parent.width
                                    text: "<b>" + (modelData.user || "system") + "</b> "
                                          + (modelData.action || "performed an action")
                                          + (modelData.details ? " — " + modelData.details : "")
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    wrapMode: Text.WordWrap
                                    textFormat: Text.RichText
                                }
                                Text {
                                    text: modelData.timestamp || ""
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
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
