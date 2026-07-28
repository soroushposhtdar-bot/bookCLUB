// =============================================================================
//  PublisherTopBar.qml  (v9 rewrite)
// =============================================================================
//  Top bar for the Publisher shell. Visually consistent with the user-role
//  TopBar, but WITHOUT the cart button (publishers don't have a cart).
//
//  Layout (left → right), using RowLayout so the spacer actually fills:
//    [theme toggle] [notifications bell + badge]
//    [page title + subtitle]              ←spacer→
//    [user name] [avatar w/ chevron → profile button]
//
//  v9 fixes vs. the previous version:
//    • Switched the root Row → RowLayout. The previous version used a plain
//      Row with an `Item { width: 1; height: 1 }` "spacer" — but Row does
//      not expand children, so the spacer was 1px wide and the user name
//      + avatar sat immediately next to the title instead of being pushed
//      to the right edge. RowLayout's `Layout.fillWidth: true` on the
//      spacer makes it actually consume the remaining space.
//    • Made the avatar a clear "profile button" — added a hover background
//      tint, a tooltip ("View profile"), and a small chevron icon so the
//      user can see it's clickable. Previously the avatar had only a
//      MouseArea with a PointingHandCursor — easy to miss.
//    • Added an optional `roleChip` ("Publisher") that can be hidden via
//      `showRoleChip`. Default true so the role is always visible.
//
//  Public API:
//      title          : string
//      subtitle       : string  (optional)
//      userName       : string
//      userInitials   : string
//      unreadCount    : int
//      showRoleChip   : bool   (default true)
//
//  Signals:
//      notificationsRequested()
//      profileRequested()
//      themeToggled()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../../theme"
import ".."
import "../buttons"
import "../inputs"
import "."

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property string userName: ""
    property string userInitials: "?"
    property int unreadCount: 0
    property bool showRoleChip: true

    signal notificationsRequested()
    signal profileRequested()
    signal themeToggled()

    implicitHeight: Theme.size.topbarHeight

    Rectangle {
        anchors.fill: parent
        color: Theme.color.topbarBackground

        // Bottom divider line — matches the user-role TopBar styling.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.color.divider
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.space.xxl
        anchors.rightMargin: Theme.space.xl
        spacing: Theme.space.md

        // ----- Theme toggle (LEFT) -----
        IconButton {
            iconName: Theme.isDark ? "light_mode" : "dark_mode"
            iconColor: Theme.color.textSecondary
            hoverIconColor: Theme.color.textPrimary
            tooltip: Theme.isDark ? "Switch to light mode" : "Switch to dark mode"
            Layout.alignment: Qt.AlignVCenter
            onClicked: root.themeToggled()
        }

        // ----- Notifications bell with badge -----
        Item {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignVCenter

            IconButton {
                anchors.fill: parent
                iconName: "notifications"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                tooltip: root.unreadCount > 0
                         ? ("%1 unread notification%2".arg(root.unreadCount).arg(root.unreadCount === 1 ? "" : "s"))
                         : "Notifications"
                onClicked: root.notificationsRequested()
            }

            Rectangle {
                visible: root.unreadCount > 0
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 4
                anchors.rightMargin: 4
                width: Math.max(18, _bellBadge.implicitWidth + 10)
                height: 18
                radius: 9
                color: Theme.color.accent

                Text {
                    id: _bellBadge
                    anchors.centerIn: parent
                    text: root.unreadCount > 99 ? "99+" : String(root.unreadCount)
                    color: Theme.color.textOnAccent
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeMicro
                    font.weight: Theme.font.weightBold
                }
            }
        }

        // ----- Title block -----
        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: false
            spacing: 0
            visible: root.title.length > 0

            Text {
                text: root.title
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightBold
                elide: Text.ElideRight
            }
            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightRegular
            }
        }

        // ----- Spacer (fills remaining space so name+avatar push RIGHT) -----
        // v9 fix: Layout.fillWidth makes this Item actually expand. The
        // previous version used `Item { width: 1; height: 1 }` inside a
        // plain Row, which left the spacer at 1px and the name/avatar
        // stuck in the middle of the bar.
        Item { Layout.fillWidth: true; Layout.fillHeight: true }

        // ----- "Publisher" role chip (optional, before the name) -----
        Rectangle {
            visible: root.showRoleChip
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: _roleChipLbl.implicitWidth + 18
            Layout.preferredHeight: 24
            radius: height / 2
            color: Theme.color.accentSoft
            border.color: Qt.rgba(Theme.color.accent.r,
                                  Theme.color.accent.g,
                                  Theme.color.accent.b, 0.25)
            border.width: 1

            Text {
                id: _roleChipLbl
                anchors.centerIn: parent
                text: "PUBLISHER"
                color: Theme.color.accent
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeMicro2
                font.weight: Theme.font.weightBold
                letterSpacing: 0.5
            }
        }

        // ----- User name (RIGHT) -----
        Text {
            text: root.userName
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightMedium
            Layout.alignment: Qt.AlignVCenter
            elide: Text.ElideRight
            Layout.maximumWidth: 200
        }

        // ----- Profile button (avatar + chevron, with hover state) -----
        // v9: replaced the bare MouseArea-over-avatar with a real button
        // affordance — a rounded rectangle that tints on hover + a small
        // dropdown chevron so the user knows it opens the profile page.
        Rectangle {
            id: _profileBtn
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: _profileRow.implicitWidth + 16
            Layout.preferredHeight: Theme.size.avatarSize + 8
            radius: height / 2
            color: _profileHover.hovered ? Theme.ripple.colorHover : "transparent"
            border.width: 1
            border.color: _profileHover.hovered ? Theme.color.border : "transparent"

            HoverHandler {
                id: _profileHover
                cursorShape: Qt.PointingHandCursor
            }

            RowLayout {
                id: _profileRow
                anchors.centerIn: parent
                spacing: Theme.space.xs

                Avatar {
                    Layout.preferredWidth: Theme.size.avatarSize
                    Layout.preferredHeight: Theme.size.avatarSize
                    initials: root.userInitials
                    size: Theme.size.avatarSize
                    online: true
                }

                AppIcon {
                    name: "expand_more"
                    size: 18
                    color: _profileHover.hovered ? Theme.color.textPrimary : Theme.color.textSecondary
                }
            }

            ToolTip {
                text: "View profile"
                delay: 600
                timeout: 4000
                visible: _profileHover.hovered
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.profileRequested()
            }
        }
    }
}
