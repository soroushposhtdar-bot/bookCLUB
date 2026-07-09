// =============================================================================
//  DropShadowBase.qml
// =============================================================================
//  Tiny convenience wrapper around QtGraphicalEffects' DropShadow.
//  Consumes a "colorSpec" object (one of Theme.shadow entries) — keeps the
//  background-only shadow usage consistent across the entire auth module.
//
//  Usage (inside a `layer.effect:`):
//      layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }
//
//  NOTE: The consuming item must have `layer.enabled: true` and a
//  transparent background so the effect samples the right pixels.
// =============================================================================
import QtQuick 2.15
import QtGraphicalEffects 1.15

DropShadow {
    // Accepts one of Theme.shadow entries: { color, blur, offsetY }
    property var colorSpec: ({ "color": "rgba(0,0,0,0.10)", "blur": 16, "offsetY": 6 })

    color: colorSpec.color || "rgba(0,0,0,0.10)"
    radius: colorSpec.blur || 16
    samples: (colorSpec.blur || 16) * 2 + 1
    verticalOffset: colorSpec.offsetY || 6
    horizontalOffset: 0
    spread: 0
    transparentBorder: true
}
