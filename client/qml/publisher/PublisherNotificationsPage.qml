// =============================================================================
//  PublisherNotificationsPage.qml
// =============================================================================
//  Publisher-specific notifications: sales milestones, review alerts,
//  platform announcements, and promo performance.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `publisherNotifications` (QVariantList of { type, icon, title, body,
//  time, read }) plus `markAllNotificationsRead()` and
//  `clearReadNotifications()`.
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
import BookClub.ViewModels 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)

    // ----- Notifications (QVariantList from the VM) -----
    readonly property var _notifications: page.viewModel ? page.viewModel.publisherNotifications : []

    function _toneColor(t) {
        return { success: Theme.color.success, info: Theme.color.info, warning: Theme.color.warning, error: Theme.color.error }[t] || Theme.color.info
    }
    function _toneSoft(t) {
        return { success: Theme.color.successSoft, info: Theme.color.infoSoft, warning: Theme.color.warningSoft, error: Theme.color.errorSoft }[t] || Theme.color.infoSoft
    }
    function _unreadCount() {
        if (!page._notifications) return 0
        let n = 0
        for (let i = 0; i < page._notifications.length; ++i) {
            if (page._notifications[i].read === false) ++n
        }
        return n
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header row -----
            Card {
                width: parent.width
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                Row {
                    width: parent.width
                    spacing: Theme.space.md

                    Column {
                        spacing: 0
                        anchors.verticalCenter: parent.verticalCenter
                        Text { text: "%1 unread".arg(page._unreadCount()); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold }
                        Text { text: "%1 total notifications".arg(page._notifications ? page._notifications.length : 0); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    SecondaryButton {
                        text: "Mark all as read"
                        iconName: "done_all"
                        onClicked: {
                            if (page.viewModel) page.viewModel.markAllNotificationsRead()
                            page.toastRequested("success", "All caught up", "Every notification has been marked as read.")
                        }
                    }
                    SecondaryButton {
                        text: "Clear read"
                        iconName: "delete_outline"
                        onClicked: {
                            if (page.viewModel) page.viewModel.clearReadNotifications()
                            page.toastRequested("info", "Cleared", "Read notifications have been removed.")
                        }
                    }
                }
            }

            // ----- Notifications list -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.sm

                    ListView {
                        width: parent.width
                        height: contentHeight
                        clip: true
                        interactive: false
                        model: page._notifications
                        spacing: 0

                        delegate: Item {
                            width: parent.width
                            height: 80

                            Rectangle {
                                anchors.fill: parent
                                color: modelData.read ? "transparent" : Theme.color.accentSoft
                                opacity: 0.4
                                visible: !modelData.read
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.md
                                anchors.rightMargin: Theme.space.md
                                spacing: Theme.space.md

                                Rectangle {
                                    width: 40; height: 40; radius: 12
                                    color: page._toneSoft(modelData.type)
                                    anchors.verticalCenter: parent.verticalCenter
                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: modelData.icon
                                        size: 20
                                        color: page._toneColor(modelData.type)
                                    }
                                }

                                Column {
                                    width: parent.width - 40 - Theme.space.md * 2 - 32 - Theme.space.md
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter

                                    Row {
                                        width: parent.width
                                        spacing: Theme.space.sm
                                        Text {
                                            text: modelData.title
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: modelData.read ? Theme.font.weightMedium : Theme.font.weightBold
                                            elide: Text.ElideRight
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                        Text {
                                            text: modelData.time
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                    Text {
                                        width: parent.width
                                        text: modelData.body
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        wrapMode: Text.WordWrap
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        textFormat: Text.RichText
                                    }
                                }

                                IconButton {
                                    iconName: modelData.read ? "check_box" : "radio_button_unchecked"
                                    anchors.verticalCenter: parent.verticalCenter
                                    // Per-item mark-as-read: toggles this single
                                    // notification's read state via the VM's
                                    // markNotificationRead(id, read) method.
                                    onClicked: {
                                        if (page.viewModel && typeof page.viewModel.markNotificationRead === "function") {
                                            page.viewModel.markNotificationRead(modelData.id || "", !modelData.read)
                                        }
                                    }
                                }
                            }

                            Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 1; color: Theme.color.divider }
                        }
                    }

                    EmptyState {
                        width: parent.width
                        height: 200
                        visible: !page._notifications || page._notifications.length === 0
                        iconName: "notifications"
                        title: "No notifications"
                        description: "You're all caught up. New alerts will appear here."
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
