// =============================================================================
//  BrandLogo.qml
// =============================================================================
//  BookClub brand mark — hexagonal tile with stylized open-book glyph.
//  Used in the hero panel of every auth screen.
//
//  Public API:
//      size  : int    — overall diameter
//      filled: bool   — solid black bg vs. outlined
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Item {
    id: root

    property int size: Theme.size.logoSize
    property bool filled: true

    implicitWidth: size
    implicitHeight: size

    // Hexagonal background
    Canvas {
        id: _hex
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.beginPath()
            var cx = width/2, cy = height/2, r = Math.min(width, height)/2 - 1
            for (var i = 0; i < 6; i++) {
                var a = Math.PI/6 + i * Math.PI/3
                var x = cx + r * Math.cos(a)
                var y = cy + r * Math.sin(a)
                if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
            }
            ctx.closePath()
            ctx.fillStyle = root.filled ? Theme.color.primary : "transparent"
            ctx.fill()
            ctx.lineWidth = 2
            ctx.strokeStyle = Theme.color.primary
            ctx.stroke()
        }
    }

    // Book glyph (centered) — simplified "menu_book" Material outline
    AppIcon {
        name: "auto_stories"
        size: root.size * 0.5
        color: root.filled ? Theme.color.onPrimary : Theme.color.primary
        anchors.centerIn: parent
    }

    onFilledChanged: _hex.requestPaint()
    onSizeChanged: _hex.requestPaint()
}
