// =============================================================================
//  SkeletonLoader.qml
// =============================================================================
//  Shimmer placeholder used while real content loads. Drop one (or several)
//  over the area where the content will appear, then `visible = false` once
//  the data arrives.
//
//  Public API:
//      width / height : inherited from parent (use anchors.fill)
//      radius         : int  — corner radius (matches the shape being simulated)
//      shape          : string — "rect" | "pill" | "circle"
//      active         : bool — when false, hides itself and stops the animation
//
//  The shimmer band sweeps left → right on an infinite loop. Color comes from
//  Theme.skeleton so it adapts to light/dark automatically.
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property int radius: Theme.radius.md
    property string shape: "rect"      // rect | pill | circle
    property bool active: true
    visible: active

    Rectangle {
        id: _base
        anchors.fill: parent
        radius: root.shape === "circle" ? Math.min(width, height) / 2
              : root.shape === "pill"   ? height / 2
              :                            root.radius
        color: Theme.skeleton.base
        clip: true

        // Shimmer band — a translucent gradient rectangle sweeping across
        Rectangle {
            id: _shimmer
            width: parent.width * Theme.skeleton.shimmerWidth
            height: parent.height
            x: -width
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: Theme.skeleton.highlight }
                GradientStop { position: 1.0; color: "transparent" }
            }
            opacity: 0.85

            NumberAnimation on x {
                from: -_shimmer.width
                to: _base.width
                duration: Theme.skeleton.shimmerDuration
                loops: Animation.Infinite
                running: root.active
                easing.type: Easing.InOutQuad
            }
        }
    }
}
