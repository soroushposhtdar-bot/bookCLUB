// =============================================================================
//  PublisherSalesPage.qml
// =============================================================================
//  Sales analytics for the publisher role. Revenue trend, units by genre,
//  top books, and geographic distribution.
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

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)

    // ----- Top books (QVariantList from the VM) -----
    readonly property var _topBooks: page.viewModel ? page.viewModel.topBooks : []

    // ----- Genre breakdown (QVariantList from the VM; each entry has
    //       .name / .share / .color / .value) -----
    readonly property var _genres: page.viewModel ? page.viewModel.genreBreakdown : []

    // ----- Geographic distribution — bound to viewModel.geographicBreakdown -----
    readonly property var _regions: page.viewModel ? (page.viewModel.geographicBreakdown || []) : []

    // ----- Revenue series (last 14 days) — QVariantList from the VM -----
    readonly property var _revenue: page.viewModel ? (page.viewModel.revenueSeries || []) : []

    // ----- Monthly revenue (last 12 months) — QVariantList from the VM -----
    readonly property var _monthly: page.viewModel ? (page.viewModel.monthlyRevenue || []) : []
    readonly property real _monthlyMax: {
        let m = 0
        for (let i = 0; i < page._monthly.length; ++i) {
            const v = page._monthly[i].value || 0
            if (v > m) m = v
        }
        return m
    }

    // ----- Derived KPI values (live from the VM) -----
    //   Avg. order value = total revenue / total units (with safe fallbacks).
    //   Repeat buyer rate is synthesized until the VM exposes it natively.
    readonly property real _avgOrder: {
        const units = page.viewModel ? page.viewModel.totalUnitsSold : 0
        if (units <= 0) return 0
        // Strip the "$" and any thousands separators from totalRevenue.
        const revStr = page.viewModel ? page.viewModel.totalRevenue : "$0"
        const n = parseFloat(String(revStr).replace(/[^0-9.]/g, ""))
        return isNaN(n) ? 0 : n / units
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards row — bound to live VM data -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard { width: (parent.width - 3 * Theme.space.lg) / 4; iconName: "attach_money";  value: page.viewModel ? page.viewModel.totalRevenue : "$0"; label: "Revenue (30 days)"; delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%"; deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0; accent: Theme.color.success }
                StatCard { width: (parent.width - 3 * Theme.space.lg) / 4; iconName: "shopping_cart"; value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0); label: "Units sold";        delta: page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%"; deltaUp: (page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%").indexOf("+") === 0;  accent: Theme.color.accent  }
                StatCard { width: (parent.width - 3 * Theme.space.lg) / 4; iconName: "trending_up";    value: "$%1".arg(page._avgOrder.toFixed(2)); label: "Avg. order value";  delta: "Across all orders";                  deltaUp: true;  accent: Theme.color.info    }
                StatCard { width: (parent.width - 3 * Theme.space.lg) / 4; iconName: "percent";       value: (page.viewModel ? page.viewModel.repeatBuyerRate : 0) + "%";    label: "Repeat buyer rate"; delta: "From returning customers";                   deltaUp: true;  accent: Theme.color.warning }
            }

            // ----- Monthly revenue bar chart (12 months) -----
            Card {
                width: parent.width
                height: 280
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Monthly revenue (last 12 months)"
                        subtitle: "Gross revenue per month in USD"
                    }

                    Canvas {
                        id: _monthlyChart
                        width: parent.width
                        height: 200
                        readonly property var _series: page._monthly
                        readonly property real _max: page._monthlyMax

                        Connections {
                            target: page.viewModel
                            ignoreUnknownSignals: true
                            onBooksChanged: _monthlyChart.requestPaint()
                        }

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            if (_series.length === 0 || _max <= 0) return
                            const w = width, h = height, pad = 24
                            const n = _series.length
                            const gap = 4
                            const barW = (w - 2 * pad - (n - 1) * gap) / n

                            // Y-axis grid lines (4 horizontal)
                            ctx.strokeStyle = Qt.rgba(Theme.color.divider.r,
                                                      Theme.color.divider.g,
                                                      Theme.color.divider.b, 0.5)
                            ctx.lineWidth = 1
                            for (let g = 0; g < 4; ++g) {
                                const y = pad + g * (h - 2 * pad) / 3
                                ctx.beginPath()
                                ctx.moveTo(pad, y); ctx.lineTo(w - pad, y); ctx.stroke()
                            }

                            // Bars
                            for (let i = 0; i < n; ++i) {
                                const x = pad + i * (barW + gap)
                                const v = _series[i].value || 0
                                const barH = _max > 0 ? (v / _max) * (h - 2 * pad) : 0
                                const y = h - pad - barH

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

                            // X-axis labels (month abbreviations)
                            ctx.fillStyle = Theme.color.textMuted
                            ctx.font = "%1px %2".arg(Theme.font.sizeCaption).arg(Theme.font.family)
                            ctx.textAlign = "center"
                            for (let j = 0; j < n; ++j) {
                                const x = pad + j * (barW + gap) + barW / 2
                                const label = _series[j].label || ""
                                ctx.fillText(label, x, h - 4)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md
                        Text {
                            text: "Peak: $%1".arg(page._monthlyMax > 0 ? page._monthlyMax.toFixed(0) : "0")
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "12-month total: $%1".arg((function() {
                                let s = 0
                                for (let i = 0; i < page._monthly.length; ++i) s += page._monthly[i].value || 0
                                return s.toFixed(0)
                            })())
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                        TextButton { text: "Copy CSV"; iconName: "download"; onClicked: {
                            var csv = "Month,Revenue\n"
                            for (var i = 0; i < page._monthly.length; ++i) {
                                csv += page._monthly[i].label + "," + page._monthly[i].value.toFixed(2) + "\n"
                            }
                            try { if (typeof Qt.application !== "undefined" && Qt.application.clipboard) Qt.application.clipboard.setText(csv) } catch(e) {}
                            page.toastRequested("success", "CSV copied", page._monthly.length + " months copied to clipboard.")
                        } }
                    }
                }
            }

            // ----- Revenue trend + genre breakdown -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                Card {
                    width: parent.width * 0.62 - Theme.space.lg / 2
                    height: 320
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Revenue (last 14 days)"
                            subtitle: "Daily gross in USD"
                        }

                        Canvas {
                            id: _chart
                            width: parent.width
                            height: 220
                            readonly property var _series: page._revenue
                            readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                            readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                            Connections {
                                target: page.viewModel
                                ignoreUnknownSignals: true
                                onBooksChanged: _chart.requestPaint()
                            }

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                const w = width, h = height, pad = 10
                                const stepX = (w - 2 * pad) / (_series.length - 1)
                                const range = Math.max(1, _max - _min)

                                // Grid lines
                                ctx.strokeStyle = Qt.rgba(Theme.color.divider.r,
                                                          Theme.color.divider.g,
                                                          Theme.color.divider.b, 0.5)
                                ctx.lineWidth = 1
                                for (let g = 0; g < 4; ++g) {
                                    const y = pad + g * (h - 2 * pad) / 3
                                    ctx.beginPath()
                                    ctx.moveTo(pad, y); ctx.lineTo(w - pad, y); ctx.stroke()
                                }

                                // Area fill
                                ctx.beginPath()
                                ctx.moveTo(pad, h - pad)
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                    ctx.lineTo(x, y)
                                }
                                ctx.lineTo(w - pad, h - pad); ctx.closePath()
                                ctx.fillStyle = Qt.rgba(Theme.color.accent.r,
                                                        Theme.color.accent.g,
                                                        Theme.color.accent.b, 0.18)
                                ctx.fill()

                                // Stroke
                                ctx.beginPath()
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                                }
                                ctx.lineWidth = 2
                                ctx.strokeStyle = Theme.color.accent
                                ctx.stroke()

                                // Points
                                for (let i = 0; i < _series.length; ++i) {
                                    const x = pad + i * stepX
                                    const y = pad + (1 - (_series[i] - _min) / range) * (h - 2 * pad)
                                    ctx.beginPath()
                                    ctx.arc(x, y, 3, 0, 2 * Math.PI)
                                    ctx.fillStyle = Theme.color.accent
                                    ctx.fill()
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            spacing: Theme.space.md
                            Text { text: "Peak: $%1".arg(page._chart._max > 0 ? page._chart._max.toFixed(0) : "0"); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold }
                            Text { text: "Avg: $%1".arg(page._chart._series.length > 0 ? (page._chart._series.reduce(function(a, b) { return a + b }, 0) / page._chart._series.length).toFixed(0) : "0");  color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            TextButton { text: "Copy CSV"; iconName: "download"; onClicked: {
                                var csv = "Day,Revenue\n"
                                for (var i = 0; i < page._revenue.length; ++i) {
                                    csv += "Day " + (i+1) + "," + page._revenue[i].toFixed(2) + "\n"
                                }
                                try { if (typeof Qt.application !== "undefined" && Qt.application.clipboard) Qt.application.clipboard.setText(csv) } catch(e) {}
                                page.toastRequested("success", "CSV copied", page._revenue.length + " days copied to clipboard.")
                            } }
                        }
                    }
                }

                Card {
                    width: parent.width * 0.38 - Theme.space.lg / 2
                    height: 320
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader { width: parent.width; title: "Units by genre" }

                        ListView {
                            width: parent.width
                            height: parent.height - 40
                            clip: true
                            interactive: false
                            model: page._genres
                            spacing: Theme.space.sm

                            delegate: Column {
                                width: parent.width
                                spacing: 4

                                Row {
                                    width: parent.width
                                    Text { text: modelData.name; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text { text: "%1 · %2%".arg(modelData.value).arg(Math.round(modelData.share * 100)); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                                }
                                Rectangle {
                                    width: parent.width
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
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader { width: parent.width; title: "Top performing titles"; subtitle: "By units sold this month" }

                    ListView {
                        width: parent.width
                        height: 240
                        clip: true
                        interactive: false
                        model: page._topBooks
                        spacing: Theme.space.sm

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Text {
                                width: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            BookCover { width: 40; height: 56; book: modelData; anchors.verticalCenter: parent.verticalCenter }
                            Column {
                                width: parent.width - 28 - 40 - Theme.space.md * 2 - 220
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter
                                Text { width: parent.width; text: modelData.title; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold; elide: Text.ElideRight }
                                Text { text: modelData.authorName; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                            }
                            Column {
                                width: 220
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter
                                Text { text: "%1 units · $%2".arg(modelData.totalSales).arg((modelData.totalSales * modelData.price).toFixed(0)); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold; horizontalAlignment: Text.AlignRight; anchors.right: parent.right }
                                Row {
                                    anchors.right: parent.right
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text { text: "%1".arg(modelData.averageRating.toFixed(1)); color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; anchors.verticalCenter: parent.verticalCenter }
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

                    SectionHeader { width: parent.width; title: "Geographic distribution"; subtitle: "Revenue by region (last 30 days)" }

                    ListView {
                        width: parent.width
                        height: 240
                        clip: true
                        interactive: false
                        model: page._regions
                        spacing: Theme.space.sm

                        delegate: Column {
                            width: parent.width
                            spacing: 4
                            Row {
                                width: parent.width
                                Text { text: modelData.name; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium }
                                Item { width: 1; Layout.fillWidth: true; height: 1 }
                                Text { text: modelData.revenueText || ("$" + Math.round(modelData.revenue || 0)); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold }
                                Text { text: "%1%".arg(Math.round(modelData.share * 100)); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; leftPadding: 8 }
                            }
                            Rectangle {
                                width: parent.width; height: 6; radius: 3; color: Theme.color.fieldFilled
                                Rectangle { width: parent.width * modelData.share; height: parent.height; radius: parent.radius; color: Theme.color.accent; Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } } }
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
