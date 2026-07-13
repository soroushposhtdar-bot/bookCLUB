// =============================================================================
//  Avatar.qml
// =============================================================================
//  Circular user avatar — shows initials over a solid colored background.
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property string initials: "?"
    property string displayName: ""
    property int size: Theme.size.avatarSize
    property color backgroundColor: Theme.color.primary
    property bool online: false

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        id: _circle
        anchors.fill: parent
        radius: width / 2
        color: root.backgroundColor
        border.color: Theme.color.cardBackground
        border.width: 2
    }

    Text {
        anchors.centerIn: parent
        text: root.initials.length > 0 ? root.initials : "?"
        color: Theme.color.textOnPrimary
        font.family: Theme.font.family
        font.pixelSize: Math.max(11, root.size * 0.40)
        font.weight: Theme.font.weightBold
    }

    // Online dot
    Rectangle {
        visible: root.online
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: root.size * 0.28
        height: width
        radius: width / 2
        color: Theme.color.success
        border.color: Theme.color.cardBackground
        border.width: 2
    }
}
