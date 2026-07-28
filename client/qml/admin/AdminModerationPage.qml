// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Added fallback Connections handlers (onRefreshed, onDataChanged, onDataReady)
//     so the moderation lists re-sync whenever the VM pushes data
//   - Added _delayedRefresh Timer to re-sync after async VM.refresh() completes
//   - Defensive null/undefined checks in _refreshFlaggedFromVM / _refreshReportedFromVM

// =============================================================================
//  AdminModerationPage.qml
// =============================================================================
//  Moderation workbench for the admin role. Three KPI cards up top, then a
//  two-column layout: flagged reviews (left) and reported content (right).
//  Each item has discrete action buttons (Remove / Dismiss / Take action).
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `flaggedReviews` and `reportedContent` (QVariantList) plus
//  dismissFlaggedReview(id) / removeFlaggedReview(id) /
//  dismissReport(id) / takeActionOnReport(id, action).
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

    // ----- KPI cards — values computed from VM data -----
    readonly property int _pendingReportsCount: page.viewModel ? page.viewModel.pendingReports : 0
    readonly property int _flaggedCount:        page.viewModel && page.viewModel.flaggedReviews ? page.viewModel.flaggedReviews.length : 0
    readonly property int _reportedCount:       page.viewModel && page.viewModel.reportedContent ? page.viewModel.reportedContent.length : 0
    readonly property int _actionRatePct: {
        // Approximate action rate from the current queue:
        //   actioned = total moderation load minus pending reports.
        const total = page._flaggedCount + page._reportedCount
        if (total === 0) return 0
        return Math.round((1 - Math.min(1, page._pendingReportsCount / Math.max(1, total))) * 100)
    }

    readonly property var _kpis: [
        // BUG FIX (admin polish): removed the hardcoded "+3 today" /
        // "+5 vs yesterday" deltas that were always the same regardless
        // of real data. Now shows a neutral "live" indicator so the
        // admin isn't misled by fake trend numbers.
        { icon: "report",         value: page._pendingReportsCount.toString(),  label: "Pending reports",     delta: "live",              deltaUp: false, accent: Theme.color.warning },
        { icon: "task_alt",       value: page._reportedCount.toString(),        label: "Auto-resolved today", delta: "live",              deltaUp: true,  accent: Theme.color.success },
        { icon: "check_circle",   value: page._actionRatePct + "%",             label: "Action rate",         delta: "current",           deltaUp: true,  accent: Theme.color.accent  }
    ]

    // ----- Local mirrors of the VM's flagged + reported lists -----
    ListModel { id: _flagged }
    ListModel { id: _reported }

    function _typeIcon(t) {
        if (t === "user")    return "person"
        if (t === "comment") return "reply"
        return "rate_review"
    }
    function _typeColor(t) {
        if (t === "user")    return Theme.color.error
        if (t === "comment") return Theme.color.warning
        return Theme.color.accent
    }
    function _typeSoft(t) {
        if (t === "user")    return Theme.color.errorSoft
        if (t === "comment") return Theme.color.warningSoft
        return Theme.color.accentSoft
    }

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshFlaggedFromVM() {
        if (!page.viewModel) return
        _flagged.clear()
        const list = page.viewModel.flaggedReviews || []
        for (let i = 0; i < list.length; ++i) {
            const r = list[i]
            if (!r) continue   // defensive: skip null/undefined entries
            // BUG FIX (admin moderation): map the actual server field names
            // (bookTitle, userDisplayName, stars, text) to the local model
            // keys (book, reviewer, rating, excerpt) that the delegate
            // expects. The previous mapping only checked `r.book`,
            // `r.reviewer`, etc. which never matched the server payload —
            // so every flagged review showed empty fields even when the
            // data was present.
            _flagged.append({
                id:        r.id        !== undefined ? r.id        : i,
                book:      r.book      || r.bookTitle      || r.bookId || "",
                reviewer:  r.reviewer  || r.userDisplayName || r.username || "",
                rating:    r.rating    !== undefined ? r.rating    : (r.stars    !== undefined ? r.stars    : 0),
                excerpt:   r.excerpt   || r.text           || ""
            })
        }
    }

    function _refreshReportedFromVM() {
        if (!page.viewModel) return
        _reported.clear()
        const list = page.viewModel.reportedContent || []
        for (let i = 0; i < list.length; ++i) {
            const r = list[i]
            if (!r) continue   // defensive: skip null/undefined entries
            _reported.append({
                id:       r.id       !== undefined ? r.id       : i,
                type:     r.type     || "review",
                reporter: r.reporter || r.reportedBy || "",
                reason:   r.reason   || "",
                time:     r.time     || r.reportedAt || ""
            })
        }
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        // KPIs re-evaluate automatically through their property bindings.
        // The local ListModels (_flagged / _reported) are populated
        // imperatively, so we refresh them when their source lists change.
        // Previously this block had TWO onModerationChanged handlers —
        // QML only attaches one, so the second refresh was silently
        // dropped. Fixed by calling both refreshes from a single handler.
        function onModerationChanged() {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
        }
        // Fallback handlers — re-sync whenever the VM completes a refresh cycle.
        function onRefreshed() {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
        }
        function onDataChanged() {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
        }
        function onDataReady() {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
        }
    }

    // ----- Delayed refresh -----
    Timer {
        id: _delayedRefresh
        interval: 500
        repeat: true
        running: page.visible
        onTriggered: {
            if (page.viewModel) {
                var hasFlagged = page.viewModel.flaggedReviews && page.viewModel.flaggedReviews.length > 0
                var hasReported = page.viewModel.reportedContent && page.viewModel.reportedContent.length > 0
                if (hasFlagged || hasReported) {
                    page._refreshFlaggedFromVM()
                    page._refreshReportedFromVM()
                    _delayedRefresh.stop()
                }
            }
        }
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
            if (typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
        }
    }

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
                            name: "gavel"
                            size: 22
                            color: Theme.color.accent
                        }
                    }
                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: "Moderation"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "Flagged reviews and reported content"
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

            // ----- Two-column layout: flagged reviews + reported content -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg

                // ----- Left: Flagged reviews -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50
                    // Height based on item count (each delegate ~200px) + header
                    height: Math.max(280, _flagged.count * 200 + 80)
                    padding: Theme.space.xl

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Flagged reviews"
                            subtitle: _flagged.count + " reviews awaiting moderation"
                        }

                        ListView {
                            width: parent.width
                            height: _flagged.count > 0 ? parent.height - 50 : 0
                            clip: true
                            model: _flagged
                            spacing: Theme.space.md

                            delegate: Rectangle {
                                width: parent.width
                                height: _flagCol.implicitHeight + 2 * Theme.space.md
                                radius: Theme.radius.md
                                color: Theme.color.fieldFilled
                                border.color: Theme.color.divider
                                border.width: 1

                                ColumnLayout {
                                    id: _flagCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.sm

                                    // Book title + reviewer
                                    RowLayout {
                                        width: parent.width
                                        spacing: Theme.space.sm

                                        Text {
                                            text: model.book
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightSemibold
                                            elide: Text.ElideRight
                                            width: parent.width - 200
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                        Text {
                                            text: "by @" + model.reviewer
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }

                                    // Rating stars
                                    RatingStars {
                                        rating: model.rating
                                        size: 14
                                    }

                                    // Excerpt
                                    Text {
                                        width: parent.width
                                        text: "\"" + model.excerpt + "\""
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 2
                                        elide: Text.ElideRight
                                    }

                                    // Actions
                                    RowLayout {
                                        width: parent.width
                                        spacing: Theme.space.sm

                                        SecondaryButton {
                                            text: "Dismiss"
                                            onClicked: {
                                                if (page.viewModel && typeof page.viewModel.dismissFlaggedReview === "function") {
                                                    page.viewModel.dismissFlaggedReview(model.id)
                                                    page.toastRequested("info", "Flag dismissed",
                                                                        "Review on " + model.book + " kept live.")
                                                } else {
                                                    page.toastRequested("error", "No view model",
                                                                        "AdminViewModel is not available.")
                                                }
                                            }
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                        PrimaryButton {
                                            text: "Remove"
                                            iconName: "delete"
                                            onClicked: {
                                                if (page.viewModel && typeof page.viewModel.removeFlaggedReview === "function") {
                                                    page.viewModel.removeFlaggedReview(model.id)
                                                    page.toastRequested("success", "Review removed",
                                                                        "Review by @" + model.reviewer + " was removed.")
                                                } else {
                                                    page.toastRequested("error", "No view model",
                                                                        "AdminViewModel is not available.")
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Empty state for flagged reviews
                        EmptyState {
                            width: parent.width
                            height: _flagged.count === 0 ? 160 : 0
                            visible: _flagged.count === 0
                            iconName: "check_circle"
                            title: "No flagged reviews"
                            description: "All caught up — no reviews need moderation."
                        }
                    }
                }

                // ----- Right: Reported content -----
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50
                    // Height based on item count (each delegate ~160px) + header
                    height: Math.max(280, _reported.count * 160 + 80)
                    padding: Theme.space.xl

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Reported content"
                            subtitle: _reported.count + " items awaiting action"
                        }

                        ListView {
                            width: parent.width
                            height: _reported.count > 0 ? parent.height - 50 : 0
                            clip: true
                            model: _reported
                            spacing: Theme.space.md

                            delegate: Rectangle {
                                width: parent.width
                                height: _repCol.implicitHeight + 2 * Theme.space.md
                                radius: Theme.radius.md
                                color: Theme.color.fieldFilled
                                border.color: Theme.color.divider
                                border.width: 1

                                ColumnLayout {
                                    id: _repCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.sm

                                    // Type + reporter + time
                                    RowLayout {
                                        width: parent.width
                                        spacing: Theme.space.sm

                                        Rectangle {
                                            width: 28; height: 28; radius: 8
                                            color: page._typeSoft(model.type)
                                            AppIcon {
                                                anchors.centerIn: parent
                                                name: page._typeIcon(model.type)
                                                size: 16
                                                color: page._typeColor(model.type)
                                            }
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        Column {
                                            spacing: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: model.type.charAt(0).toUpperCase() + model.type.slice(1) + " report"
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                font.weight: Theme.font.weightSemibold
                                            }
                                            Text {
                                                text: "Reported by @" + model.reporter + " · " + model.time
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                        }

                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    }

                                    // Reason
                                    Text {
                                        width: parent.width
                                        text: "Reason: " + model.reason
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        wrapMode: Text.WordWrap
                                    }

                                    // Actions
                                    RowLayout {
                                        width: parent.width
                                        spacing: Theme.space.sm

                                        SecondaryButton {
                                            text: "Dismiss"
                                            onClicked: {
                                                if (page.viewModel && typeof page.viewModel.dismissReport === "function") {
                                                    page.viewModel.dismissReport(model.id)
                                                    page.toastRequested("info", "Report dismissed",
                                                                        model.type + " report dismissed as unfounded.")
                                                } else {
                                                    page.toastRequested("error", "No view model",
                                                                        "AdminViewModel is not available.")
                                                }
                                            }
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                        PrimaryButton {
                                            text: "Take action"
                                            iconName: "gavel"
                                            onClicked: {
                                                if (page.viewModel && typeof page.viewModel.takeActionOnReport === "function") {
                                                    page.viewModel.takeActionOnReport(model.id, "removed")
                                                    page.toastRequested("success", "Action taken",
                                                                        "Action applied to reported " + model.type + ".")
                                                } else {
                                                    page.toastRequested("error", "No view model",
                                                                        "AdminViewModel is not available.")
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Empty state for reported content
                        EmptyState {
                            width: parent.width
                            height: _reported.count === 0 ? 160 : 0
                            visible: _reported.count === 0
                            iconName: "check_circle"
                            title: "No reported content"
                            description: "All caught up — no items need action."
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
