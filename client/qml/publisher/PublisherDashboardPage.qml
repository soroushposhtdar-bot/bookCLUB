// =============================================================================
//  PublisherDashboardPage.qml
// =============================================================================
//  Overview screen for the publisher role. Shows KPI stat cards, a revenue
//  sparkline, recent activity, and top-performing titles. All data is
//  synthesized from the BookService catalog so the page is never empty.
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

Item {
    id: page

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)
    signal navigateToRequested(string route)   // emitted by "View report" / "See all" buttons

    // ----- KPI cards — values bound to the PublisherViewModel -----
    //   All four KPIs now read live from the VM. The "delta" strings on the
    //   first three are derived from VM data too (revenueTrend, units delta,
    //   active titles). Avg. rating keeps its synthesized delta for now.
    readonly property var _kpis: [
        { icon: "attach_money",  value: page.viewModel ? page.viewModel.totalRevenue : "$0", label: "Revenue (30 days)", delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%", deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0, accent: Theme.color.success },
        { icon: "shopping_cart", value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0), label: "Units sold",        delta: page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%",  deltaUp: (page.viewModel ? page.viewModel.unitsSoldTrend : "+0.0%").indexOf("+") === 0,  accent: Theme.color.accent  },
        { icon: "library_books", value: (page.viewModel ? page.viewModel.activeTitles : 0).toString(),      label: "Active titles",     delta: "%1 of %2 total".arg(page.viewModel ? page.viewModel.activeTitles : 0).arg(page.viewModel ? page.viewModel.totalBooks : 0), deltaUp: true,  accent: Theme.color.info    },
        { icon: "star",          value: page.viewModel ? page.viewModel.averageRating : "0.00",    label: "Avg. rating",       delta: "Across all rated titles", deltaUp: true, accent: Theme.color.warning }
    ]

    // ----- Top performing titles (QVariantList from the VM) -----
    readonly property var _topBooks: page.viewModel ? page.viewModel.topBooks : []

    // ----- Top 5 most-viewed titles (QVariantList from the VM) -----
    // Bound to PublisherViewModel.topViewedBooks which delegates to
    // PublisherService::topViewedBooksVariant(5). The service synthesizes a
    // deterministic `viewCount` per book (ratingCount * 7 + totalSales) since
    // the mock doesn't track per-book view events.
    readonly property var _topViewed: page.viewModel ? page.viewModel.topViewedBooks : []

    // ----- Recent activity feed — bound to viewModel.activityFeed -----
    readonly property var _activity: page.viewModel ? (page.viewModel.activityFeed || []) : []

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

            // ----- Revenue + Activity row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // Revenue sparkline card
                Card {
                    width: parent.width * 0.62 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Revenue (last 14 days)"
                            subtitle: "Daily gross in USD"
                        }

                        // Sparkline — series bound to the VM's revenueSeries
                        // (QVariantList of daily revenue values). Falls back to
                        // an empty array until the VM loads.
                        Canvas {
                            id: _spark
                            width: parent.width
                            height: 180
                            readonly property var _series: page.viewModel ? (page.viewModel.revenueSeries || []) : []
                            readonly property real _max: _series.length > 0 ? Math.max.apply(null, _series) : 0
                            readonly property real _min: _series.length > 0 ? Math.min.apply(null, _series) : 0

                            Connections {
                                target: page.viewModel
                                ignoreUnknownSignals: true
                                onBooksChanged: _spark.requestPaint()
                            }

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
                                    if (i === 0) ctx.lineTo(x, y)
                                    else         ctx.lineTo(x, y)
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
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
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
                    width: parent.width * 0.38 - Theme.space.lg / 2
                    height: 280
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader { width: parent.width; title: "Recent activity" }

                        ListView {
                            width: parent.width
                            height: parent.height - 40
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
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Top performing titles"
                        subtitle: "Sorted by units sold this month"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("catalog")
                    }

                    ListView {
                        width: parent.width
                        height: 280
                        clip: true
                        model: page._topBooks
                        spacing: Theme.space.sm
                        interactive: false

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            // Rank
                            Text {
                                width: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Cover
                            BookCover {
                                width: 44
                                height: 60
                                book: modelData
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Title + author
                            Column {
                                width: parent.width - 28 - 44 - Theme.space.md * 3 - 200
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    width: parent.width
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
                            Column {
                                width: 200
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: "%1 units · $%2".arg(modelData.totalSales).arg((modelData.totalSales * modelData.price).toFixed(0))
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    horizontalAlignment: Text.AlignRight
                                    anchors.right: parent.right
                                }
                                Row {
                                    anchors.right: parent.right
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1 (%2)".arg(modelData.averageRating.toFixed(1)).arg(modelData.ratingCount)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Top 5 most-viewed titles -----
            // Second ranking card. The "Top performing titles" card above is
            // sorted by units sold; this one is sorted by view count (proxied
            // via ratingCount in the mock — see PublisherService.cpp). The two
            // lists often overlap, but the order is different and highlights
            // books that get attention but don't always convert to sales.
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Top 5 most-viewed titles"
                        subtitle: "Sorted by estimated views (rating count as proxy)"
                        showSeeAll: true
                        onSeeAllClicked: page.navigateToRequested("sales")
                    }

                    ListView {
                        width: parent.width
                        height: 220
                        clip: true
                        model: page._topViewed
                        spacing: Theme.space.sm
                        interactive: false

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            // Rank
                            Text {
                                width: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Cover
                            BookCover {
                                width: 44
                                height: 60
                                book: modelData
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Title + author
                            Column {
                                width: parent.width - 28 - 44 - Theme.space.md * 3 - 220
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    width: parent.width
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

                            // Views + rating
                            Column {
                                width: 220
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Row {
                                    anchors.right: parent.right
                                    spacing: 4

                                    AppIcon {
                                        name: "visibility"
                                        size: 14
                                        color: Theme.color.textMuted
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "%1 views".arg((modelData.viewCount || 0).toLocaleString(Qt.locale(), "f", 0))
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                Row {
                                    anchors.right: parent.right
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1 (%2 ratings)".arg(modelData.averageRating.toFixed(1)).arg((modelData.ratingCount || 0).toLocaleString(Qt.locale(), "f", 0))
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Recent orders + Top buyers row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // Recent orders feed (left, 60%)
                Card {
                    width: parent.width * 0.60 - Theme.space.lg / 2
                    height: 320
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Recent orders"
                            subtitle: "Latest purchases from your catalog"
                        }

                        ListView {
                            width: parent.width
                            height: parent.height - 50
                            clip: true
                            model: page.viewModel ? (page.viewModel.recentOrders || []) : []
                            spacing: Theme.space.xs
                            interactive: true

                            delegate: Rectangle {
                                width: parent.width
                                height: 44
                                color: _rowHover.hovered ? Theme.color.fieldFilled : "transparent"

                                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                                HoverHandler { id: _rowHover; cursorShape: Qt.PointingHandCursor }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.space.sm
                                    anchors.rightMargin: Theme.space.sm
                                    spacing: Theme.space.md

                                    Rectangle {
                                        width: 28; height: 28; radius: 8
                                        color: Theme.color.accentSoft
                                        anchors.verticalCenter: parent.verticalCenter
                                        AppIcon {
                                            anchors.centerIn: parent
                                            name: "shopping_cart"
                                            size: 14
                                            color: Theme.color.accent
                                        }
                                    }
                                    Column {
                                        width: parent.width - 28 - Theme.space.md - 140 - Theme.space.md
                                        spacing: 1
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            width: parent.width
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
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Column {
                                        width: 140
                                        spacing: 1
                                        anchors.verticalCenter: parent.verticalCenter

                                        Text {
                                            text: "$%1".arg((modelData.total || 0).toFixed(2))
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                            horizontalAlignment: Text.AlignRight
                                            anchors.right: parent.right
                                        }
                                        Text {
                                            text: modelData.status
                                            color: modelData.status === "Completed" ? Theme.color.success
                                                   : modelData.status === "Pending" ? Theme.color.warning
                                                   : Theme.color.error
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            horizontalAlignment: Text.AlignRight
                                            anchors.right: parent.right
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Top buyers (right, 40%)
                Card {
                    width: parent.width * 0.40 - Theme.space.lg / 2
                    height: 320
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Top buyers"
                            subtitle: "Most loyal customers"
                        }

                        ListView {
                            width: parent.width
                            height: parent.height - 50
                            clip: true
                            model: page.viewModel ? (page.viewModel.topBuyers || []) : []
                            spacing: Theme.space.sm
                            interactive: true

                            delegate: Row {
                                width: parent.width
                                spacing: Theme.space.md

                                Rectangle {
                                    width: 36; height: 36; radius: 18
                                    color: modelData.avatarColor
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.initials
                                        color: Theme.color.textOnAccent
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                }
                                Column {
                                    width: parent.width - 36 - Theme.space.md - 100 - Theme.space.md
                                    spacing: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        width: parent.width
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
                                Item { width: 1; Layout.fillWidth: true; height: 1 }
                                Text {
                                    width: 100
                                    text: "$%1".arg((modelData.totalSpent || 0).toFixed(0))
                                    color: Theme.color.success
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    horizontalAlignment: Text.AlignRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }

            // ----- Top 5 least-selling books (spec §3-3) -----
            //   Bound to viewModel.leastSellingBooks (sorted ascending by totalSales).
            //   Helps the publisher identify underperforming titles.
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Top 5 least-selling titles"
                        subtitle: "Underperforming books that may need promotion"
                    }

                    ListView {
                        width: parent.width
                        height: 220
                        clip: true
                        interactive: false
                        model: page.viewModel ? (page.viewModel.leastSellingBooks || []) : []
                        spacing: Theme.space.sm

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Text {
                                width: 28
                                text: (index + 1).toString()
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            BookCover {
                                width: 40; height: 56
                                book: modelData
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Column {
                                width: parent.width - 28 - 40 - Theme.space.md * 2 - 200
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    width: parent.width
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

                            Column {
                                width: 200
                                spacing: 2
                                anchors.verticalCenter: parent.verticalCenter

                                Text {
                                    text: "%1 units · $%2".arg(modelData.totalSales).arg((modelData.totalSales * modelData.price).toFixed(0))
                                    color: Theme.color.error
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                    horizontalAlignment: Text.AlignRight
                                    anchors.right: parent.right
                                }
                                Row {
                                    anchors.right: parent.right
                                    spacing: 4
                                    RatingStars { size: 12; rating: modelData.averageRating }
                                    Text {
                                        text: "%1 (%2)".arg(modelData.averageRating.toFixed(1)).arg(modelData.ratingCount)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Per-book rating distribution (spec §3-3) -----
            //   Shows a 1-5 star histogram for the top-selling book (or the
            //   first book in the catalog). The publisher can use this to see
            //   how ratings are distributed for their best-known title.
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Rating distribution"
                        subtitle: page._topBooks.length > 0 ? "For: " + page._topBooks[0].title : "No books yet"
                    }

                    // Rating bars — 5★ down to 1★
                    Repeater {
                        model: page.viewModel && page._topBooks.length > 0
                               ? page.viewModel.ratingDistribution(page._topBooks[0].id || page._topBooks[0].bookId || "")
                               : []
                        delegate: Column {
                            width: parent.width
                            spacing: Theme.space.xs

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: modelData.label
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                    width: 40
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                // Bar
                                Item {
                                    width: parent.width - 40 - 60 - Theme.space.sm * 2
                                    height: 12
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 6
                                        color: Theme.color.fieldFilled
                                    }
                                    Rectangle {
                                        width: parent.width * (modelData.share || 0)
                                        height: parent.height
                                        radius: 6
                                        color: modelData.stars >= 4 ? Theme.color.success
                                               : modelData.stars >= 3 ? Theme.color.warning
                                               : Theme.color.error
                                        Behavior on width { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
                                    }
                                }

                                Text {
                                    text: "%1 (%2%)".arg(modelData.count).arg(Math.round((modelData.share || 0) * 100))
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    width: 60
                                    horizontalAlignment: Text.AlignRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    EmptyState {
                        width: parent.width
                        height: 120
                        visible: page._topBooks.length === 0
                        iconName: "star"
                        title: "No books to analyze"
                        description: "Publish a book to see its rating distribution here."
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
