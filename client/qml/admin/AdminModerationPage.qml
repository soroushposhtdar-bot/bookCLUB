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
        { icon: "report",         value: page._pendingReportsCount.toString(),  label: "Pending reports",     delta: "+3 today",          deltaUp: false, accent: Theme.color.warning },
        { icon: "task_alt",       value: page._reportedCount.toString(),        label: "Auto-resolved today", delta: "+5 vs yesterday",   deltaUp: true,  accent: Theme.color.success },
        { icon: "check_circle",   value: page._actionRatePct + "%",             label: "Action rate",         delta: "+1.2% this week",   deltaUp: true,  accent: Theme.color.accent  }
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
            _flagged.append({
                id:        r.id        !== undefined ? r.id        : i,
                book:      r.book      || r.bookTitle    || "",
                reviewer:  r.reviewer  || r.username     || "",
                rating:    r.rating    !== undefined ? r.rating : 0,
                excerpt:   r.excerpt   || r.text         || ""
            })
        }
    }

    function _refreshReportedFromVM() {
        if (!page.viewModel) return
        _reported.clear()
        const list = page.viewModel.reportedContent || []
        for (let i = 0; i < list.length; ++i) {
            const r = list[i]
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
        onModerationChanged: {
            page._refreshFlaggedFromVM()
            page._refreshReportedFromVM()
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
                        width: (parent.width - 2 * Theme.space.lg) / 3
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
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // ----- Left: Flagged reviews -----
                Card {
                    width: parent.width * 0.50 - Theme.space.lg / 2
                    height: Math.max(280, _flagged.count > 0 ? _flagCol.implicitHeight + 80 : 280)
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
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

                                Column {
                                    id: _flagCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.sm

                                    // Book title + reviewer
                                    Row {
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
                                    Row {
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
                            height: 160
                            visible: _flagged.count === 0
                            iconName: "check_circle"
                            title: "No flagged reviews"
                            description: "All caught up — no reviews need moderation."
                        }
                    }
                }

                // ----- Right: Reported content -----
                Card {
                    width: parent.width * 0.50 - Theme.space.lg / 2
                    // Previously this height binding referenced _flagged.count
                    // and _flagCol.implicitHeight (both from the LEFT card)
                    // due to a copy-paste error. Fixed to use _reported.count
                    // and _repCol.implicitHeight so the right card's height
                    // reflects its own content.
                    height: Math.max(280, _reported.count > 0 ? _repCol.implicitHeight + 80 : 280)
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
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

                                Column {
                                    id: _repCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.sm

                                    // Type + reporter + time
                                    Row {
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
                                    Row {
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
                            height: 160
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
