// =============================================================================
//  DropShadowBase.qml
// =============================================================================
//  Tiny convenience wrapper for drop shadows, used as a `layer.effect`.
//
//  Qt6 strategy (no Qt5Compat required):
//    Uses MultiEffect from QtQuick.Effects (available since Qt 6.5).
//    MultiEffect can be used directly as a layer.effect: it auto-binds the
//    parent item's layer texture to its `source` property.
//
//  Qt5Compat fallback (optional):
//    If you explicitly enabled BOOKCLUB_USE_QT5COMPAT at configure time and
//    your Qt installation has the Qt5Compat module, you can swap this file
//    for the legacy `import Qt5Compat.GraphicalEffects` + `DropShadow {}`
//    implementation. The API (a `colorSpec` var) is preserved either way.
//
//  Usage (inside a `layer.effect:`):
//      layer.enabled: true
//      layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }
//
//  NOTE: The consuming item must have `layer.enabled: true` and a
//  transparent background so the effect samples the right pixels.
// =============================================================================
import QtQuick
import QtQuick.Effects

MultiEffect {
    // Accepts one of Theme.shadow entries: { color, blur, offsetY }
    property var colorSpec: ({ "color": Theme.shadow.md.color, "blur": 16, "offsetY": 6 })

    // MultiEffect's blur/shadow strength is normalized to [0.0, 1.0].
    // Theme.shadow.*.blur is in pixels (4..32), so we map blur/32 → [0,1].
    readonly property real _blurNorm: Math.max(0.0, Math.min(1.0, (colorSpec.blur || 16) / 32.0))

    // Shadow configuration
    shadowEnabled: true
    shadowColor: colorSpec.color || Theme.shadow.md.color
    shadowBlur: _blurNorm
    shadowBlurMax: 32
    shadowVerticalOffset: colorSpec.offsetY || 6
    shadowHorizontalOffset: 0
    shadowScale: 1.0
    shadowOpacity: 1.0

    // Make sure MultiEffect does NOT touch brightness/contrast/saturation
    brightness: 0.0
    contrast: 0.0
    saturation: 0.0

    // Don't apply a blur to the source item itself; only emit the shadow.
    blurEnabled: false

    // When used as a layer.effect, MultiEffect's `source` property is
    // auto-set by Qt to the parent item's layer texture. It then fills
    // the parent's area automatically — no explicit width/height needed.
}
