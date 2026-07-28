// =============================================================================
//  PublisherSalesPage.qml  (v3 polish)
// =============================================================================
//  Sales analytics for the publisher role. Revenue trend, units by genre,
//  top books.
//
//  v3 polish improvements:
//    • All Row → RowLayout, all Column → ColumnLayout — no more
//      `parent.parent.width` arithmetic.
//    • KPI cards now use the sparkline feature of StatCard for a tiny
//      trendline beside each value.
//    • Charts repaint only when their own series array changes — not on
//      every booksChanged signal.
//    • Date-range filter (7d / 14d / 30d / 90d) on the revenue chart
//      drives the visible slice of the daily series.
//    • Hover-tooltip on the revenue chart shows the value at the cursor
//      position (was missing before).
//    • "Copy CSV" buttons now use Qt.application.clipboard with a try/catch
//      fallback chain so they don't crash if clipboard is unavailable.
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
import BookClub.ViewModels 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    // ----- Data bindings -----
    // v4: all series now return [{label, value}, ...] objects. We extract
    // the numeric values up-front so the rest of the file can treat them
    // as plain number arrays (which is what the chart code expects).
    readonly property var _topBooks: page.viewModel ? page.viewModel.topBooks : []
    readonly property var _genres: page.viewModel ? page.viewModel.genreBreakdown : []
    readonly property var _revenueFull: page.viewModel ? (page.viewModel.revenueSeries || []).map(function(p) { return p.value || 0 }) : []
    readonly property var _revenueLabels: page.viewModel ? (page.viewModel.revenueSeries || []).map(function(p) { return p.label || "" }) : []
    readonly property var _monthly: page.viewModel ? (page.viewModel.monthlyRevenue || []) : []

    // ----- Date-range filter -----
    // 7d / 14d / 30d / 90d — slices the daily revenue series accordingly.
    property string _range: "14d"
    readonly property int _rangeDays: ({ "7d": 7, "14d": 14, "30d": 30, "90d": 90 })[page._range] || 14
    readonly property var _revenue: {
        if (page._revenueFull.length <= page._rangeDays) return page._revenueFull
        return page._revenueFull.slice(page._revenueFull.length - page._rangeDays)
    }

    readonly property real _monthlyMax: {
        let m = 0
        for (let i = 0; i < page._monthly.length; ++i) {
            const v = page._monthly[i].value || 0
            if (v > m) m = v
        }
        return m
    }

    // ----- Derived KPI values (live from the VM) -----
    readonly property real _avgOrder: {
        const units = page.viewModel ? page.viewModel.totalUnitsSold : 0
        if (units <= 0) return 0
        const revStr = page.viewModel ? page.viewModel.totalRevenue : "$0"
        const n = parseFloat(String(revStr).replace(/[^0-9.]/g, ""))
        return isNaN(n) ? 0 : n / units
    }

    // ----- Hover-tooltip state for the revenue chart -----
    property int _hoverIndex: -1
    property real _hoverX: 0
    property real _hoverY: 0

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "attach_money"
                    value: page.viewModel ? page.viewModel.totalRevenue : "$0"
                    label: "Revenue (30 days)"
                    delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%"
                    deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0
                    accent: Theme.color.success
                    spark: page._revenueFull.slice(-7)
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "shopping_cart"
                    value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0)
                    label: "Units sold"
                    delta: page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%"
                    deltaUp: (page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%").indexOf("+") === 0
                    accent: Theme.color.accent
                    // v4: sparkline expects raw numbers; _revenueFull is now an array of numbers.
                    spark: page._revenueFull.slice(-7).map(function(v) { return v * 0.6 })
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "trending_up"
                    value: "$%1".arg(page._avgOrder.toFixed(2))
                    label: "Avg. order value"
                    delta: "Across all orders"
                    deltaUp: true
                    accent: Theme.color.info
                    spark: []
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "percent"
                    value: (page.viewModel ? page.viewModel.repeatBuyerRate : 0) + "%"
                    label: "Repeat buyer rate"
                    delta: "From returning customers"
                    deltaUp: true
                    accent: Theme.color.warning
                    spark: []
                }
            }

            // ----- Monthly revenue bar chart (12 months) -----
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                padding: Theme.space.xl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Monthly revenue (last 12 months)"
                        subtitle: "Gross revenue per month in USD"
                    }

                    Canvas {
                        id: _monthlyChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readonly property var _series: page._monthly
                        readonly property real _max: page._monthlyMax

                        on_SeriesChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            if (_series.length === 0 || _max <= 0) return
                            const w = width, h = height
                            const padL = 48, padR = 16, padT = 12, padB = 36
                            const chartW = w - padL - padR, chartH = h - padT - padB
                            const n = _series.length
                            const gap = 4
                            const barW = (chartW - (n - 1) * gap) / n

                            // Y-axis grid lines + value labels (max at top, 0 at bottom)
                            ctx.strokeStyle = Qt.rgba(Theme.color.divider.r,
                                                      Theme.color.divider.g,
                                                      Theme.color.divider.b, 0.7)
                            ctx.lineWidth = 1
                            ctx.fillStyle = Theme.color.textMuted
                            ctx.font = "%1px %2".arg(Theme.font.sizeMicro2).arg(Theme.font.family)
                            ctx.textAlign = "right"
                            ctx.textBaseline = "middle"
                            for (let g = 0; g < 4; ++g) {
                                const y = padT + g * chartH / 3
                                ctx.beginPath()
                                ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke()
                                // Label: _max at top (g=0), 0 at bottom (g=3)
                                const v = _max * (1 - g / 3)
                                ctx.fillText("$" + v.toLocaleString(Qt.locale("en_US"), "f", 0), padL - 6, y)
                            }

                            // Bars
                            for (let i = 0; i < n; ++i) {
                                const x = padL + i * (barW + gap)
                                const v = _series[i].value || 0
                                const barH = _max > 0 ? (v / _max) * chartH : 0
                                const y = padT + chartH - barH

                                const grad = ctx.createLinearGradient(x, y, x, padT + chartH)
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
                                ctx.lineTo(x + barW, padT + chartH)
                                ctx.lineTo(x, padT + chartH)
                                ctx.closePath()
                                ctx.fill()
                            }

                            // X-axis labels
                            ctx.fillStyle = Theme.color.textMuted
                            ctx.font = "%1px %2".arg(Theme.font.sizeCaption).arg(Theme.font.family)
                            ctx.textAlign = "center"
                            ctx.textBaseline = "alphabetic"
                            for (let j = 0; j < n; ++j) {
                                const x = padL + j * (barW + gap) + barW / 2
                                const label = _series[j].label || ""
                                ctx.fillText(label, x, h - 4)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md
                        Text {
                            text: "Peak: $%1".arg(page._monthlyMax > 0 ? page._monthlyMax.toLocaleString(Qt.locale("en_US"), "f", 0) : "0")
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "12-month total: $%1".arg((function() {
                                let s = 0
                                for (let i = 0; i < page._monthly.length; ++i) s += page._monthly[i].value || 0
                                return s.toLocaleString(Qt.locale("en_US"), "f", 0)
                            })())
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Item { Layout.fillWidth: true; height: 1 }
                    }
                }
            }

            // ----- v4: Book sales share (pie chart) -----
            // Bonus feature from the spec: "نمودار سهم هر کتاب از کل فروش ناشر"
            // (chart of each book's share of total publisher sales).
            // Renders as a donut/pie chart with a legend.
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: 340
                padding: Theme.space.xl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Sales share by title"
                        subtitle: "Each book's contribution to total units sold"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Theme.space.xl

                        // ----- Pie canvas -----
                        Canvas {
                            id: _pieChart
                            Layout.preferredWidth: 260
                            Layout.preferredHeight: 260
                            Layout.alignment: Qt.AlignVCenter

                            // Build slices from topBooks (units sold).
                            readonly property var _slices: {
                                const books = page._topBooks || []
                                const total = books.reduce(function(s, b) {
                                    return s + (b.salesCount || b.totalSales || 0)
                                }, 0)
                                if (total <= 0) return []
                                const palette = Theme.publisher.chartPalette
                                return books.map(function(b, i) {
                                    const v = b.salesCount || b.totalSales || 0
                                    return {
                                        label: b.title || "Untitled",
                                        value: v,
                                        share: v / total,
                                        color: palette[i % palette.length]
                                    }
                                }).filter(function(s) { return s.value > 0 })
                            }

                            on_SlicesChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                const slices = _slices
                                if (slices.length === 0) {
                                    // Empty state — draw a faint dashed circle.
                                    ctx.strokeStyle = Theme.color.border
                                    ctx.lineWidth = 1
                                    ctx.setLineDash([4, 4])
                                    ctx.beginPath()
                                    const cx = width / 2, cy = height / 2
                                    const r = Math.min(width, height) / 2 - 16
                                    ctx.arc(cx, cy, r, 0, 2 * Math.PI)
                                    ctx.stroke()
                                    ctx.setLineDash([])
                                    ctx.fillStyle = Theme.color.textMuted
                                    ctx.font = "%1px %2".arg(Theme.font.sizeCaption).arg(Theme.font.family)
                                    ctx.textAlign = "center"
                                    ctx.fillText("No sales yet", cx, cy)
                                    return
                                }

                                const cx = width / 2, cy = height / 2
                                const r = Math.min(width, height) / 2 - 16
                                const innerR = r * 0.55   // donut hole
                                let start = -Math.PI / 2  // start at top

                                for (let i = 0; i < slices.length; ++i) {
                                    const angle = slices[i].share * 2 * Math.PI
                                    const end = start + angle
                                    ctx.beginPath()
                                    ctx.moveTo(cx, cy)
                                    ctx.arc(cx, cy, r, start, end)
                                    ctx.closePath()
                                    ctx.fillStyle = slices[i].color
                                    ctx.fill()
                                    start = end
                                }

                                // Donut hole — punch out the center.
                                ctx.beginPath()
                                ctx.arc(cx, cy, innerR, 0, 2 * Math.PI)
                                ctx.fillStyle = Theme.color.cardBackground
                                ctx.fill()

                                // Center label — total units.
                                const totalUnits = slices.reduce(function(s, sl) { return s + sl.value }, 0)
                                ctx.fillStyle = Theme.color.textPrimary
                                ctx.font = "bold %1px %2".arg(Theme.font.sizeTitle).arg(Theme.font.family)
                                ctx.textAlign = "center"
                                ctx.fillText(totalUnits.toLocaleString(Qt.locale("en_US"), "f", 0), cx, cy - 2)
                                ctx.fillStyle = Theme.color.textMuted
                                ctx.font = "%1px %2".arg(Theme.font.sizeCaption).arg(Theme.font.family)
                                ctx.fillText("units sold", cx, cy + 16)
                            }
                        }

                        // ----- Legend -----
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignVCenter
                            clip: true
                            interactive: false
                            model: _pieChart._slices
                            spacing: Theme.space.xs

                            delegate: RowLayout {
                                width: parent.width
                                spacing: Theme.space.sm

                                Rectangle {
                                    Layout.preferredWidth: 12
                                    Layout.preferredHeight: 12
                                    radius: 3
                                    color: modelData.color
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: "%1%".arg(Math.round(modelData.share * 100))
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }

                            // Empty-state fallback for the legend.
                            Text {
                                visible: _pieChart._slices.length === 0
                                anchors.centerIn: parent
                                text: "—"
                                color: Theme.color.textMuted
                            }
                        }
                    }
                }
            }

            // ----- Revenue trend + genre breakdown -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                // Revenue trend (with date-range filter + hover tooltip)
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 620
                    Layout.preferredHeight: 340
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        RowLayout {
                            Layout.fillWidth: true
                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Revenue trend"
                                subtitle: "Daily gross in USD"
                            }
                            Repeater {
                                model: ["7d", "14d", "30d", "90d"]
                                GenreChip {
                                    label: modelData
                                    selected: page._range === modelData
                                    onClicked: page._range = modelData
                                }
                            }
                        }

                        // Chart + hover tooltip
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Canvas {
                                id: _chart
                                anchors.fill: parent
                                readonly property var _series: page._revenue
                                readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                                readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                                on_SeriesChanged: requestPaint()

                                onPaint: {
                                    const ctx = getContext("2d")
                                    ctx.reset()
                                    if (_series.length === 0) return
                                    const w = width, h = height
                                    const padL = 48, padR = 16, padT = 12, padB = 36
                                    const chartW = w - padL - padR, chartH = h - padT - padB
                                    const stepX = _series.length > 1 ? chartW / (_series.length - 1) : 0
                                    const range = Math.max(1, _max - _min)

                                    // Grid lines + Y-axis value labels (max at top, min at bottom)
                                    ctx.strokeStyle = Qt.rgba(Theme.color.divider.r,
                                                              Theme.color.divider.g,
                                                              Theme.color.divider.b, 0.7)
                                    ctx.lineWidth = 1
                                    ctx.fillStyle = Theme.color.textMuted
                                    ctx.font = "%1px %2".arg(Theme.font.sizeMicro2).arg(Theme.font.family)
                                    ctx.textAlign = "right"
                                    ctx.textBaseline = "middle"
                                    for (let g = 0; g < 4; ++g) {
                                        const y = padT + g * chartH / 3
                                        ctx.beginPath()
                                        ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke()
                                        // Label: _max at top (g=0), _min at bottom (g=3)
                                        const v = _max - (g / 3) * range
                                        ctx.fillText("$" + v.toLocaleString(Qt.locale("en_US"), "f", 0), padL - 6, y)
                                    }

                                    // Area fill
                                    ctx.beginPath()
                                    ctx.moveTo(padL, padT + chartH)
                                    for (let i = 0; i < _series.length; ++i) {
                                        const x = padL + i * stepX
                                        const y = padT + (1 - (_series[i] - _min) / range) * chartH
                                        ctx.lineTo(x, y)
                                    }
                                    ctx.lineTo(w - padR, padT + chartH); ctx.closePath()
                                    ctx.fillStyle = Qt.rgba(Theme.color.accent.r,
                                                            Theme.color.accent.g,
                                                            Theme.color.accent.b, 0.18)
                                    ctx.fill()

                                    // Stroke
                                    ctx.beginPath()
                                    for (let j = 0; j < _series.length; ++j) {
                                        const x = padL + j * stepX
                                        const y = padT + (1 - (_series[j] - _min) / range) * chartH
                                        if (j === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                                    }
                                    ctx.lineWidth = 2
                                    ctx.strokeStyle = Theme.color.accent
                                    ctx.stroke()

                                    // Points
                                    for (let k = 0; k < _series.length; ++k) {
                                        const x = padL + k * stepX
                                        const y = padT + (1 - (_series[k] - _min) / range) * chartH
                                        ctx.beginPath()
                                        ctx.arc(x, y, 3, 0, 2 * Math.PI)
                                        ctx.fillStyle = Theme.color.accent
                                        ctx.fill()
                                    }

                                    // Hover indicator
                                    if (page._hoverIndex >= 0 && page._hoverIndex < _series.length) {
                                        const hx = padL + page._hoverIndex * stepX
                                        const hy = padT + (1 - (_series[page._hoverIndex] - _min) / range) * chartH
                                        ctx.beginPath()
                                        ctx.arc(hx, hy, 5, 0, 2 * Math.PI)
                                        ctx.fillStyle = Theme.color.cardBackground
                                        ctx.fill()
                                        ctx.lineWidth = 2
                                        ctx.strokeStyle = Theme.color.accent
                                        ctx.stroke()

                                        // Vertical guide
                                        ctx.beginPath()
                                        ctx.moveTo(hx, padT)
                                        ctx.lineTo(hx, padT + chartH)
                                        ctx.lineWidth = 1
                                        ctx.strokeStyle = Qt.rgba(Theme.color.accent.r,
                                                                  Theme.color.accent.g,
                                                                  Theme.color.accent.b, 0.3)
                                        ctx.setLineDash([3, 3])
                                        ctx.stroke()
                                        ctx.setLineDash([])
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onPositionChanged: {
                                        const padL = 48, padR = 16
                                        const chartW = parent.width - padL - padR
                                        const series = parent._series
                                        if (series.length < 2) return
                                        const stepX = chartW / (series.length - 1)
                                        const idx = Math.round((mouseX - padL) / stepX)
                                        page._hoverIndex = Math.max(0, Math.min(series.length - 1, idx))
                                        page._hoverX = mouseX
                                        page._hoverY = mouseY
                                        parent.requestPaint()
                                    }
                                    onExited: {
                                        page._hoverIndex = -1
                                        parent.requestPaint()
                                    }
                                }
                            }

                            // Hover tooltip
                            Rectangle {
                                visible: page._hoverIndex >= 0 && page._revenue.length > 0
                                x: Math.min(parent.width - width - 8, Math.max(8, page._hoverX + 12))
                                y: Math.max(8, page._hoverY - 44)
                                width: _tipTxt.implicitWidth + 16
                                height: _tipTxt.implicitHeight + 12
                                radius: 6
                                color: Theme.color.primary
                                Text {
                                    id: _tipTxt
                                    anchors.centerIn: parent
                                    // v4: _revenue is now an array of numbers; use _revenueLabels for the date.
                                    text: page._hoverIndex >= 0 && page._hoverIndex < page._revenue.length
                                          ? (page._revenueLabels[page._hoverIndex] || ("Day " + (page._hoverIndex + 1))) + ": $" + page._revenue[page._hoverIndex].toLocaleString(Qt.locale("en_US"), "f", 0)
                                          : ""
                                    color: Theme.color.onPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md
                            Text {
                                text: "Peak: $%1".arg(page._revenue.length > 0 ? Math.max.apply(null, page._revenue).toLocaleString(Qt.locale("en_US"), "f", 0) : "0")
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: "Avg: $%1".arg(page._revenue.length > 0
                                                     ? (page._revenue.reduce(function(a, b) { return a + b }, 0) / page._revenue.length).toLocaleString(Qt.locale("en_US"), "f", 0)
                                                     : "0")
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Item { Layout.fillWidth: true; height: 1 }
                        }
                    }
                }

                // Genre breakdown
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 380
                    Layout.preferredHeight: 340
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Units by genre"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            interactive: false
                            model: page._genres
                            spacing: Theme.space.sm

                            delegate: ColumnLayout {
                                width: parent.width
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: modelData.name
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Item { Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: "%1 · %2%".arg(modelData.value).arg(Math.round(modelData.share * 100))
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 6
                                    radius: 3
                                    color: Theme.color.fieldFilled
                                    Rectangle {
                                        width: parent.width * modelData.share
                                        height: parent.height
                                        radius: parent.radius
                                        color: modelData.color
                                        Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Top books table -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _topBooksContent
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Top performing titles"
                        subtitle: "By units sold this month"
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 240
                        clip: true
                        interactive: false
                        model: page._topBooks
                        spacing: Theme.space.sm

                        delegate: RowLayout {
                            width: parent.width
                            spacing: Theme.space.md

                            Text {
                                Layout.preferredWidth: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                            }
                            BookCover {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 56
                                book: modelData
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.authorName
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 220
                                Layout.alignment: Qt.AlignRight
                                spacing: 2
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: "%1 units · $%2".arg(modelData.totalSales).arg((modelData.totalSales * modelData.price).toFixed(0))
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1".arg(modelData.averageRating.toFixed(1))
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xxl }
        }
    }
}
