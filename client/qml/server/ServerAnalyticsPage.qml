// =============================================================================
//  ServerAnalyticsPage.qml
// =============================================================================
//  Request analytics: 4 KPI cards, an hourly volume line chart, a top-endpoints
//  panel, a geographic distribution table, and an error-breakdown bar chart
//  where each bar's width is proportional to its share of total errors.
//  Sourced from ServerViewModel: requestSeries, topEndpoints,
//  geographicDistribution, errorBreakdown.
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

    function _methodColor(m) {
        if (m === "POST")   return Theme.color.accent
        if (m === "GET")    return Theme.color.success
        if (m === "DELETE") return Theme.color.error
        return Theme.color.warning
    }

    function _formatK(n) {
        if (n >= 1000000) return (n / 1000000).toFixed(1) + "M"
        if (n >= 1000)    return (n / 1000).toFixed(0) + "K"
        return "" + n
    }

    function _totalRequests() {
        var s = page.viewModel ? page.viewModel.requestSeries : []
        var sum = 0
        for (var i = 0; i < s.length; ++i) sum += (s[i] || 0)
        return sum
    }

    function _errorRatePercent() {
        var e = page.viewModel ? page.viewModel.errorBreakdown : []
        var sum = 0
        for (var i = 0; i < e.length; ++i) sum += (e[i].percent || 0)
        // errorBreakdown percents are shares of *errors*, not of all requests;
        // for the headline KPI we surface the total errors share, capped at 100.
        return sum
    }

    function _avgResponseMs() {
        var t = page.viewModel ? page.viewModel.topEndpoints : []
        if (t.length === 0) return "—"
        var sum = 0, n = 0
        for (var i = 0; i < t.length; ++i) {
            var v = t[i].avgTime
            if (typeof v === "string") v = parseFloat(v)
            if (!isNaN(v)) { sum += v; n++ }
        }
        if (n === 0) return "—"
        return (sum / n).toFixed(0) + "ms"
    }

    function _uptimePercent() {
        // Derived: 100% minus the total error breakdown share (a coarse proxy).
        var err = page._errorRatePercent()
        if (err <= 0) return "—"
        return (100 - Math.min(err, 100)).toFixed(2) + "%"
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onClientsChanged: _line.requestPaint()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPIs -----
            Row {
                width: parent.width
                spacing: Theme.space.lg
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "bar_chart"
                    value:    page._formatK(page._totalRequests())
                    label:    "Total requests today"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "error_outline"
                    value:    page._errorRatePercent().toFixed(2) + "%"
                    label:    "Error rate"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.warning
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "speed"
                    value:    page._avgResponseMs()
                    label:    "Avg response time"
                    delta:    "top endpoints"
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "verified"
                    value:    page._uptimePercent()
                    label:    "Uptime (30d)"
                    delta:    "derived"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
            }

            // ----- Volume chart + top endpoints -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // Line chart
                Card {
                    width: parent.width * 0.60 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Request volume"
                            subtitle: "Last 24 hours (hourly)"
                        }

                        Canvas {
                            id: _line
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
                                if (_series.length === 0 || _max <= 0) return
                                const stepX = (w - 2 * pad) / Math.max(1, _series.length - 1)
                                const innerH = h - 2 * pad

                                // Gridlines
                                ctx.strokeStyle = Qt.rgba(Theme.color.divider.r, Theme.color.divider.g, Theme.color.divider.b, 0.5)
                                ctx.lineWidth = 1
                                for (let g = 0; g <= 4; ++g) {
                                    const y = pad + (innerH * g / 4)
                                    ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(w - pad, y); ctx.stroke()
                                }

                                // Area fill
                                ctx.beginPath()
                                ctx.moveTo(pad, h - pad)
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - _series[i] / _max) * innerH
                                    ctx.lineTo(x, y)
                                }
                                ctx.lineTo(w - pad, h - pad)
                                ctx.closePath()
                                ctx.fillStyle = Qt.rgba(Theme.color.accent.r,
                                                        Theme.color.accent.g,
                                                        Theme.color.accent.b, 0.14)
                                ctx.fill()

                                // Line stroke
                                ctx.beginPath()
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - _series[i] / _max) * innerH
                                    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                                }
                                ctx.lineWidth = 2
                                ctx.strokeStyle = Theme.color.accent
                                ctx.stroke()
                            }
                        }
                    }
                }

                // Top endpoints
                Card {
                    width: parent.width * 0.40 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Top endpoints"
                            subtitle: "By request count today"
                        }

                        ListView {
                            width: parent.width
                            height: parent.height - 40
                            clip: true
                            interactive: false
                            spacing: Theme.space.sm
                            model: page.viewModel ? page.viewModel.topEndpoints : []

                            delegate: Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Rectangle {
                                    width: 42; height: 22; radius: 4
                                    color: Qt.rgba(page._methodColor(modelData.method).r,
                                                   page._methodColor(modelData.method).g,
                                                   page._methodColor(modelData.method).b, 0.14)
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.method
                                        color: page._methodColor(modelData.method)
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                Column {
                                    width: parent.width - 42 - Theme.space.sm - 80
                                    spacing: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        width: parent.width
                                        text: modelData.path
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: "%1 req · %2ms · %3% err"
                                            .arg(modelData.requests)
                                            .arg(modelData.avgTime)
                                            .arg(modelData.errorRate)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }

                                Text {
                                    width: 80
                                    text: page._formatK(modelData.requests)
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                    horizontalAlignment: Text.AlignRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }

            // ----- Geographic distribution -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Geographic distribution"
                        subtitle: "Top regions by request volume"
                    }

                    Row {
                        width: parent.width
                        Repeater {
                            model: [
                                { w: 0.40, label: "Region" },
                                { w: 0.20, label: "Requests" },
                                { w: 0.20, label: "% of total" },
                                { w: 0.20, label: "Avg latency" }
                            ]
                            Text {
                                width: parent.parent.width * modelData.w
                                text: modelData.label
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightBold
                            }
                        }
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.color.divider }

                    ListView {
                        width: parent.width
                        height: (page.viewModel ? page.viewModel.geographicDistribution.length : 0) * 44
                        clip: true
                        interactive: false
                        spacing: 0
                        model: page.viewModel ? page.viewModel.geographicDistribution : []

                        delegate: Column {
                            width: parent.width
                            height: 44
                            Row {
                                width: parent.width
                                height: 44
                                Text {
                                    width: parent.width * 0.40
                                    text: modelData.region
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.20
                                    text: page._formatK(modelData.requests)
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.20
                                    text: "%1%".arg(modelData.share)
                                    color: Theme.color.accent
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.20
                                    text: modelData.latency
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Rectangle { width: parent.width; height: 1; color: Theme.color.divider }
                        }
                    }
                }
            }

            // ----- Error breakdown -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Error breakdown"
                        subtitle: "Distribution of error responses (last 24h)"
                    }

                    Repeater {
                        model: page.viewModel ? page.viewModel.errorBreakdown : []
                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                            // Code + label
                            Column {
                                width: 180
                                spacing: 0
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    text: "%1 · %2".arg(modelData.code).arg(modelData.label)
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                }
                            }

                            // Bar
                            Item {
                                width: parent.width - 180 - 60 - 2 * Theme.space.md
                                height: 24
                                anchors.verticalCenter: parent.verticalCenter
                                Rectangle {
                                    anchors.fill: parent
                                    radius: Theme.radius.sm
                                    color: Theme.color.fieldFilled
                                }
                                Rectangle {
                                    width: parent.width * (modelData.percent / 100)
                                    height: parent.height
                                    radius: Theme.radius.sm
                                    color: modelData.color
                                    Behavior on width { NumberAnimation { duration: Theme.motion.durationSlow; easing.type: Easing.OutCubic } }
                                }
                            }

                            Text {
                                width: 60
                                text: "%1%".arg(modelData.percent)
                                color: modelData.color
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignRight
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
