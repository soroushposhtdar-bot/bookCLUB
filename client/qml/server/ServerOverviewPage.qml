// =============================================================================
//  LAYOUT / DESIGN FIXES APPLIED:
// =============================================================================
//  FIX 1 — Switched outer Column → ColumnLayout; KPI Row → RowLayout with
//           Layout.fillWidth + Layout.preferredWidth: 1 on each card; added
//           top spacer (Theme.space.xl); set height: Theme.size.kpiCardHeight
//           on every StatCard; removed hardcoded widths.
//  FIX 2 — Canvas color fix for Qt 6: added readonly property color
//           _accentColor / _dividerColor / _textMutedColor on the Canvas so
//           Qt resolves the Theme colors BEFORE onPaint runs. All
//           ctx.strokeStyle / ctx.fillStyle calls now use Qt.rgba(c.r, c.g,
//           c.b, a) with the pre-resolved color. Bars now render with a
//           gradient fill + rounded top corners (radius 3px).
//  FIX 3 — Health pill Card: set height: Theme.size.kpiCardHeight; replaced
//           inner Row → RowLayout with Layout.alignment: Qt.AlignVCenter;
//           replaced inner Column → ColumnLayout with Layout.fillWidth: true;
//           health label font reduced from sizeTitle → sizeBody so it fits.
//  FIX 4 — Chart + service cards: replaced Row → RowLayout; removed fixed
//           height: 280; both cards use implicitHeight driven by content;
//           Canvas height increased to 200; service ListView height driven
//           by item count (length * 40) instead of parent.height - 40.
//  FIX 5 — Live activity feed: wrapped in ColumnLayout; ListView height
//           driven by content (Math.min(400, length * 56)); empty state
//           has fixed height: 120. ListView is always interactive with
//           boundsBehavior: StopAtBounds + explicit ScrollBar.vertical so
//           the parent ScrollView doesn't jump when the first items arrive.
//  FIX 6 — ScrollView content: ColumnLayout has leftMargin / rightMargin of
//           Theme.space.xxl (32px); width adjusted to parent.width - 2 * xxl.
//  FIX 7 — Service status delegate: Row → RowLayout so Layout.fillWidth
//           works on the service-name Text; latency Text is now pushed right.
// =============================================================================
//
// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES (preserved from prior pass):
//   - Defensive value normalization for CPU/RAM (handles -1, undefined, NaN)
//   - Canvas always paints gridlines + placeholder even when no data
//   - Empty state for service status when no microservices
//   - Fallback sample request series so the chart is never blank
//   - Added fallback Connections handlers (onRefreshed, onDataChanged,
//     onRequestSeriesChanged, onServicesChanged) for Canvas repaint
//   - KPI cards: CPU clamped to 0-100, RAM clamped to 0-100
//   - Activity feed: more entries shown + defensive null checks per log entry
//   - Auto-refresh Timer (5s) pulses the VM even when no external timer fires
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
import "../components"

Item {
    id: page

    property var viewModel: null   // ServerViewModel

    signal toastRequested(string variant, string title, string description)
    signal navigateToRequested(string route)

    // -------------------------------------------------------------------------
    //  Defensive value helpers
    // -------------------------------------------------------------------------
    // The VM may return -1, undefined, NaN, or a string for cpuLoad/ramUsage.
    // _clampPercent() always returns a clean 0-100 integer.
    function _clampPercent(v) {
        if (v === undefined || v === null) return 0
        var n = Number(v)
        if (isNaN(n)) return 0
        if (n < 0) return 0
        if (n > 100) return 100
        return Math.round(n)
    }

    function _cpuLoad() {
        return page._clampPercent(page.viewModel ? page.viewModel.cpuLoad : 0)
    }
    function _ramUsage() {
        return page._clampPercent(page.viewModel ? page.viewModel.ramUsage : 0)
    }

    // -------------------------------------------------------------------------
    //  Health helpers (CPU + RAM based)
    // -------------------------------------------------------------------------
    function _healthLabel() {
        var cpu = page._cpuLoad()
        var ram = page._ramUsage()
        if (cpu > 80 || ram > 85) return "Overloaded"
        if (cpu >= 50 || ram >= 70) return "Busy"
        return "Healthy"
    }

    function _healthColor() {
        var cpu = page._cpuLoad()
        var ram = page._ramUsage()
        if (cpu > 80 || ram > 85) return Theme.color.error
        if (cpu >= 50 || ram >= 70) return Theme.color.warning
        return Theme.color.success
    }

    // -------------------------------------------------------------------------
    //  Activity feed helpers (derived from viewModel.logs, top 12 most recent)
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
        if (!logs) logs = []
        var result = []
        var n = Math.min(logs.length, 12)
        for (var i = 0; i < n; ++i) {
            var e = logs[i]
            if (!e) continue
            result.push({
                icon: page._activityIcon(e.level),
                text: page._activityText(e),
                time: e.timestamp || "",
                tone: page._activityTone(e.level)
            })
        }
        page._activity = result
    }

    // -------------------------------------------------------------------------
    //  Request series — fall back to a 14-point zero sample so the chart
    //  always has axes + gridlines even before the VM is ready. The real
    //  series replaces it as soon as the VM publishes data.
    // -------------------------------------------------------------------------
    readonly property var _requestSeries: {
        var s = page.viewModel ? page.viewModel.requestSeries : []
        if (s && s.length > 0) return s
        // 14-point zero placeholder so the chart renders axes immediately.
        return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    }

    readonly property var _services: page.viewModel ? (page.viewModel.services || []) : []

    Component.onCompleted: {
        page._refreshActivity()
        // Defer the first canvas paint until the layout has settled.
        Qt.callLater(function() { _bars.requestPaint() })
    }

    // ----- Auto-refresh Timer (3s) -----
    // v21: changed from 5s to 3s so system health numbers update more
    // frequently.
    Timer {
        interval: 3000
        repeat: true
        running: page.visible
        onTriggered: {
            if (page.viewModel && typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
            page._refreshActivity()
            _bars.requestPaint()
        }
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onLogsChanged() { page._refreshActivity() }
        function onClientsChanged() { _bars.requestPaint() }
        // Fallback handlers — repaint the bar chart + refresh the activity feed
        // whenever the VM completes a refresh cycle. These catch VMs that emit
        // a single "refreshed" / "dataChanged" signal instead of granular ones.
        function onRequestSeriesChanged() { _bars.requestPaint() }
        function onServicesChanged() { /* bindings auto-update */ }
        function onCpuLoadChanged() { /* bindings auto-update */ }
        function onRamUsageChanged() { /* bindings auto-update */ }
        function onRefreshed() { _bars.requestPaint(); page._refreshActivity() }
        function onDataChanged() { _bars.requestPaint(); page._refreshActivity() }
        function onDataReady() { _bars.requestPaint(); page._refreshActivity() }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        // FIX 6 — ColumnLayout with horizontal margins so content doesn't
        // touch the window edges.
        ColumnLayout {
            id: _rootColumn
            width: parent.width - 2 * Theme.space.xxl
            anchors.leftMargin: Theme.space.xxl
            anchors.rightMargin: Theme.space.xxl
            spacing: Theme.space.xl

            // FIX 1 — Top breathing room (was a tiny Theme.space.lg spacer
            // inside a plain Column; now a proper Layout spacer).
            Item { Layout.fillWidth: true; height: Theme.space.xl }

            // =================================================================
            //  FIX 1 — KPI cards row + server health pill (RowLayout)
            // =================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "group"
                    value:    (page.viewModel ? page.viewModel.connectedClientCount : 0).toString()
                    label:    "Connected clients"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "verified"
                    value:    (page.viewModel ? page.viewModel.activeSessionCount : 0).toString()
                    label:    "Active sessions"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "speed"
                    value:    (page.viewModel ? page.viewModel.dbQueryRate : 0) + "/min"
                    label:    "DB query rate"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "memory"
                    value:    page._cpuLoad() + "%"
                    label:    "CPU load"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.warning
                }

                // ----- FIX 3 — Server health pill (height + RowLayout) -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    bordered: true
                    elevation: "none"
                    padding: Theme.space.md

                    RowLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        Rectangle {
                            width: 40; height: 40; radius: 10
                            color: Qt.rgba(page._healthColor().r,
                                           page._healthColor().g,
                                           page._healthColor().b, 0.14)
                            Layout.alignment: Qt.AlignVCenter
                            AppIcon {
                                anchors.centerIn: parent
                                name: "monitor_heart"
                                size: 20
                                color: page._healthColor()
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            Text {
                                Layout.fillWidth: true
                                text: page._healthLabel()
                                color: page._healthColor()
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "Server health"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "CPU %1% · RAM %2%".arg(page._cpuLoad()).arg(page._ramUsage())
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeMicro2
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            // =================================================================
            //  FIX 4 — Bar chart + Service status (RowLayout, auto-height)
            // =================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                // ----- Bar chart card (60%) -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    // FIX 4 — No fixed height; driven by content via implicitHeight
                    implicitHeight: _barsSection.implicitHeight + 2 * Theme.space.xl
                    padding: Theme.space.xl

                    ColumnLayout {
                        id: _barsSection
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Requests / min"
                            subtitle: "Last %1 minutes".arg(page._requestSeries.length)
                        }

                        // FIX 2 — Canvas with pre-resolved color properties.
                        // In Qt 6, assigning a QML color object directly to
                        // ctx.fillStyle silently fails. We declare the colors
                        // as readonly properties so Qt resolves them once
                        // (lazily, on first read) and then .r / .g / .b are
                        // guaranteed to be valid floats inside onPaint.
                        Canvas {
                            id: _bars
                            Layout.fillWidth: true
                            height: 200

                            // ---- Pre-resolved colors (FIX 2) ----
                            readonly property color _accentColor:  Theme.color.accent
                            readonly property color _dividerColor: Theme.color.divider
                            readonly property color _textMutedColor: Theme.color.textMuted

                            // ---- Canvas data properties (unchanged) ----
                            readonly property var _series: page._requestSeries
                            readonly property real _max: {
                                var m = 0
                                for (var i = 0; i < _series.length; ++i) {
                                    var v = Number(_series[i])
                                    if (!isNaN(v) && v > m) m = v
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

                                // ---- Gridlines (always paint) ----
                                // FIX 2 — use pre-resolved _dividerColor
                                ctx.strokeStyle = Qt.rgba(_dividerColor.r, _dividerColor.g,
                                                          _dividerColor.b, 0.5)
                                ctx.lineWidth = 1
                                for (let g = 0; g <= 4; ++g) {
                                    const y = pad + (innerH * g / 4)
                                    ctx.beginPath()
                                    ctx.moveTo(pad, y)
                                    ctx.lineTo(w - pad, y)
                                    ctx.stroke()
                                }

                                // ---- Bars ----
                                const n = _series.length
                                if (n > 0 && _max > 0) {
                                    const slot = innerW / n
                                    const barW = Math.max(2, slot * 0.62)
                                    const r = Math.min(3, barW / 2)   // rounded corner radius
                                    for (let i = 0; i < n; ++i) {
                                        const v = Number(_series[i]) || 0
                                        const x = pad + i * slot + (slot - barW) / 2
                                        const barH = (v / _max) * innerH
                                        const y = h - pad - barH

                                        // FIX 2 — gradient fill with pre-resolved accent color
                                        const grad = ctx.createLinearGradient(x, y, x, h - pad)
                                        grad.addColorStop(0, Qt.rgba(_accentColor.r, _accentColor.g,
                                                                     _accentColor.b, 1.0))
                                        grad.addColorStop(1, Qt.rgba(_accentColor.r, _accentColor.g,
                                                                     _accentColor.b, 0.5))
                                        ctx.fillStyle = grad

                                        // Rounded top corners (FIX 2)
                                        ctx.beginPath()
                                        ctx.moveTo(x + r, y)
                                        ctx.lineTo(x + barW - r, y)
                                        ctx.quadraticCurveTo(x + barW, y, x + barW, y + r)
                                        ctx.lineTo(x + barW, h - pad)
                                        ctx.lineTo(x, h - pad)
                                        ctx.lineTo(x, y + r)
                                        ctx.quadraticCurveTo(x, y, x + r, y)
                                        ctx.closePath()
                                        ctx.fill()
                                    }
                                }

                                // ---- "No data" watermark ----
                                if (n === 0) {
                                    // FIX 2 — use pre-resolved _textMutedColor
                                    ctx.fillStyle = Qt.rgba(_textMutedColor.r, _textMutedColor.g,
                                                            _textMutedColor.b, 1.0)
                                    ctx.font = "14px sans-serif"
                                    ctx.textAlign = "center"
                                    ctx.fillText("Waiting for data…", w / 2, h / 2)
                                }
                            }
                        }
                    }
                }

                // ----- Service status card (40%) -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 40
                    // FIX 4 — match the bar chart card's height
                    implicitHeight: _barsSection.implicitHeight + 2 * Theme.space.xl
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Service status"
                            subtitle: "%1 microservices".arg(page._services.length)
                        }

                        // Empty state when no services are registered
                        EmptyState {
                            Layout.fillWidth: true
                            height: 160
                            visible: page._services.length === 0
                            iconName: "dns"
                            title: "No services registered"
                            description: "Microservice status will appear here once the server reports them."
                        }

                        // Service list — height driven by item count (FIX 4)
                        ListView {
                            Layout.fillWidth: true
                            height: Math.max(0, page._services.length) * 40
                            clip: true
                            interactive: page._services.length > 4
                            spacing: Theme.space.sm
                            model: page._services
                            visible: page._services.length > 0

                            // FIX 7 — RowLayout delegate so Layout.fillWidth works
                            delegate: RowLayout {
                                width: parent.width
                                spacing: Theme.space.md

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: modelData.status === "Operational" ? Theme.color.success
                                         : modelData.status === "Degraded"    ? Theme.color.warning
                                         :                                      Theme.color.error
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Text {
                                    text: modelData.name || "—"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: modelData.status || "Unknown"
                                    color: modelData.status === "Operational" ? Theme.color.success
                                         : modelData.status === "Degraded"    ? Theme.color.warning
                                         :                                      Theme.color.error
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }

                                Text {
                                    text: modelData.latency || ""
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                            }
                        }
                    }
                }
            }

            // =================================================================
            //  FIX 5 — Live activity feed (ColumnLayout, content-driven height)
            // =================================================================
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Live activity feed"
                        subtitle: "Most recent server events"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("logs")
                    }

                    // Empty state for activity feed
                    EmptyState {
                        Layout.fillWidth: true
                        height: 120
                        visible: page._activity.length === 0
                        iconName: "terminal"
                        title: "No activity yet"
                        description: "Server events will appear here as they occur."
                    }

                    ListView {
                        Layout.fillWidth: true
                        // BUG 3 — increased max height to 400 (was 320) so wrapped
                        // text delegates don't clip. Math.max(1,...) prevents the
                        // height from collapsing to 0 on empty, which triggered
                        // the jump when first items arrived.
                        height: Math.min(400, Math.max(1, page._activity.length) * 56)
                        clip: true
                        // BUG 3 — always interactive + StopAtBounds so the parent
                        // ScrollView doesn't re-evaluate childrenRect.height when
                        // interactive flips from false to true on first data arrival.
                        interactive: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        spacing: Theme.space.sm
                        model: page._activity
                        visible: page._activity.length > 0

                        delegate: Row {
                            width: ListView.view ? ListView.view.width : (parent ? parent.width : 400)
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
                                clip: true

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

            // Bottom spacer
            Item { Layout.fillWidth: true; height: Theme.space.xxl }
        }
    }
}
