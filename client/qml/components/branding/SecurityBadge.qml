// =============================================================================
//  SecurityBadge.qml
// =============================================================================
//  Small pill-shaped badge — shield icon + label — used in the hero panel
//  of auth screens to communicate the "secure & private" promise.
//
//  Public API:
//      label : string
//      icon  : string  (Material Symbols name)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Item {
    id: root

    property string label: "Secure & Private"
    property string icon: "shield"

    implicitWidth: _row.implicitWidth + 2 * Theme.space.md
    implicitHeight: 32

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: height / 2
        color: Qt.rgba(255, 255, 255, 0.85)
        border.color: Theme.color.border
        border.width: 1
    }

    Row {
        id: _row
        anchors.centerIn: parent
        spacing: Theme.space.xs

        AppIcon {
            name: root.icon
            size: Theme.size.iconSm
            color: Theme.color.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.label
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
