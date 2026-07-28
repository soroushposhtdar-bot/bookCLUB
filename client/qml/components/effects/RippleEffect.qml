// =============================================================================
//  RippleEffect.qml
// =============================================================================
//  Material-style ripple animation triggered on click/tap. Drop this inside
//  any Item's `background` and call `ripple(mouseX, mouseY)` from a
//  MouseArea's `onPressed` to fire a ripple from that point.
//
//  Public API:
//      color        : color  (ripple tint)
//      maxOpacity   : real   (peak opacity, decays from here to 0)
//      duration     : int    (ms)
//      radius       : int    (clip radius, matches parent's rounded corners)
//      centered     : bool   (force ripple to start from center, not the
//                             click point — useful for keyboard activation)
//
//  Functions:
//      ripple(real x, real y)  — fire a ripple from (x, y) in parent coords
//      rippleCentered()        — fire from center
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root
    anchors.fill: parent

    property color color: Theme.ripple.color
    property real maxOpacity: Theme.ripple.maxOpacity
    property int duration: Theme.ripple.duration
    property int radius: 0
    property bool centered: false

    clip: radius > 0
    layer.enabled: radius > 0
    layer.effect: null

    // Reusable ripple template — cloned per click for overlap support
    Component {
        id: _rippleComp
        Rectangle {
            property real originX: 0
            property real originY: 0
            property real maxR: 0
            x: originX - width / 2
            y: originY - height / 2
            width: 0
            height: width
            radius: width / 2
            color: root.color
            opacity: 0
            scale: 1.0
            clip: true

            ParallelAnimation {
                id: _anim
                NumberAnimation {
                    target: parent
                    property: "width"
                    from: 0
                    to: parent.maxR
                    duration: root.duration
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    target: parent
                    property: "opacity"
                    from: root.maxOpacity
                    to: 0.0
                    duration: root.duration
                    easing.type: Easing.InQuad
                }
                onStopped: parent.destroy()
            }
            Component.onCompleted: _anim.start()
        }
    }

    function ripple(x, y) {
        if (root.centered) { rippleCentered(); return; }
        var maxR = Math.max(
            Math.hypot(x, y),
            Math.hypot(x - root.width, y),
            Math.hypot(x, y - root.height),
            Math.hypot(x - root.width, y - root.height)
        );
        var r = _rippleComp.createObject(root, {
            "originX": x, "originY": y, "maxR": maxR * 2.2
        });
    }

    function rippleCentered() {
        var cx = root.width / 2, cy = root.height / 2;
        var maxR = Math.hypot(cx, cy);
        var r = _rippleComp.createObject(root, {
            "originX": cx, "originY": cy, "maxR": maxR * 2.2
        });
    }
}
