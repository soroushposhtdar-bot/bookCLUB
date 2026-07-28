// =============================================================================
//  PublisherNotificationsPage.qml  (v3 polish)
// =============================================================================
//  Publisher-specific notifications: sales milestones, review alerts,
//  platform announcements, and promo performance.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `publisherNotifications` (QVariantList of { type, icon, title, body,
//  time, read }) plus `markAllNotificationsRead()` and
//  `clearReadNotifications()`.
//
//  v3 polish improvements:
//    • Filter tabs (All / Unread / Sales / Reviews / System) — previously
//      the page showed everything with no way to narrow down.
//    • Layout switched to RowLayout/ColumnLayout — no more `parent.width`
//      arithmetic.
//    • Per-notification click toggles read state (was checkbox-only).
//    • Better empty state with a contextual message per filter.
//    • "Mark all as read" button disabled when nothing is unread (was
//      always enabled, leading to a confusing "All caught up" toast when
//      there was nothing to do).
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
import "../components"

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    // ----- Notifications -----
    readonly property var _notifications: page.viewModel ? page.viewModel.publisherNotifications : []

    // ----- Filter (All / Unread / Sales / Reviews / System) -----
    property string _filter: "all"

    function _matchesFilter(n) {
        switch (page._filter) {
            case "all":     return true
            case "unread":  return n.read === false
            case "sales":   return n.type === "success" || n.icon === "shopping_cart"
            case "reviews": return n.icon === "rate_review" || n.icon === "star"
            case "system":  return n.type === "info" || n.icon === "campaign"
        }
        return true
    }

    readonly property var _filtered: {
        if (!page._notifications) return []
        const out = []
        for (let i = 0; i < page._notifications.length; ++i) {
            if (page._matchesFilter(page._notifications[i])) out.push(page._notifications[i])
        }
        return out
    }

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
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header row -----
            Card {
                Layout.fillWidth: true
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: "%1 unread".arg(page._unreadCount())
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "%1 total notifications".arg(page._notifications ? page._notifications.length : 0)
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }
                    Item { Layout.fillWidth: true; height: 1 }
                    SecondaryButton {
                        text: "Mark all as read"
                        iconName: "done_all"
                        enabled: page._unreadCount() > 0
                        onClicked: {
                            if (page.viewModel) page.viewModel.markAllNotificationsRead()
                            page.toastRequested("success", "All caught up",
                                                 "Every notification has been marked as read.")
                        }
                    }
                    SecondaryButton {
                        text: "Clear read"
                        iconName: "delete_outline"
                        onClicked: {
                            if (page.viewModel) page.viewModel.clearReadNotifications()
                            page.toastRequested("info", "Cleared",
                                                 "Read notifications have been removed.")
                        }
                    }
                }
            }

            // ----- Filter tabs -----
            Card {
                Layout.fillWidth: true
                elevation: "none"
                bordered: true
                padding: Theme.space.md

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.sm

                    Repeater {
                        model: [
                            { key: "all",     label: "All" },
                            { key: "unread",  label: "Unread" },
                            { key: "sales",   label: "Sales" },
                            { key: "reviews", label: "Reviews" },
                            { key: "system",  label: "System" }
                        ]
                        GenreChip {
                            label: modelData.label
                            selected: page._filter === modelData.key
                            onClicked: page._filter = modelData.key
                        }
                    }
                    Item { Layout.fillWidth: true; height: 1 }
                    Text {
                        text: "%1 shown".arg(page._filtered.length)
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
            }

            // ----- Notifications list -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _notifsContent
                    width: parent.width
                    spacing: Theme.space.sm

                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        clip: true
                        interactive: false
                        model: page._filtered
                        spacing: 0

                        delegate: Item {
                            width: parent.width
                            height: 80

                            // v4: clicking anywhere on the row (except the
                            // IconButton on the right) toggles read state.
                            // The IconButton is rendered above this MouseArea
                            // (later in the children list) so its clicks
                            // take precedence.
                            MouseArea {
                                anchors.fill: parent
                                propagateComposedEvents: true
                                onClicked: {
                                    if (page.viewModel && typeof page.viewModel.markNotificationRead === "function") {
                                        page.viewModel.markNotificationRead(modelData.id || "", !modelData.read)
                                    }
                                    mouse.accepted = false
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: modelData.read ? "transparent" : Theme.color.accentSoft
                                opacity: 0.4
                                visible: !modelData.read

                                Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.md
                                anchors.rightMargin: Theme.space.md
                                spacing: Theme.space.md

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 12
                                    color: page._toneSoft(modelData.type)
                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: modelData.icon
                                        size: 20
                                        color: page._toneColor(modelData.type)
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space.sm
                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.title
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: modelData.read ? Theme.font.weightMedium : Theme.font.weightBold
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.time
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                    Text {
                                        Layout.fillWidth: true
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
                                    tooltip: modelData.read ? "Mark as unread" : "Mark as read"
                                    onClicked: {
                                        if (page.viewModel && typeof page.viewModel.markNotificationRead === "function") {
                                            page.viewModel.markNotificationRead(modelData.id || "", !modelData.read)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: Theme.color.divider
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        visible: page._filtered.length === 0
                        iconName: page._filter === "unread" ? "check_circle" : "notifications"
                        title: page._filter === "unread" ? "All caught up" : "No notifications"
                        description: {
                            if (page._filter === "unread")
                                return "You've read every notification. New alerts will appear here."
                            if (page._filter === "all")
                                return "You're all caught up. New alerts will appear here."
                            return "No %1 notifications right now. Try a different filter.".arg(page._filter)
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xxl }
        }
    }
}
