// =============================================================================
//  NotificationsPage.qml
// =============================================================================
//  Notifications center — full rebuild.
//
//  Layout (top → bottom):
//      1. Header — "Notifications" title + unread badge ("N new") +
//         "Mark all read" TextButton.
//      2. Category tabs row — horizontally scrollable. Each tab is a
//         GenreChip with a count badge; active when selected.
//         Categories: All / Purchase / Review / Discount / Recommendation /
//         Publisher / System / Security / Reminder.
//      3. Search bar — SearchField bound to viewModel.searchQuery.
//      4. Notifications list — vertical ListView of NotificationItem
//         components. Each item is wrapped to support right-click context
//         menu (Mark read/unread, Archive, Delete). Left-click marks read
//         and emits bookDetailRequested when bookId is present.
//      5. Empty state — EmptyIllustration "You're all caught up".
//      6. Loading state — 4 skeleton notification rows.
//
//  Real-time:
//      viewModel.realtimeNotificationReceived fires → this page emits the
//      same signal upward (the App router shows a toast via ToastManager).
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/data"
import "../components/feedback"
import "../components/progress"
import "../components/navigation"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // NotificationsViewModel

    signal bookDetailRequested(string bookId)
    signal realtimeNotificationReceived(var notification)

    readonly property int _horizontalPadding: Theme.space.xxxl

    // ----- Category catalog (label + iconName + viewModel count property) -----
    readonly property var _categories: [
        { key: "all",            label: "All",             icon: "notifications" },
        { key: "purchase",       label: "Purchase",        icon: "shopping_cart" },
        { key: "review",         label: "Review",          icon: "rate_review" },
        { key: "discount",       label: "Discount",        icon: "local_offer" },
        { key: "recommendation", label: "Recommendation",  icon: "auto_stories" },
        { key: "publisher",      label: "Publisher",       icon: "campaign" },
        { key: "system",         label: "System",          icon: "info_outline" },
        { key: "security",       label: "Security",        icon: "shield" },
        { key: "reminder",       label: "Reminder",        icon: "schedule" }
    ]

    // -------------------------------------------------------------------------
    //  Page background
    // -------------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // -------------------------------------------------------------------------
    //  Scrollable content
    // -------------------------------------------------------------------------
    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.lg

            Item { width: 1; height: Theme.space.sm }

            // -----------------------------------------------------------------
            //  1. Header
            // -----------------------------------------------------------------
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.md

                Text {
                    text: "Notifications"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Unread badge
                Rectangle {
                    visible: root.viewModel && root.viewModel.unreadCount > 0
                    width: _unreadText.implicitWidth + 16
                    height: 26
                    radius: 13
                    color: Theme.color.accent
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        id: _unreadText
                        anchors.centerIn: parent
                        text: "%1 new".arg(root.viewModel ? root.viewModel.unreadCount : 0)
                        color: Theme.color.textOnAccent
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightBold
                    }

                    // Pop-in animation
                    scale: 1.0
                    Behavior on scale {
                        NumberAnimation {
                            duration: Theme.motion.durationBase
                            easing.type: Easing.OutBack
                        }
                    }
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                TextButton {
                    text: "Mark all read"
                    iconName: "done_all"
                    visible: root.viewModel && root.viewModel.unreadCount > 0
                    onClicked: {
                        if (root.viewModel) root.viewModel.markAllRead()
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // -----------------------------------------------------------------
            //  2. Category tabs (horizontally scrollable)
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: 44

                Flickable {
                    anchors.fill: parent
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    contentWidth: _categoriesRow.implicitWidth
                    contentHeight: height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.HorizontalFlick
                    ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOff }

                    Row {
                        id: _categoriesRow
                        spacing: Theme.space.sm

                        Repeater {
                            model: root._categories
                            delegate: Item {
                                width: _catChip.width
                                height: 38
                                anchors.verticalCenter: parent.verticalCenter

                                // Count badge (only rendered if count > 0)
                                Rectangle {
                                    visible: _countForCategory(modelData.key) > 0
                                    anchors.right: _catChip.right
                                    anchors.top: _catChip.top
                                    anchors.rightMargin: -4
                                    anchors.topMargin: -4
                                    width: _catCountText.implicitWidth + 10
                                    height: 18
                                    radius: 9
                                    color: modelData.key === (root.viewModel ? root.viewModel.activeCategory : "all")
                                           ? Theme.color.primary
                                           : Theme.color.accent
                                    border.color: Theme.color.cardBackground
                                    border.width: 2
                                    z: 2

                                    Text {
                                        id: _catCountText
                                        anchors.centerIn: parent
                                        text: _countForCategory(modelData.key)
                                        color: Theme.color.textOnPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeMicro
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                GenreChip {
                                    id: _catChip
                                    label: modelData.label
                                    iconName: modelData.icon
                                    selected: root.viewModel && root.viewModel.activeCategory === modelData.key
                                    onClicked: {
                                        if (root.viewModel) root.viewModel.setActiveCategory(modelData.key)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  3. Search bar
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: 56

                SearchField {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    anchors.verticalCenter: parent.verticalCenter
                    height: 48
                    placeholder: "Search notifications…"
                    text: root.viewModel ? root.viewModel.searchQuery : ""
                    onTextEdited: {
                        if (root.viewModel) root.viewModel.setSearchQuery(newText)
                    }
                }
            }

            // -----------------------------------------------------------------
            //  4. Notifications list / states
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: _listColumn.implicitHeight

                Column {
                    id: _listColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.sm

                    // ----- Loading state — 4 skeleton rows -----
                    Column {
                        width: parent.width
                        visible: root.viewModel && root.viewModel.isBusy
                        spacing: Theme.space.sm

                        Repeater {
                            model: 4
                            delegate: Rectangle {
                                width: parent.width
                                height: 76
                                radius: Theme.radius.md
                                color: Theme.color.cardBackground
                                border.color: Theme.color.divider
                                border.width: 1

                                Row {
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    spacing: Theme.space.md

                                        SkeletonLoader {
                                            width: 44
                                            height: 44
                                            shape: "circle"
                                        }

                                        Column {
                                            width: parent.width - 44 - Theme.space.md
                                            spacing: 6
                                            anchors.verticalCenter: parent.verticalCenter

                                            SkeletonLoader {
                                                width: parent.width * 0.6
                                                height: 14
                                                radius: Theme.radius.xs
                                            }
                                            SkeletonLoader {
                                                width: parent.width * 0.9
                                                height: 12
                                                radius: Theme.radius.xs
                                            }
                                            SkeletonLoader {
                                                width: parent.width * 0.75
                                                height: 12
                                                radius: Theme.radius.xs
                                            }
                                        }
                                }
                            }
                        }
                    }

                    // ----- Empty state -----
                    EmptyIllustration {
                        width: parent.width
                        height: 420
                        visible: root.viewModel && !root.viewModel.isBusy && !root.viewModel.hasAny
                        iconName: "notifications"
                        title: "You're all caught up"
                        description: root.viewModel && root.viewModel.searchQuery.length > 0
                                     ? "No notifications match your search."
                                     : "Notifications about new books, discounts, and reviews will appear here."
                    }

                    // ----- Notifications list -----
                    Repeater {
                        id: _notifRepeater
                        model: root.viewModel && !root.viewModel.isBusy ? root.viewModel.notifications : []

                        delegate: Item {
                            width: parent.width
                            height: 76

                            NotificationItem {
                                id: _notif
                                anchors.fill: parent
                                notification: modelData
                                onClicked: {
                                    if (root.viewModel && !modelData.read) {
                                        root.viewModel.markRead(modelData.id)
                                    }
                                    if (modelData.bookId && modelData.bookId.length > 0) {
                                        root.bookDetailRequested(modelData.bookId)
                                    }
                                }
                            }

                            // Right-click context menu surface
                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                z: 1
                                onClicked: {
                                    if (mouse.button === Qt.RightButton) {
                                        _itemMenu.actions = _buildItemActions(modelData)
                                        _itemMenu.openAt(mouse.x, mouse.y)
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

    // -------------------------------------------------------------------------
    //  Shared context menu for notification items
    // -------------------------------------------------------------------------
    ContextMenu {
        id: _itemMenu
        parent: root
    }

    // -------------------------------------------------------------------------
    //  Helpers
    // -------------------------------------------------------------------------
    function _countForCategory(key) {
        if (!root.viewModel) return 0
        switch (key) {
            case "all":            return root.viewModel.allCount
            case "purchase":       return root.viewModel.purchaseCount
            case "review":         return root.viewModel.reviewCount
            case "discount":       return root.viewModel.discountCount
            case "recommendation": return root.viewModel.recommendationCount
            case "publisher":      return root.viewModel.publisherCount
            case "system":         return root.viewModel.systemCount
            case "security":       return root.viewModel.securityCount
            case "reminder":       return root.viewModel.reminderCount
        }
        return 0
    }

    function _buildItemActions(notif) {
        var list = []
        if (notif.read) {
            list.push({
                text: "Mark as unread",
                iconName: "mark_email_unread",
                action: function() { if (root.viewModel) root.viewModel.markUnread(notif.id) }
            })
        } else {
            list.push({
                text: "Mark as read",
                iconName: "mark_email_read",
                action: function() { if (root.viewModel) root.viewModel.markRead(notif.id) }
            })
        }
        list.push({
            text: "Archive",
            iconName: "archive",
            action: function() { if (root.viewModel) root.viewModel.archiveNotification(notif.id) }
        })
        list.push({ separator: true })
        list.push({
            text: "Delete",
            iconName: "delete_outline",
            destructive: true,
            action: function() { if (root.viewModel) root.viewModel.deleteNotification(notif.id) }
        })
        return list
    }

    // -------------------------------------------------------------------------
    //  Real-time relay — surface the signal upward so App.qml can show a toast
    // -------------------------------------------------------------------------
    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onRealtimeNotificationReceived: {
            root.realtimeNotificationReceived(notification)
        }
    }
}
