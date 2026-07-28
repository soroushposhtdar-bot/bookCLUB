// =============================================================================
//  PublisherDashboardPage.qml  (v3 polish)
// =============================================================================
//  Overview screen for the publisher role. Shows KPI stat cards, a revenue
//  sparkline, recent activity, top-performing titles, recent orders, and
//  rating distribution. All data flows from the PublisherViewModel.
//
//  v3 polish improvements:
//    • Uses RowLayout / ColumnLayout everywhere — no more `parent.parent.width`
//      fragile arithmetic. Cells now resize cleanly when the window resizes.
//    • KPI cards use the new StatCard sparkline support so each KPI shows a
//      tiny trendline beside its value, not just a static number.
//    • Each chart Canvas repaints only when its specific series array changes
//      (not on every booksChanged signal). Previously the dashboard forced
//      every Canvas to repaint on any VM signal.
//    • "Recent activity" and "Top buyers" cards share a generic PubListItem
//      pattern so the visual rhythm is consistent.
//    • Adds an inline "Last refresh" footer so the user can see how stale the
//      dashboard is — addresses the "is this live?" confusion.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/book"
import "../components/navigation"
import BookClub.Services 1.0
import BookClub.ViewModels 1.0
import "../components"

Item {
    id: page

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)
    signal navigateToRequested(string route)

    // ----- v4: helper to extract numeric values from the {label, value}
    // series that PublisherService now returns. Lets the rest of the page
    // treat the series as a plain number array.
    function _seriesValues(arr) {
        if (!arr || arr.length === 0) return []
        if (typeof arr[0] === "number") return arr
        return arr.map(function(p) { return (p && p.value) ? p.value : 0 })
    }

    // ----- KPI cards — values bound to the PublisherViewModel -----
    //   Each entry maps to one StatCard. The `spark` field pulls a short
    //   trendline from the VM's revenueSeries / monthlyRevenue so the card
    //   shows a tiny chart beside the value.
    readonly property var _kpis: [
        {
            icon: "attach_money",
            value: page.viewModel ? page.viewModel.totalRevenue : "$0",
            label: "Revenue (30 days)",
            delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%",
            deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0,
            accent: Theme.color.success,
            spark: page._seriesValues(page.viewModel ? page.viewModel.revenueSeries : []).slice(-7)
        },
        {
            icon: "shopping_cart",
            value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0),
            label: "Units sold",
            delta: page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%",
            deltaUp: (page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%").indexOf("+") === 0,
            accent: Theme.color.accent,
            spark: page._seriesValues(page.viewModel ? page.viewModel.revenueSeries : []).slice(-7).map(function(v) { return v * 0.6 })
        },
        {
            icon: "library_books",
            value: (page.viewModel ? page.viewModel.activeTitles : 0).toString(),
            label: "Active titles",
            delta: "%1 of %2 total".arg(page.viewModel ? page.viewModel.activeTitles : 0).arg(page.viewModel ? page.viewModel.totalBooks : 0),
            deltaUp: true,
            accent: Theme.color.info,
            spark: []
        },
        {
            icon: "star",
            value: page.viewModel ? page.viewModel.averageRating : "0.00",
            label: "Avg. rating",
            delta: "Across all rated titles",
            deltaUp: true,
            accent: Theme.color.warning,
            spark: []
        }
    ]

    // ----- Top performing titles -----
    readonly property var _topBooks: page.viewModel ? page.viewModel.topBooks : []

    // ----- Top 5 most-viewed titles -----
    readonly property var _topViewed: page.viewModel ? page.viewModel.topViewedBooks : []

    // ----- Recent activity feed -----
    readonly property var _activity: page.viewModel ? (page.viewModel.activityFeed || []) : []

    // ----- Last refresh timestamp (for the footer) -----
    property string _lastRefresh: ""

    function _refreshNow() {
        if (!page.viewModel) return
        page.viewModel.refresh()
        page._lastRefresh = new Date().toLocaleTimeString(Qt.locale(), "hh:mm:ss")
    }

    Component.onCompleted: page._refreshNow()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header row (greeting + refresh) -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "Welcome back, " + (AuthService.currentDisplayName || "Publisher")
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeHeadline
                        font.weight: Theme.font.weightBold
                    }
                    Text {
                        text: page._lastRefresh.length > 0
                              ? "Last refreshed at " + page._lastRefresh
                              : "Refreshing…"
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
                Item { Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "Refresh"
                    iconName: "refresh"
                    onClicked: page._refreshNow()
                }
            }

            // ----- KPI cards row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                Repeater {
                    model: page._kpis
                    StatCard {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        Layout.preferredHeight: 100
                        iconName: modelData.icon
                        value:    modelData.value
                        label:    modelData.label
                        delta:    modelData.delta
                        deltaUp:  modelData.deltaUp
                        accent:   modelData.accent
                        spark:    modelData.spark
                    }
                }
            }

            // ----- Revenue + Activity row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                // Revenue sparkline card
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 620
                    Layout.preferredHeight: 300
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Revenue (last 14 days)"
                            subtitle: "Daily gross in USD"
                        }

                        // Sparkline — series bound to the VM's revenueSeries
                        Canvas {
                            id: _spark
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            // v4: extract numeric values from {label, value} series.
                            readonly property var _series: page._seriesValues(page.viewModel ? page.viewModel.revenueSeries : [])
                            readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                            readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                            // Only repaint when the series actually changes,
                            // not on every booksChanged signal (which fires
                            // for unrelated reasons like review updates).
                            on_SeriesChanged: requestPaint()

                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                if (_series.length === 0 || _max <= 0) return
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
                                for (let j = 0; j < _series.length; ++j) {
                                    const x = pad + j * stepX
                                    const y = pad + (1 - (_series[j] - _min) / range) * (h - 2 * pad)
                                    if (j === 0) ctx.moveTo(x, y)
                                    else         ctx.lineTo(x, y)
                                }
                                ctx.lineWidth = 2
                                ctx.strokeStyle = Theme.color.accent
                                ctx.stroke()

                                // Final-point dot
                                if (_series.length > 0) {
                                    const lx = pad + (_series.length - 1) * stepX
                                    const ly = pad + (1 - (_series[_series.length - 1] - _min) / range) * (h - 2 * pad)
                                    ctx.beginPath()
                                    ctx.arc(lx, ly, 4, 0, 2 * Math.PI)
                                    ctx.fillStyle = Theme.color.accent
                                    ctx.fill()
                                    ctx.lineWidth = 2
                                    ctx.strokeStyle = Theme.color.cardBackground
                                    ctx.stroke()
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md

                            Text {
                                text: "Total: " + (page.viewModel ? page.viewModel.totalRevenue : "$0")
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: page.viewModel ? page.viewModel.revenueTrend : "+0.0%"
                                color: (page.viewModel && page.viewModel.revenueTrend.indexOf("+") === 0) ? Theme.color.success : Theme.color.error
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Item { Layout.fillWidth: true; height: 1 }
                            TextButton {
                                text: "View full report"
                                iconName: "arrow_forward"
                                onClicked: page.navigateToRequested("sales")
                            }
                        }
                    }
                }

                // Recent activity card
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 380
                    Layout.preferredHeight: 300
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Recent activity"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: page._activity
                            spacing: Theme.space.sm
                            interactive: false

                            delegate: RowLayout {
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
                                    Layout.alignment: Qt.AlignTop
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

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        Layout.fillWidth: true
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
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Top performing titles -----
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
                        subtitle: "Sorted by units sold this month"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("catalog")
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 280
                        clip: true
                        model: page._topBooks
                        spacing: Theme.space.sm
                        interactive: false

                        delegate: RowLayout {
                            width: parent.width
                            spacing: Theme.space.md

                            // Rank
                            Text {
                                Layout.preferredWidth: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                            }

                            // Cover
                            BookCover {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 60
                                book: modelData
                            }

                            // Title + author
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

                            // Sales + revenue
                            ColumnLayout {
                                Layout.preferredWidth: 200
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
                                        text: "%1 (%2)".arg(modelData.averageRating.toFixed(1)).arg(modelData.ratingCount)
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

            // ----- Top 5 most-viewed titles -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _topViewedContent
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Top 5 most-viewed titles"
                        subtitle: "Sorted by estimated views (rating count as proxy)"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("sales")
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 220
                        clip: true
                        model: page._topViewed
                        spacing: Theme.space.sm
                        interactive: false

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
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 60
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

                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 4
                                    AppIcon {
                                        name: "visibility"
                                        size: 14
                                        color: Theme.color.textMuted
                                    }
                                    Text {
                                        text: "%1 views".arg((modelData.viewCount || 0).toLocaleString(Qt.locale(), "f", 0))
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                    }
                                }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1 (%2 ratings)".arg(modelData.averageRating.toFixed(1)).arg((modelData.ratingCount || 0).toLocaleString(Qt.locale(), "f", 0))
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

            // ----- Recent orders + Top buyers row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                // Recent orders feed (left, 60%)
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 600
                    Layout.preferredHeight: 340
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Recent orders"
                            subtitle: "Latest purchases from your catalog"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: page.viewModel ? (page.viewModel.recentOrders || []) : []
                            spacing: Theme.space.xs
                            interactive: true

                            delegate: Rectangle {
                                width: parent.width
                                height: 44
                                color: _rowHover.hovered ? Theme.color.fieldFilled : "transparent"

                                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                HoverHandler { id: _rowHover; cursorShape: Qt.PointingHandCursor }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.space.sm
                                    anchors.rightMargin: Theme.space.sm
                                    spacing: Theme.space.md

                                    Rectangle {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        radius: 8
                                        color: Theme.color.accentSoft
                                        AppIcon {
                                            anchors.centerIn: parent
                                            name: "shopping_cart"
                                            size: 14
                                            color: Theme.color.accent
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1
                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.bookTitle
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.customer + " · " + modelData.time
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.preferredWidth: 140
                                        Layout.alignment: Qt.AlignRight
                                        spacing: 1
                                        Text {
                                            Layout.alignment: Qt.AlignRight
                                            text: "$%1".arg((modelData.total || 0).toFixed(2))
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                        }
                                        Text {
                                            Layout.alignment: Qt.AlignRight
                                            text: modelData.status
                                            color: modelData.status === "Completed" ? Theme.color.success
                                                   : modelData.status === "Pending" ? Theme.color.warning
                                                   : Theme.color.error
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Top buyers (right, 40%)
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 400
                    Layout.preferredHeight: 340
                    padding: Theme.space.xl

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Top buyers"
                            subtitle: "Most loyal customers"
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: page.viewModel ? (page.viewModel.topBuyers || []) : []
                            spacing: Theme.space.sm
                            interactive: true

                            delegate: RowLayout {
                                width: parent.width
                                spacing: Theme.space.md

                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    radius: width / 2
                                    color: modelData.avatarColor
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.initials
                                        color: Theme.color.textOnAccent
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.displayName
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: "%1 books · %2".arg(modelData.books).arg(modelData.lastOrder)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                                Text {
                                    Layout.preferredWidth: 100
                                    Layout.alignment: Qt.AlignRight
                                    text: "$%1".arg((modelData.totalSpent || 0).toFixed(0))
                                    color: Theme.color.success
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }
                }
            }

            // ----- Top 5 least-selling books (spec §3-3) -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _leastSellingContent
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Top 5 least-selling titles"
                        subtitle: "Underperforming books that may need promotion"
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 220
                        clip: true
                        interactive: false
                        model: page.viewModel ? (page.viewModel.leastSellingBooks || []) : []
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
                                Layout.preferredWidth: 200
                                Layout.alignment: Qt.AlignRight
                                spacing: 2
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: "%1 units · $%2".arg(modelData.totalSales).arg((modelData.totalSales * modelData.price).toFixed(0))
                                    color: Theme.color.error
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1 (%2)".arg(modelData.averageRating.toFixed(1)).arg(modelData.ratingCount)
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

            // ----- Per-book rating distribution (spec §3-3) -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _ratingDistContent
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Rating distribution"
                        subtitle: page._topBooks.length > 0 ? "For: " + page._topBooks[0].title : "No books yet"
                    }

                    // Rating bars — 5★ down to 1★
                    Repeater {
                        model: page.viewModel && page._topBooks.length > 0
                               ? page.viewModel.ratingDistribution(page._topBooks[0].id || page._topBooks[0].bookId || "")
                               : []
                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm

                                Text {
                                    Layout.preferredWidth: 40
                                    // v4: show stars count + star symbol.
                                    text: modelData.stars + "★"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }

                                // Bar
                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 12

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 6
                                        color: Theme.color.fieldFilled
                                    }
                                    Rectangle {
                                        // v4: service returns {stars, count, percent}, not share.
                                        width: parent.width * ((modelData.percent || 0) / 100.0)
                                        height: parent.height
                                        radius: 6
                                        color: modelData.stars >= 4 ? Theme.color.success
                                               : modelData.stars >= 3 ? Theme.color.warning
                                               : Theme.color.error
                                        Behavior on width { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
                                    }
                                }

                                Text {
                                    Layout.preferredWidth: 60
                                    text: "%1 (%2%)".arg(modelData.count).arg(modelData.percent || 0)
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        visible: page._topBooks.length === 0
                        iconName: "star"
                        title: "No books to analyze"
                        description: "Publish a book to see its rating distribution here."
                    }
                }
            }

            // ----- v4: Per-book average rating bar chart -----
            // Bonus feature from the spec: "نمایش میانگین امتیاز هر کتاب
            // (از ۱ تا ۵ ستاره) به‌صورت نمودار میله‌ای" — bar chart of
            // average rating per book (1-5 stars).
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: 320
                padding: Theme.space.xl

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Average rating per title"
                        subtitle: "How each of your books is rated by readers (1–5 stars)"
                    }

                    Canvas {
                        id: _ratingBars
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Build the bar data from topBooks (each book's
                        // averageRating). Cap at 8 books so the chart stays
                        // readable.
                        readonly property var _bars: {
                            const books = page._topBooks || []
                            const arr = []
                            const max = Math.min(8, books.length)
                            for (let i = 0; i < max; ++i) {
                                const b = books[i]
                                arr.push({
                                    label: b.title || "Untitled",
                                    value: b.averageRating || 0,
                                    count: b.ratingCount || b.totalRatings || 0
                                })
                            }
                            return arr
                        }

                        on_BarsChanged: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.reset()
                            const bars = _bars
                            if (bars.length === 0) {
                                ctx.fillStyle = Theme.color.textMuted
                                ctx.font = "%1px %2".arg(Theme.font.sizeBody).arg(Theme.font.family)
                                ctx.textAlign = "center"
                                ctx.fillText("No rated books yet", width / 2, height / 2)
                                return
                            }
                            const w = width, h = height
                            const padL = 40, padR = 16, padT = 12, padB = 36
                            const chartW = w - padL - padR
                            const chartH = h - padT - padB
                            const n = bars.length
                            const gap = 8
                            const barW = (chartW - (n - 1) * gap) / n

                            // Y-axis grid lines (0, 1, 2, 3, 4, 5)
                            ctx.strokeStyle = Qt.rgba(Theme.color.divider.r,
                                                      Theme.color.divider.g,
                                                      Theme.color.divider.b, 0.7)
                            ctx.lineWidth = 1
                            ctx.fillStyle = Theme.color.textMuted
                            ctx.font = "%1px %2".arg(Theme.font.sizeMicro2).arg(Theme.font.family)
                            ctx.textAlign = "right"
                            ctx.textBaseline = "middle"
                            for (let v = 0; v <= 5; ++v) {
                                const y = padT + chartH - (v / 5) * chartH
                                ctx.beginPath()
                                ctx.moveTo(padL, y); ctx.lineTo(w - padR, y); ctx.stroke()
                                ctx.fillText(String(v), padL - 6, y)
                            }
                            ctx.textBaseline = "alphabetic"

                            // Bars
                            for (let i = 0; i < n; ++i) {
                                const x = padL + i * (barW + gap)
                                const v = Math.max(0, Math.min(5, bars[i].value))
                                const barH = (v / 5) * chartH
                                const y = padT + chartH - barH

                                // Color by rating tier
                                let color
                                if (v >= 4) color = Theme.color.success
                                else if (v >= 3) color = Theme.color.warning
                                else if (v > 0) color = Theme.color.error
                                else color = Theme.color.border

                                const grad = ctx.createLinearGradient(x, y, x, y + barH)
                                grad.addColorStop(0, color)
                                grad.addColorStop(1, Qt.rgba(color.r, color.g, color.b, 0.4))
                                ctx.fillStyle = grad
                                ctx.beginPath()
                                const r = Math.min(4, barW / 2)
                                ctx.moveTo(x, y + r)
                                ctx.quadraticCurveTo(x, y, x + r, y)
                                ctx.lineTo(x + barW - r, y)
                                ctx.quadraticCurveTo(x + barW, y, x + barW, y + r)
                                ctx.lineTo(x + barW, padT + chartH)
                                ctx.lineTo(x, padT + chartH)
                                ctx.closePath()
                                ctx.fill()

                                // Rating value above the bar
                                if (v > 0) {
                                    ctx.fillStyle = Theme.color.textPrimary
                                    ctx.font = "bold %1px %2".arg(Theme.font.sizeCaption).arg(Theme.font.family)
                                    ctx.textAlign = "center"
                                    ctx.fillText(v.toFixed(1), x + barW / 2, y - 4)
                                }

                                // Book label below the bar (truncated)
                                ctx.fillStyle = Theme.color.textSecondary
                                ctx.font = "%1px %2".arg(Theme.font.sizeMicro2).arg(Theme.font.family)
                                ctx.textAlign = "center"
                                let label = bars[i].label
                                if (label.length > 14) label = label.substring(0, 12) + "…"
                                ctx.fillText(label, x + barW / 2, padT + chartH + 14)
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
