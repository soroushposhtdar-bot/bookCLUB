// =============================================================================
//  GenreChip.qml
// =============================================================================
//  Pill-shaped selectable genre chip. Used by:
//      • Onboarding genre grid (multi-select)
//      • Search filter panel (multi-select)
//      • Profile genre editing (multi-select with cap)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"

Item {
    id: root

    property string label: ""
    property bool selected: false
    property bool enabled: true
    property string iconName: ""

    signal clicked()

    implicitWidth: _row.implicitWidth + 2 * Theme.space.lg
    implicitHeight: 38

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: Theme.radius.pill
        color: !root.enabled ? Theme.color.fieldDisabled
             : root.selected ? Theme.color.primary
             : _ma.containsMouse ? Theme.color.fieldFilled
             : Theme.color.cardBackground
        border.color: !root.enabled ? Theme.color.border
                    : root.selected ? Theme.color.primary
                    : _ma.containsMouse ? Theme.color.borderStrong
                    : Theme.color.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
    }

    Row {
        id: _row
        anchors.centerIn: parent
        spacing: Theme.space.xs

        AppIcon {
            name: root.iconName.length > 0 ? root.iconName
                  : root.selected ? "check" : ""
            size: 16
            color: root.selected ? Theme.color.onPrimary : Theme.color.textSecondary
            visible: name.length > 0
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.label
            color: !root.enabled ? Theme.color.textMuted
                 : root.selected ? Theme.color.onPrimary
                 : Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: root.selected ? Theme.font.weightSemibold : Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
