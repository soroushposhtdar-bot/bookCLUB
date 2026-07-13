// =============================================================================
//  FilterChip.qml
// =============================================================================
//  Removable filter chip — shows the active filter value with an "x" button
//  on the right. Used by the Search page and Wishlist filters.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Item {
    id: root
    implicitWidth: _row.implicitWidth + 2 * Theme.space.md
    implicitHeight: 32

    property string label: ""
    property string iconName: "filter_alt"

    signal removeClicked()
    signal clicked()

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.pill
        color: _ma.containsMouse ? Theme.color.accentSoft : Theme.color.fieldFilled
        border.color: Theme.color.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
    }

    Row {
        id: _row
        anchors.centerIn: parent
        spacing: 6

        AppIcon {
            name: root.iconName
            size: 14
            color: Theme.color.textSecondary
            visible: name.length > 0
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.label
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
        }

        Item {
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: _xMa.containsMouse ? Theme.color.borderStrong : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            }
            AppIcon {
                anchors.centerIn: parent
                name: "close"
                size: 12
                color: Theme.color.textSecondary
            }
            MouseArea {
                id: _xMa
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.removeClicked()
            }
        }
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
        z: -1
    }
}
