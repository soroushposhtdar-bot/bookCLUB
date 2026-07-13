// =============================================================================
//  NavItem.qml
// =============================================================================
//  Single sidebar navigation item — icon + label, supports active state and
//  an optional trailing badge (e.g. unread count).
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"

Item {
    id: root

    property string iconName: ""
    property string label: ""
    property bool active: false
    property bool collapsed: false
    property int badgeCount: 0

    signal clicked()

    implicitWidth: parent ? parent.width : 200
    implicitHeight: Theme.size.navItemHeight

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: Theme.radius.md
        color: root.active ? Theme.color.sidebarItemActive
             : _ma.containsMouse ? Theme.color.sidebarItemHover
             : "transparent"

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: root.collapsed ? 0 : Theme.space.lg
        anchors.right: parent.right
        anchors.rightMargin: Theme.space.lg
        spacing: Theme.space.md

        // Center the icon when collapsed
        Item {
            width: Theme.size.iconMd
            height: Theme.size.iconMd
            anchors.verticalCenter: parent.verticalCenter
            x: root.collapsed ? (root.width - Theme.size.iconMd) / 2 - Theme.space.lg : 0

            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: Theme.size.iconMd
                color: root.active ? Theme.color.sidebarItemActiveFg
                     : _ma.containsMouse ? Theme.color.textPrimary
                     : Theme.color.textSecondary
                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            }
        }

        Text {
            visible: !root.collapsed
            text: root.label
            color: root.active ? Theme.color.textPrimary
                 : _ma.containsMouse ? Theme.color.textPrimary
                 : Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: root.active ? Theme.font.weightSemibold : Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        Item { Layout.fillWidth: true; width: 1; height: 1 }

        // Badge
        Rectangle {
            visible: !root.collapsed && root.badgeCount > 0
            width: Math.max(20, _badgeText.implicitWidth + 12)
            height: 20
            radius: 10
            color: Theme.color.accent
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: _badgeText
                anchors.centerIn: parent
                text: root.badgeCount > 99 ? "99+" : String(root.badgeCount)
                color: Theme.color.textOnAccent
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightBold
            }
        }

        // Collapsed badge dot
        Rectangle {
            visible: root.collapsed && root.badgeCount > 0
            width: 8; height: 8; radius: 4
            color: Theme.color.accent
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: -2
            anchors.rightMargin: -2
        }
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
