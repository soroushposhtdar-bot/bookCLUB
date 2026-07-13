// =============================================================================
//  ServerClientsPage.qml
// =============================================================================
//  Connected clients table. Each row represents a live socket connection:
//  client id, user, role badge, IP, connection duration, latency, and quick
//  actions (view / disconnect). Includes an empty-state fallback. Sourced from
//  ServerViewModel.clients with client-side pagination.
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

    // ----- Pagination state -----
    property int _pageSize: 8
    property int _currentPage: 1
    property var _pagedClients: []

    function _totalPages() {
        var n = page.viewModel ? page.viewModel.clients.length : 0
        return Math.max(1, Math.ceil(n / page._pageSize))
    }

    function _refreshPaged() {
        var all = page.viewModel ? page.viewModel.clients : []
        var start = (page._currentPage - 1) * page._pageSize
        if (start >= all.length) {
            // Clamp the current page if the data shrank.
            page._currentPage = 1
            start = 0
        }
        var end = Math.min(start + page._pageSize, all.length)
        var slice = []
        for (var i = start; i < end; ++i) slice.push(all[i])
        page._pagedClients = slice
    }

    function _roleColor(role) {
        if (role === "admin")     return Theme.color.error
        if (role === "publisher") return Theme.color.warning
        if (role === "server")    return Theme.color.success
        return Theme.color.accent
    }

    function _roleLabel(role) {
        if (role === "admin")     return "Admin"
        if (role === "publisher") return "Publisher"
        if (role === "server")    return "Service"
        return "User"
    }

    function _roleBreakdown() {
        var clients = page.viewModel ? page.viewModel.clients : []
        var users = 0, pubs = 0, ops = 0
        for (var i = 0; i < clients.length; ++i) {
            var r = clients[i].role
            if (r === "admin") ops++
            else if (r === "publisher") pubs++
            else users++
        }
        return users + " · " + pubs + " · " + ops
    }

    Component.onCompleted: page._refreshPaged()

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onClientsChanged: page._refreshPaged()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "group"
                    value:    page.viewModel ? page.viewModel.connectedClientCount : 0
                    label:    "Total connected"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "badge"
                    value:    page._roleBreakdown()
                    label:    "Users / pubs / ops"
                    delta:    "Breakdown by role"
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "trending_up"
                    value:    (page.viewModel ? page.viewModel.connectedClientCount : 0) + 3   // peak = live + offset (mock)
                    label:    "Peak today"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
            }

            // ----- Clients table -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Connected clients"
                        subtitle: "%1 live socket connections".arg(page.viewModel ? page.viewModel.clients.length : 0)
                    }

                    // Header row
                    Row {
                        width: parent.width
                        spacing: 0

                        Repeater {
                            model: [
                                { w: 0.14, label: "Client ID" },
                                { w: 0.16, label: "User" },
                                { w: 0.11, label: "Role" },
                                { w: 0.15, label: "IP address" },
                                { w: 0.13, label: "Connected" },
                                { w: 0.11, label: "Latency" },
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

                    // Empty state OR rows
                    EmptyState {
                        width: parent.width
                        height: page._pagedClients.length === 0 ? 240 : 0
                        visible: page._pagedClients.length === 0
                        iconName: "group"
                        title: "No clients connected"
                        description: "The server is currently idle. New connections will appear here in real time."
                    }

                    ListView {
                        width: parent.width
                        height: page._pagedClients.length * 52
                        clip: true
                        interactive: false
                        spacing: 0
                        model: page._pagedClients
                        visible: page._pagedClients.length > 0

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
                                spacing: 0

                                Text {
                                    width: parent.width * 0.14
                                    text: modelData.clientId
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: parent.width * 0.16
                                    text: modelData.user
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                // Role badge
                                Item {
                                    width: parent.width * 0.11
                                    height: parent.height
                                    Row {
                                        spacing: 4
                                        anchors.verticalCenter: parent.verticalCenter
                                        Rectangle {
                                            width: _roleTxt.implicitWidth + 12; height: 20; radius: 10
                                            color: Qt.rgba(page._roleColor(modelData.role).r,
                                                           page._roleColor(modelData.role).g,
                                                           page._roleColor(modelData.role).b, 0.14)
                                            Text {
                                                id: _roleTxt
                                                anchors.centerIn: parent
                                                text: page._roleLabel(modelData.role)
                                                color: page._roleColor(modelData.role)
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                font.weight: Theme.font.weightBold
                                            }
                                        }
                                    }
                                }

                                Text {
                                    width: parent.width * 0.15
                                    text: modelData.ip
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: parent.width * 0.13
                                    text: modelData.since
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    width: parent.width * 0.11
                                    text: modelData.latency
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Row {
                                    width: parent.width * 0.20
                                    spacing: Theme.space.xs
                                    anchors.verticalCenter: parent.verticalCenter
                                    IconButton {
                                        iconName: "logout"
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.disconnectClient === "function") {
                                                page.viewModel.disconnectClient(modelData.clientId)
                                                page.toastRequested("warning", "Client disconnected",
                                                                    "Client %1 (%2) has been disconnected.".arg(modelData.clientId).arg(modelData.user))
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }
                        }
                    }

                    // Pagination
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
                                                     "Switching to page %1 of clients.".arg(pageNum))
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
