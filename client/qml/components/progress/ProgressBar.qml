// =============================================================================
//  ProgressBar.qml
// =============================================================================
//  Linear progress bar (determinate or indeterminate).
//
//  Public API:
//      value    : real  — 0..1, -1 for indeterminate
//      height   : int
//      color    : color — bar color
//      trackColor: color — track color
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property real value: -1   // -1 = indeterminate
    property int barHeight: 4
    property color color: Theme.color.accent
    property color trackColor: Qt.rgba(0, 0, 0, 0.08)

    implicitWidth: 200
    implicitHeight: barHeight

    // Track
    Rectangle {
        anchors.fill: parent
        radius: root.barHeight / 2
        color: root.trackColor
    }

    // Fill
    Rectangle {
        id: _fill
        height: parent.height
        radius: root.barHeight / 2
        color: root.color

        // Determinate
        width: root.value >= 0 ? parent.width * Math.max(0, Math.min(1, root.value)) : parent.width * 0.35
        x: root.value >= 0 ? 0 : 0

        Behavior on width {
            enabled: root.value >= 0
            NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
        }
    }

    // Indeterminate animation
    SequentialAnimation {
        running: root.value < 0
        loops: Animation.Infinite

        ParallelAnimation {
            NumberAnimation { target: _fill; property: "x"; from: -_fill.width; to: root.width; duration: 1100; easing.type: Easing.InOutQuad }
            NumberAnimation { target: _fill; property: "opacity"; from: 0.0; to: 1.0; duration: 200 }
        }
        NumberAnimation { target: _fill; property: "opacity"; to: 0.0; duration: 200 }
    }
}
