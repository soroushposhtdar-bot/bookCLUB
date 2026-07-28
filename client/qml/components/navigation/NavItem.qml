// =============================================================================
//  NavItem.qml
// =============================================================================
//  Single sidebar navigation item — icon + label, supports active state and
//  an optional trailing badge (e.g. unread count).
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import ".."

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

        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    // v11: Use RowLayout instead of Row so Layout.fillWidth works on the
    // spacer + text. This prevents the badge from overlapping the text.
    RowLayout {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.collapsed ? 0 : Theme.space.lg
        anchors.rightMargin: root.collapsed ? 0 : Theme.space.lg
        spacing: Theme.space.md

        // Center the icon when collapsed
        Item {
            Layout.preferredWidth: Theme.size.iconMd
            Layout.preferredHeight: Theme.size.iconMd
            Layout.alignment: Qt.AlignVCenter

            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: Theme.size.iconMd
                color: root.active ? Theme.color.sidebarItemActiveFg
                     : _ma.containsMouse ? Theme.color.textPrimary
                     : Theme.color.textSecondary
                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
            }
        }

        Text {
            visible: !root.collapsed
            Layout.fillWidth: true
            text: root.label
            color: root.active ? Theme.color.textPrimary
                 : _ma.containsMouse ? Theme.color.textPrimary
                 : Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: root.active ? Theme.font.weightSemibold : Theme.font.weightMedium
            elide: Text.ElideRight
            Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
        }

        // Badge — sits AFTER the text (which fills width), so it never overlaps
        Rectangle {
            visible: !root.collapsed && root.badgeCount > 0
            Layout.preferredWidth: Math.max(20, _badgeText.implicitWidth + 12)
            Layout.preferredHeight: 20
            Layout.alignment: Qt.AlignVCenter
            radius: 10
            color: Theme.color.accent

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
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            Layout.alignment: Qt.AlignTop | Qt.AlignRight
            radius: 4
            color: Theme.color.accent
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
