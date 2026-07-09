// =============================================================================
//  Spinner.qml
// =============================================================================
//  Determinate/indeterminate circular progress indicator.
//  Material-style: thin arc that rotates; for determinate mode the arc fills
//  proportional to `progress`.
//
//  Public API:
//      size     : int    — pixel diameter
//      color    : color  — arc color
//      trackColor: color — background ring color
//      progress : real   — 0..1, -1 for indeterminate (default)
//      thickness: int    — ring thickness in px
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property int size: 24
    property color color: Theme.color.accent
    property color trackColor: Qt.rgba(0, 0, 0, 0.08)
    property real progress: -1   // -1 = indeterminate
    property int thickness: 3

    implicitWidth: size
    implicitHeight: size

    // Track ring
    Canvas {
        id: _track
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.beginPath()
            ctx.arc(width/2, height/2, (width - root.thickness)/2, 0, 2 * Math.PI)
            ctx.lineWidth = root.thickness
            ctx.strokeStyle = root.trackColor
            ctx.lineCap = "round"
            ctx.stroke()
        }
    }

    // Progress ring
    Canvas {
        id: _arc
        anchors.fill: parent
        rotation: root.progress < 0 ? _indeterminateAnim.rotation : 0

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var angleSpan = root.progress < 0 ? Math.PI * 1.4 : Math.max(0.001, root.progress) * 2 * Math.PI
            ctx.beginPath()
            ctx.arc(width/2, height/2, (width - root.thickness)/2, -Math.PI/2, -Math.PI/2 + angleSpan)
            ctx.lineWidth = root.thickness
            ctx.strokeStyle = root.color
            ctx.lineCap = "round"
            ctx.stroke()
        }

        onProgressChanged: requestPaint()
        Component.onCompleted: requestPaint()
    }

    // Indeterminate rotation animation
    QtObject {
        id: _indeterminateAnim
        property real rotation: 0
    }

    RotationAnimation on rotation {
        target: _arc
        running: root.progress < 0
        loops: Animation.Infinite
        from: 0
        to: 360
        duration: 900
        easing.type: Easing.InOutQuad
    }

    onProgressChanged: _arc.requestPaint()
    onColorChanged: _arc.requestPaint()
    onTrackColorChanged: _track.requestPaint()
    onSizeChanged: { _track.requestPaint(); _arc.requestPaint() }
}
