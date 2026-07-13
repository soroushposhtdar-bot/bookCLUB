// =============================================================================
//  Divider.qml
// =============================================================================
//  Horizontal (default) or vertical thin separator.
//
//  Public API:
//      orientation : string — "horizontal" | "vertical"
//      label       : string — optional text label centered on the divider
//                             (e.g. "Or" between Login and alt-auth sections)
//      color       : color
//      thickness   : int
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property string orientation: "horizontal"
    property string label: ""
    property color color: Theme.color.divider
    property int thickness: 1

    implicitWidth: orientation === "horizontal" ? 200 : thickness
    implicitHeight: orientation === "horizontal" ? (label.length > 0 ? 24 : thickness) : 200

    // No-label case: simple line
    Rectangle {
        visible: root.label.length === 0
        anchors.centerIn: parent
        width:  root.orientation === "horizontal" ? parent.width : root.thickness
        height: root.orientation === "horizontal" ? root.thickness : parent.height
        color: root.color
    }

    // Labeled case: line — label — line (horizontal only)
    Row {
        visible: root.label.length > 0 && root.orientation === "horizontal"
        anchors.fill: parent
        spacing: Theme.space.md

        Rectangle {
            width: (parent.width - _label.width - 2 * Theme.space.md) / 2
            height: root.thickness
            color: root.color
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: _label
            text: root.label
            color: Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            width: (parent.width - _label.width - 2 * Theme.space.md) / 2
            height: root.thickness
            color: root.color
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
