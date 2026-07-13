// =============================================================================
//  ServerSessionsPage.qml
// =============================================================================
//  Active user sessions table (login-bounded socket conversations), plus a
//  "Group reading rooms" panel for collaborative sessions. Includes KPI
//  cards, per-row actions (view + terminate), and pagination. Sourced from
//  ServerViewModel.sessions and ServerViewModel.rooms.
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

    // ----- Pagination state (sessions only) -----
    property int _pageSize: 8
    property int _currentPage: 1
    property var _pagedSessions: []

    function _totalPages() {
        var n = page.viewModel ? page.viewModel.sessions.length : 0
        return Math.max(1, Math.ceil(n / page._pageSize))
    }

    function _refreshPaged() {
        var all = page.viewModel ? page.viewModel.sessions : []
        var start = (page._currentPage - 1) * page._pageSize
        if (start >= all.length) {
            page._currentPage = 1
            start = 0
        }
        var end = Math.min(start + page._pageSize, all.length)
        var slice = []
        for (var i = start; i < end; ++i) slice.push(all[i])
        page._pagedSessions = slice
    }

    function _statusColor(s) {
        if (s === "Active")   return Theme.color.success
        if (s === "Idle")     return Theme.color.warning
        return Theme.color.error
    }

    function _typeColor(t) {
        if (t === "Reading")   return Theme.color.accent
        if (t === "Cart")      return Theme.color.warning
        if (t === "Checkout")  return Theme.color.success
        if (t === "Auth")      return Theme.color.info
        return Theme.color.textSecondary
    }

    function _readingCount() {
        var sessions = page.viewModel ? page.viewModel.sessions : []
        var n = 0
        for (var i = 0; i < sessions.length; ++i) {
            if (sessions[i].type === "Reading") n++
        }
        return n
    }

    Component.onCompleted: page._refreshPaged()

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onSessionsChanged: page._refreshPaged()
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
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "verified"
                    value:    page.viewModel ? page.viewModel.activeSessionCount : 0
                    label:    "Active sessions"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "schedule"
                    value:    "—"
                    label:    "Avg duration"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "auto_stories"
                    value:    page._readingCount()
                    label:    "Concurrent reads"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
            }

            // ----- Active sessions table -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Active sessions"
                        subtitle: "%1 live sessions".arg(page.viewModel ? page.viewModel.sessions.length : 0)
                    }

                    Row {
                        width: parent.width
                        Repeater {
                            model: [
                                { w: 0.18, label: "Session ID" },
                                { w: 0.13, label: "User" },
                                { w: 0.10, label: "Started" },
                                { w: 0.13, label: "Last activity" },
                                { w: 0.12, label: "Type" },
                                { w: 0.14, label: "Status" },
                                { w: 0.20, label: "Actions" }
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
                        height: page._pagedSessions.length * 52
                        clip: true
                        interactive: false
                        spacing: 0
                        model: page._pagedSessions

                        delegate: Rectangle {
                            width: parent.width
                            height: 52
                            color: _rowHover1.hovered ? Theme.color.fieldFilled : "transparent"

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover1
                                cursorShape: Qt.PointingHandCursor
                            }

                            Row {
                                width: parent.width
                                height: 52

                                Text {
                                    width: parent.width * 0.18
                                    text: modelData.sessionId
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.13
                                    text: modelData.user
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.10
                                    text: modelData.started
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.13
                                    text: modelData.lastActivity
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.12
                                    text: modelData.type
                                    color: page._typeColor(modelData.type)
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.14
                                    text: modelData.status
                                    color: page._statusColor(modelData.status)
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightSemibold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Row {
                                    width: parent.width * 0.20
                                    spacing: Theme.space.xs
                                    anchors.verticalCenter: parent.verticalCenter
                                    IconButton {
                                        iconName: "visibility"
                                        onClicked: page.toastRequested("info", "Session details",
                                                                        "Opening session %1.".arg(modelData.sessionId))
                                    }
                                    IconButton {
                                        iconName: "close"
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.terminateSession === "function") {
                                                page.viewModel.terminateSession(modelData.sessionId)
                                                page.toastRequested("warning", "Session terminated",
                                                                    "Session %1 (%2) has been terminated.".arg(modelData.sessionId).arg(modelData.user))
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }
                        }
                    }

                    Row {
                        width: parent.width
                        layoutDirection: Qt.RightToLeft
                        Pagination {
                            currentPage: page._currentPage
                            totalPages: page._totalPages()
                            onPageRequested: function(pageNum) {
                                page._currentPage = pageNum
                                page._refreshPaged()
                                page.toastRequested("info", "Page %1".arg(pageNum),
                                                     "Switching to page %1 of sessions.".arg(pageNum))
                            }
                        }
                    }
                }
            }

            // ----- Group reading rooms -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Group reading rooms"
                        subtitle: "Collaborative reading sessions"
                    }

                    Row {
                        width: parent.width
                        Repeater {
                            model: [
                                { w: 0.24, label: "Room name" },
                                { w: 0.24, label: "Book" },
                                { w: 0.12, label: "Participants" },
                                { w: 0.14, label: "Owner" },
                                { w: 0.10, label: "Started" },
                                { w: 0.16, label: "Status" }
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
                        height: (page.viewModel ? page.viewModel.rooms.length : 0) * 48
                        clip: true
                        interactive: false
                        spacing: 0
                        model: page.viewModel ? page.viewModel.rooms : []

                        delegate: Rectangle {
                            width: parent.width
                            height: 48
                            color: _rowHover2.hovered ? Theme.color.fieldFilled : "transparent"

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover2
                                cursorShape: Qt.PointingHandCursor
                            }

                            Row {
                                width: parent.width
                                height: 48
                                Text {
                                    width: parent.width * 0.24
                                    text: modelData.room
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.24
                                    text: modelData.book
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.12
                                    text: modelData.participants
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.14
                                    text: modelData.owner
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.10
                                    text: modelData.started
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    width: parent.width * 0.16
                                    text: modelData.status
                                    color: page._statusColor(modelData.status)
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightSemibold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
