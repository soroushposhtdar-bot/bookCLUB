// =============================================================================
//  IconButton.qml
// =============================================================================
//  Square, borderless, icon-only button. Used for "back", "close",
//  "show/hide password", "refresh", and other small affordances.
//
//  Visual states:
//      • normal   — transparent, muted icon
//      • hover    — soft gray circle background
//      • pressed  — darker gray circle, scale 0.92
//      • disabled — 35% opacity
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

Button {
    id: control

    property string iconName: ""
    property int iconSize: Theme.size.iconMd
    property color iconColor: Theme.color.textSecondary
    property color hoverIconColor: Theme.color.textPrimary

    implicitWidth: 40
    implicitHeight: 40
    padding: 0

    hoverEnabled: enabled

    contentItem: AppIcon {
        name: control.iconName
        size: control.iconSize
        color: !control.enabled ? Qt.rgba(0,0,0,0.30)
              : control.pressed ? control.iconColor
              : control.hovered  ? control.hoverIconColor
              : control.iconColor
        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
    }

    background: Rectangle {
        radius: width / 2
        color: !control.enabled ? "transparent"
             : control.pressed ? Qt.rgba(0, 0, 0, 0.10)
             : control.hovered  ? Qt.rgba(0, 0, 0, 0.06)
             : "transparent"
        border.width: 0

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }

    transform: Scale {
        origin.x: control.width / 2
        origin.y: control.height / 2
        xScale: control.pressed ? 0.92 : 1.0
        yScale: control.pressed ? 0.92 : 1.0
        Behavior on xScale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
    }
}
