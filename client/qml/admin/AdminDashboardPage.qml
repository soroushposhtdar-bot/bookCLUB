// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - System health: defensive value normalization (handles 0-1 and 0-100 ranges)
//   - System health: robust status display (Healthy / Busy / Overloaded / Unknown)
//   - System health: status derived from CPU/RAM when VM status field is missing
//   - Canvas always paints gridlines + "Waiting for data…" watermark when no data
//   - Auto-refresh Timer (5s) pulses VM + repaints sparkline
//   - Added fallback Connections handlers (onRefreshed, onDataChanged,
//     onUserGrowthSeriesChanged) for Canvas repaint
//
// LAYOUT/DATA FIXES (this pass):
//   - BUG A: V4ReferenceObject — never call .reduce() on viewModel.userGrowthSeries
//     directly; use _spark._series (a safe JS array copy via property var) instead.
//   - BUG B: System health field mapping — _healthData may use cpuLoad/ramUsage/diskUsage
//     (0-100 integers) OR cpu/memory/disk (0-1 fractions). Try both key families.
//   - BUG C: Activity feed jump — fixed with Math.max(1,...) and correct layout.
//   - BUG D: Removed "Server Console" TextButton from System health card.
//   - BUG 2 (audit-log field mapping): normalizes server payload fields.
//   - LAYOUT FIX: Replaced ScrollView > ColumnLayout with plain Column + anchors.fill
//     (same pattern as AdminUsersPage). The ScrollView + ColumnLayout combo collapses
//     to zero height in Qt6 when implicitHeight is not yet computed on first paint,
//     causing the audit log and other cards to be invisible until resize.

// =============================================================================
//  AdminDashboardPage.qml
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
    signal navigateToRequested(string route)

    readonly property var _kpis: [
        { icon: "group",          value: (page.viewModel ? page.viewModel.totalUsers : 0).toLocaleString(Qt.locale(), "f", 0),            label: "Total users",        delta: "live", deltaUp: true,  accent: Theme.color.accent  },
        { icon: "business",       value: (page.viewModel ? page.viewModel.activePublishersCount : 0).toString(),                          label: "Active publishers",  delta: "live", deltaUp: true,  accent: Theme.color.success },
        { icon: "report",         value: (page.viewModel ? page.viewModel.pendingReports : 0).toString(),                                 label: "Pending reports",    delta: "live", deltaUp: false, accent: Theme.color.warning },
        { icon: "monitor_heart",  value: page.viewModel ? (page.viewModel.systemUptime || "—") : "—",                                     label: "System uptime",      delta: "live", deltaUp: true,  accent: Theme.color.info    }
    ]

    readonly property var _healthData: page.viewModel ? page.viewModel.systemHealth : {}

    function _normalizeHealthValue(v) {
        if (v === undefined || v === null) return 0
        var n = Number(v)
        if (isNaN(n)) return 0
        if (n > 1) return n / 100
        return n
    }
    function _healthColor(v, warnThreshold, errorThreshold) {
        var n = page._normalizeHealthValue(v)
        if (n > errorThreshold) return Theme.color.error
        if (n > warnThreshold)  return Theme.color.warning
        return Theme.color.success
    }

    readonly property var _health: [
        {
            label: "CPU",
            value: Math.min(1, Math.max(0,
                (_healthData.cpuLoad !== undefined ? _healthData.cpuLoad / 100 :
                 _healthData.cpu     !== undefined ? page._normalizeHealthValue(_healthData.cpu) : 0))),
            text:  _healthData.cpuLoad !== undefined ? _healthData.cpuLoad + "%" :
                   _healthData.cpuText !== undefined ? _healthData.cpuText : "0%",
            color: page._healthColor(
                _healthData.cpuLoad !== undefined ? _healthData.cpuLoad / 100 :
                (_healthData.cpu !== undefined ? page._normalizeHealthValue(_healthData.cpu) : 0),
                0.5, 0.8)
        },
        {
            label: "Memory",
            value: Math.min(1, Math.max(0,
                (_healthData.ramUsage    !== undefined ? _healthData.ramUsage / 100 :
                 _healthData.memoryUsage !== undefined ? _healthData.memoryUsage / 100 :
                 _healthData.memory      !== undefined ? page._normalizeHealthValue(_healthData.memory) : 0))),
            text:  _healthData.ramUsage    !== undefined ? _healthData.ramUsage + "%" :
                   _healthData.memoryText !== undefined ? _healthData.memoryText : "0%",
            color: page._healthColor(
                _healthData.ramUsage    !== undefined ? _healthData.ramUsage / 100 :
                (_healthData.memoryUsage !== undefined ? _healthData.memoryUsage / 100 :
                 (_healthData.memory !== undefined ? page._normalizeHealthValue(_healthData.memory) : 0)),
                0.7, 0.85)
        },
        {
            label: "Disk",
            value: Math.min(1, Math.max(0,
                (_healthData.diskUsage !== undefined ? _healthData.diskUsage / 100 :
                 _healthData.disk      !== undefined ? page._normalizeHealthValue(_healthData.disk) : 0))),
            text:  _healthData.diskUsage !== undefined ? _healthData.diskUsage + "%" :
                   _healthData.diskText !== undefined ? _healthData.diskText : "0%",
            color: Theme.color.accent
        }
    ]

    readonly property string _healthStatus: {
        var cpu = _healthData.cpuLoad !== undefined ? _healthData.cpuLoad
                : (_healthData.cpu !== undefined ? page._normalizeHealthValue(_healthData.cpu) * 100 : 0)
        var ram = _healthData.ramUsage !== undefined ? _healthData.ramUsage
                : (_healthData.memoryUsage !== undefined ? _healthData.memoryUsage
                : (_healthData.memory !== undefined ? page._normalizeHealthValue(_healthData.memory) * 100 : 0))
        if (cpu > 80 || ram > 85) return "Overloaded"
        if (cpu >= 50 || ram >= 70) return "Busy"
        return "Healthy"
    }
    readonly property color _healthStatusColor: {
        if (page._healthStatus === "Overloaded") return Theme.color.error
        if (page._healthStatus === "Busy")       return Theme.color.warning
        return Theme.color.success
    }

    function _normalizeLevel(l) {
        if (l === undefined || l === null) return "info"
        var s = String(l).toLowerCase()
        if (s === "error" || s === "err" || s === "critical") return "error"
        if (s === "warn"  || s === "warning")                  return "warning"
        if (s === "success" || s === "ok")                     return "success"
        return "info"
    }

    readonly property var _activity: {
        if (!page.viewModel || !page.viewModel.auditLog) return []
        var raw = page.viewModel.auditLog
        var out = []
        for (var i = 0; i < raw.length; ++i) {
            var e = raw[i]
            if (!e) continue
            var level = page._normalizeLevel(e.level !== undefined ? e.level : e.severity)
            out.push({
                timestamp: e.timestamp || "",
                severity:  level,
                level:     level,
                user:      e.user     || e.source    || "system",
                source:    e.source   || e.user     || "system",
                action:    e.action   || e.message  || "performed an action",
                message:   e.message  || e.action   || "",
                details:   e.details  || ""
            })
        }
        return out
    }

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

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onUsersChanged() { _spark.requestPaint() }
        function onRefreshed() { _spark.requestPaint() }
        function onDataChanged() { _spark.requestPaint() }
        function onDataReady() { _spark.requestPaint() }
        function onSystemHealthChanged() { /* bindings auto-update */ }
        function onUserGrowthSeriesChanged() { _spark.requestPaint() }
        function onAuditLogChanged() { /* bindings auto-update */ }
    }

    Timer {
        id: _dashTimer
        interval: 60000
        repeat: true
        running: page.visible
        onTriggered: {
            if (!page || !page.visible) return
            if (page.viewModel && typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
            Qt.callLater(function() { if (page && _spark) _spark.requestPaint() })
        }
    }

    onVisibleChanged: {
        if (page.visible) {
            Qt.callLater(function() {
                if (page && page.viewModel && typeof page.viewModel.refresh === "function") {
                    page.viewModel.refresh()
                }
                if (page && _spark) _spark.requestPaint()
            })
        }
    }

    Component.onCompleted: {
        if (page.viewModel && typeof page.viewModel.refresh === "function") {
            page.viewModel.refresh()
        }
    }

    // -------------------------------------------------------------------------
    //  v21b: wrapped in a Flickable so the dashboard scrolls when content
    //  exceeds the viewport height. Previously used a plain Column with
    //  anchors.fill which clipped content taller than the page.
    // -------------------------------------------------------------------------
    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _mainColumn.implicitHeight + 2 * Theme.space.xl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    Column {
        id: _mainColumn
        width: parent.width
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
                        name: "dashboard"
                        size: 22
                        color: Theme.color.accent
                    }
                }
                Column {
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: "Dashboard"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                    }
                    Text {
                        text: "Platform health at a glance"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
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
            width: _mainColumn.width
            spacing: Theme.space.lg

            // User growth sparkline card (60%)
            Card {
                width: (_mainColumn.width - Theme.space.lg) * 0.6
                height: 300
                padding: Theme.space.xl

                Column {
                    width: parent.width
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
                        readonly property var _series: page.viewModel ? (page.viewModel.userGrowthSeries || []) : []
                        readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                        readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            const w = width, h = height, pad = 8
                            const innerH = h - 2 * pad

                            ctx.strokeStyle = Qt.rgba(Theme.color.divider.r, Theme.color.divider.g, Theme.color.divider.b, 0.5)
                            ctx.lineWidth = 1
                            for (let g = 0; g <= 4; ++g) {
                                const y = pad + (innerH * g / 4)
                                ctx.beginPath()
                                ctx.moveTo(pad, y)
                                ctx.lineTo(w - pad, y)
                                ctx.stroke()
                            }

                            if (_series.length === 0) {
                                ctx.fillStyle = Theme.color.textMuted
                                ctx.font = "14px sans-serif"
                                ctx.textAlign = "center"
                                ctx.fillText("Waiting for data…", w / 2, h / 2)
                                return
                            }
                            const stepX = _series.length > 1 ? (w - 2 * pad) / (_series.length - 1) : 0
                            const range = Math.max(1, _max - _min)

                            ctx.beginPath()
                            ctx.moveTo(pad, h - pad)
                            for (let i = 0; i < _series.length; ++i) {
                                const x = pad + i * stepX
                                const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                ctx.lineTo(x, y)
                            }
                            ctx.lineTo(w - pad, h - pad)
                            ctx.closePath()
                            ctx.fillStyle = Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.16)
                            ctx.fill()

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
                            text: {
                                var s = _spark._series
                                if (!s || s.length === 0) return "Total: 0 new users"
                                var sum = 0
                                for (var i = 0; i < s.length; ++i) sum += (s[i] || 0)
                                return "Total: " + sum.toLocaleString(Qt.locale(), "f", 0) + " new users"
                            }
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: {
                                var s = _spark._series
                                if (!s || s.length === 0) return "—"
                                var first = s[0]
                                var last  = s[s.length - 1]
                                if (first === 0) return last > 0 ? "▲ 100%" : "—"
                                var pct = Math.round((last - first) / first * 100)
                                return (pct >= 0 ? "▲ " : "▼ ") + Math.abs(pct) + "%"
                            }
                            color: {
                                var s = _spark._series
                                if (!s || s.length === 0) return Theme.color.textMuted
                                var first = s[0]
                                var last  = s[s.length - 1]
                                return last >= first ? Theme.color.success : Theme.color.error
                            }
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Item { width: 1; height: 1; Layout.fillWidth: true }
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
                width: (_mainColumn.width - Theme.space.lg) * 0.4
                height: 300
                padding: Theme.space.xl

                Column {
                    width: parent.width
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

                            RowLayout {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: modelData.label
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: Math.round(modelData.value * 100) + "%"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
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

                    Item { width: 1; height: 8 }

                    Row {
                        width: parent.width
                        spacing: Theme.space.sm

                        AppIcon {
                            name: {
                                var c = page._healthStatusColor
                                if (c === Theme.color.error)   return "error"
                                if (c === Theme.color.warning) return "warning_amber"
                                return "check_circle"
                            }
                            size: Theme.size.iconSm
                            color: page._healthStatusColor
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: page._healthStatus === "Healthy" ? "All systems operational"
                                 : page._healthStatus === "Busy"    ? "System under heavy load"
                                 :                                      "System overloaded — action required"
                            color: page._healthStatusColor
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // ----- Recent moderation activity -----
        // LAYOUT FIX: Card with fixed height so it's always visible.
        // The audit log ListView fills the card body — no collapsing to 0.
        Card {
            width: _mainColumn.width
            // Height = remaining space after header(56) + kpi row(kpiCardHeight) +
            // spark/health row(300) + spacing. Use a sensible fixed height that
            // guarantees the log is always shown.
            height: Math.max(300,
                    _mainColumn.height
                    - 56                         // header
                    - Theme.size.kpiCardHeight   // KPI row
                    - 300                        // spark+health row
                    - Theme.space.lg * 3         // three gaps between rows
                    - Theme.space.xl * 2)        // top+bottom margins
            padding: 0

            Column {
                anchors.fill: parent
                spacing: 0

                // Section header area
                Item {
                    width: parent.width
                    height: 64
                    anchors.leftMargin: Theme.space.xl
                    anchors.rightMargin: Theme.space.xl

                    SectionHeader {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.xl
                        anchors.rightMargin: Theme.space.xl
                        anchors.topMargin: Theme.space.md
                        title: "Recent moderation activity"
                        subtitle: "Last 24 hours of admin actions"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("reports")
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Theme.color.divider
                }

                // Audit log ListView — fills remaining Card height
                ListView {
                    id: _activityListView
                    width: parent.width
                    height: parent.height - 65   // card height minus header+divider
                    clip: true
                    interactive: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: page._activity
                    spacing: 0

                    delegate: Rectangle {
                        width: _activityListView.width
                        height: _delRow.implicitHeight + Theme.space.md * 2
                        color: index % 2 === 0 ? "transparent" : Theme.color.fieldFilled

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.color.divider
                        }

                        Row {
                            id: _delRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
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
                                anchors.verticalCenter: parent.verticalCenter
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
                                anchors.verticalCenter: parent.verticalCenter

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

                    EmptyState {
                        anchors.centerIn: parent
                        width: parent.width
                        height: 200
                        visible: page._activity.length === 0
                        iconName: "history"
                        title: "No recent activity"
                        description: "Admin actions and system events will appear here."
                    }
                }
            }
        }
    }
    }  // v21b: close Flickable
}
