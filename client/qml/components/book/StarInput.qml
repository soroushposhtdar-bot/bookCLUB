// =============================================================================
//  StarInput.qml
// =============================================================================
//  Interactive 1–5 star rating input.
//
//  Public API:
//      value    : int   (0..5)
//      size     : int
//      enabled  : bool
//
//  Signals:
//      valueChanged(int value)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Row {
    id: root
    spacing: 4

    property int value: 0
    property int size: Theme.size.iconLg
    property bool enabled: true
    property color color: Theme.color.warning
    property color emptyColor: Qt.rgba(Theme.color.textMuted.r, Theme.color.textMuted.g, Theme.color.textMuted.b, 0.40)

    signal valueChanged(int value)

    Repeater {
        model: 5
        Item {
            width: root.size
            height: root.size
            anchors.verticalCenter: parent.verticalCenter

            property int starIndex: index + 1
            property bool hovered: _ma.containsMouse

            AppIcon {
                anchors.fill: parent
                name: (root.value >= starIndex || hovered) ? "star" : "star_outline"
                size: root.size
                color: (root.value >= starIndex || hovered) ? root.color : root.emptyColor
                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            }

            MouseArea {
                id: _ma
                anchors.fill: parent
                enabled: root.enabled
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.value = starIndex
                    root.valueChanged(starIndex)
                }
            }
        }
    }
}
