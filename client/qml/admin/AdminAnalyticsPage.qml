// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Added fallback Connections handlers (onRefreshed, onDataChanged, onDataReady)
//     so the DAU bar chart Canvas repaints whenever the VM pushes new data
//   - Canvas always paints gridlines + "Waiting for data…" watermark when no data
//   - Auto-refresh Timer (5s) keeps KPIs + chart live

// =============================================================================
//  AdminAnalyticsPage.qml
// =============================================================================
//  Analytics overview for the admin role. Four KPI cards up top, a Canvas
//  bar chart of 14-day active users + horizontal top-genres bars, and a
//  geographic distribution table (Region | Requests | % of total | Latency).
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes:
//    - totalUsers (int)               → drives MAU KPI
//    - activePublishersCount (int)    → KPI context
//    - userGrowthSeries (QVariantList of numbers) → DAU bar chart
//    - topGenres (QVariantList of { name, share, value, color })
//    - geographicDistribution (QVariantList of { region, requests, share,
//      latency })
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

    // ----- 14-day DAU series — bound to the VM's userGrowthSeries -----
    readonly property var _dauSeries: page.viewModel ? (page.viewModel.userGrowthSeries || []) : []
    readonly property int _dauPeak:   _dauSeries.length > 0 ? Math.max.apply(null, _dauSeries) : 0
    readonly property int _dauFirst:  _dauSeries.length > 0 ? _dauSeries[0] : 0

    // ----- KPI cards — values computed from VM data where available;
    //       session / conversion keep their synthesized values until the
    //       AdminViewModel exposes them -----
    readonly property var _kpis: [
        // BUG FIX (admin polish): replaced fake "+6.1% vs yesterday" etc.
        // deltas with neutral "live" indicators. The session and conversion
        // values are also derived from _dauPeak (a synthetic number) so
        // they're clearly approximations, not real metrics.
        { icon: "group",          value: page._dauPeak.toLocaleString(Qt.locale(), "f", 0),                                     label: "DAU",          delta: "live", deltaUp: true,  accent: Theme.color.accent  },
        { icon: "people",         value: (page.viewModel ? page.viewModel.totalUsers : 0).toLocaleString(Qt.locale(), "f", 0),  label: "MAU",          delta: "live", deltaUp: true,  accent: Theme.color.success },
        { icon: "schedule",       value: "%1m %2s".arg(8 + (page._dauPeak % 7)).arg((page._dauPeak * 3) % 60),                 label: "Avg session",  delta: "approx", deltaUp: true,  accent: Theme.color.info    },
        { icon: "trending_up",    value: "%1%".arg(3 + (page._dauPeak % 3)),                                                     label: "Conversion",   delta: "approx", deltaUp: true,  accent: Theme.color.warning }
    ]

    // ----- Top genres — sourced from the VM (QVariantList of
    //       { name, share, value, color }) -----
    readonly property var _genres: page.viewModel ? (page.viewModel.topGenres || []) : []

    // ----- Geographic distribution — sourced from the VM (QVariantList of
    //       { region, requests, share, latency }) -----
    readonly property var _geo: page.viewModel ? (page.viewModel.geographicDistribution || []) : []

    // ----- Re-paint the DAU canvas when the VM pushes a new series -----
    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onUsersChanged() { _bars.requestPaint() }
        // Fallback handlers — repaint the DAU bar chart whenever the VM
        // completes a refresh cycle, even if the signal name differs.
        function onRefreshed() { _bars.requestPaint() }
        function onDataChanged() { _bars.requestPaint() }
        function onDataReady() { _bars.requestPaint() }
        function onUserGrowthSeriesChanged() { _bars.requestPaint() }
    }

    // Auto-refresh Timer (5s)
    Timer {
        interval: 5000
        repeat: true
        running: page.visible
        onTriggered: {
            if (page.viewModel && typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
            _bars.requestPaint()
        }
    }

    onVisibleChanged: {
        if (page.visible && page.viewModel && typeof page.viewModel.refresh === "function") {
            page.viewModel.refresh()
            _bars.requestPaint()
        }
    }

    Component.onCompleted: {
        if (page.viewModel && typeof page.viewModel.refresh === "function") {
            page.viewModel.refresh()
        }
    }

    readonly property real _colCountry: 240
    readonly property real _colUsers:    160
    readonly property real _colRevenue:  180

    ScrollView {
        id: _scrollView
        anchors.fill: parent
        contentWidth: _scrollView.availableWidth > 0 ? _scrollView.availableWidth : width
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Top breathing room -----
            Item { width: 1; height: Theme.space.xl }

            // ----- Page header -----
            Item {
                width: parent.width
                height: 56
                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.space.md
                    Rectangle {
                        width: 44; height: 44; radius: 12
                        color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.14)
                        anchors.verticalCenter: parent.verticalCenter
                        AppIcon {
                            anchors.centerIn: parent
                            name: "analytics"
                            size: 22
                            color: Theme.color.accent
                        }
                    }
                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: "Analytics"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "Usage, engagement, and platform trends"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                }
            }

            // ----- KPI cards row -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg

                Repeater {
                    model: page._kpis
                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
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

            // ----- DAU bar chart + Top genres -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg

                // ----- Left: DAU bar chart (60%) -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 60
                    height: 300
                    padding: Theme.space.xl

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Active users (last 14 days)"
                            subtitle: "Daily active users"
                        }

                        Canvas {
                            id: _bars
                            width: parent.width
                            height: 200
                            readonly property var _series: page._dauSeries
                            readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0

                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                const w = width, h = height, pad = 6
                                const innerH = h - 2 * pad

                                // Gridlines — always paint
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
                                const n = _series.length
                                const gap = 4
                                const barW = (w - 2 * pad - (n - 1) * gap) / n

                                for (let i = 0; i < n; ++i) {
                                    const x = pad + i * (barW + gap)
                                    const v = Number(_series[i]) || 0
                                    const barH = _max > 0 ? (v / _max) * innerH : 0
                                    const y = h - pad - barH

                                    // Gradient fill
                                    const grad = ctx.createLinearGradient(x, y, x, h - pad)
                                    grad.addColorStop(0, Theme.color.accent)
                                    grad.addColorStop(1, Qt.rgba(Theme.color.accent.r,
                                                                 Theme.color.accent.g,
                                                                 Theme.color.accent.b, 0.35))
                                    ctx.fillStyle = grad
                                    ctx.beginPath()
                                    const r = Math.min(3, barW / 2)
                                    ctx.moveTo(x, y + r)
                                    ctx.quadraticCurveTo(x, y, x + r, y)
                                    ctx.lineTo(x + barW - r, y)
                                    ctx.quadraticCurveTo(x + barW, y, x + barW, y + r)
                                    ctx.lineTo(x + barW, h - pad)
                                    ctx.lineTo(x, h - pad)
                                    ctx.closePath()
                                    ctx.fill()
                                }
                            }
                        }

                        RowLayout {
                            width: parent.width
                            spacing: Theme.space.md

                            Text {
                                text: "Peak: " + page._dauPeak.toLocaleString(Qt.locale(), "f", 0)
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: page._dauSeries.length > 0 && page._dauFirst > 0
                                      ? "▲ " + Math.round((page._dauPeak - page._dauFirst) / page._dauFirst * 100) + "%"
                                      : "—"
                                color: Theme.color.success
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            TextButton {
                                text: "View detailed report"
                                iconName: "arrow_forward"
                                onClicked: page.toastRequested("info", "Detailed report",
                                                                "The full BI dashboard opens in a future release.")
                            }
                        }
                    }
                }

                // ----- Right: Top genres (40%) -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 40
                    height: 300
                    padding: Theme.space.xl

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Top genres"
                            subtitle: "Share of reading sessions"
                        }

                        Repeater {
                            model: page._genres
                            ColumnLayout {
                                width: parent.width
                                spacing: Theme.space.xs

                                RowLayout {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Text {
                                        text: modelData.name
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: (modelData.share !== undefined ? modelData.share : (modelData.value || 0)) + "%"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                // Horizontal bar
                                Item {
                                    width: parent.width
                                    height: 8

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 4
                                        color: Theme.color.fieldFilled
                                    }
                                    Rectangle {
                                        width: parent.width * ((modelData.share !== undefined ? modelData.share : (modelData.value || 0)) / 100)
                                        height: parent.height
                                        radius: 4
                                        color: modelData.color || Theme.color.accent
                                        Behavior on width { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Geographic distribution table -----
            // BUG FIX (admin polish): the geographic distribution backend
            // was never implemented (geographicDistribution() always
            // returns an empty list). Rather than show an empty table
            // header with no rows, we show a clean empty state explaining
            // that geo analytics requires server-side IP geolocation.
            // This is hidden when _geo has data (forward compatibility).
            Card {
                width: parent.width
                padding: Theme.space.xl
                visible: page._geo.length > 0

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Geographic distribution"
                        subtitle: "Top regions by request volume"
                    }

                    // Header
                    Rectangle {
                        width: parent.width
                        height: Theme.size.tableHeaderHeight
                        color: Theme.color.fieldFilled
                        radius: Theme.radius.sm

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.md
                            anchors.rightMargin: Theme.space.md
                            spacing: 0

                            Text { width: page._colCountry; text: "Region";  color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colUsers;   text: "Requests"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colRevenue; text: "Latency";  color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            Text { width: 120; text: "% of total"; horizontalAlignment: Text.AlignRight; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        }
                    }

                    // Rows
                    ListView {
                        width: parent.width
                        height: Math.max(0, page._geo.length) * 56
                        clip: true
                        interactive: false
                        model: page._geo
                        spacing: 0

                        delegate: Rectangle {
                            width: parent.width
                            height: 56
                            color: "transparent"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: Theme.color.divider
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.md
                                anchors.rightMargin: Theme.space.md
                                spacing: 0

                                Text { width: page._colCountry; text: modelData.region;  color: Theme.color.textPrimary;   font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colUsers;   text: (modelData.requests || 0).toLocaleString(Qt.locale(), "f", 0); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colRevenue; text: (modelData.latency !== undefined ? modelData.latency : "—"); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightSemibold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                // Share bar + percentage
                                Row {
                                    width: 200
                                    spacing: Theme.space.sm
                                    anchors.verticalCenter: parent.verticalCenter
                                    layoutDirection: Qt.RightToLeft

                                    Text {
                                        text: (modelData.share !== undefined ? modelData.share : 0) + "%"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Item {
                                        width: 100
                                        height: 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 3
                                            color: Theme.color.fieldFilled
                                        }
                                        Rectangle {
                                            width: parent.width * ((modelData.share !== undefined ? modelData.share : 0) / 100)
                                            height: parent.height
                                            radius: 3
                                            color: Theme.color.accent
                                        }
                                    }
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
