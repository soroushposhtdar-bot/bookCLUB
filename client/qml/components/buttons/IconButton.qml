// =============================================================================
//  IconButton.qml  (v3 polish)
// =============================================================================
//  Square, borderless, icon-only button. Used for "back", "close",
//  "show/hide password", "refresh", and other small affordances.
//
//  Visual states:
//      • normal   — transparent, muted icon
//      • hover    — soft gray circle background (now using Theme.ripple.colorHover)
//      • pressed  — darker gray circle, scale 0.92 (now using Theme.ripple.colorPressed)
//      • disabled — 35% opacity
//
//  v3 polish bug fix: previously the background used `Theme.ripple.colorHover`
//  and `Theme.ripple.colorPressed`, but those keys were undefined in Theme →
//  the hover/pressed background was `undefined`, which Qt silently coerced to
//  `transparent`. The button looked dead on hover. Now that Theme defines
//  both keys properly, the hover/pressed tints actually show.
//
//  v3 polish also adds:
//    • `tooltip` property — when set, a hover-delayed Tooltip appears.
//    • `badge` property — small red dot for "new item" indicators.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import ".."

Button {
    id: control

    property string iconName: ""
    property int iconSize: Theme.size.iconMd
    property color iconColor: Theme.color.textSecondary
    property color hoverIconColor: Theme.color.textPrimary
    // Optional tooltip shown after a 600ms hover delay.
    property string tooltip: ""
    // Optional small red badge dot (e.g. "new" indicator).
    property bool badge: false

    implicitWidth: 40
    implicitHeight: 40
    padding: 0

    hoverEnabled: enabled

    contentItem: Item {
        anchors.fill: parent

        AppIcon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            color: !control.enabled ? Theme.color.textMuted
                  : control.pressed ? control.iconColor
                  : control.hovered  ? control.hoverIconColor
                  : control.iconColor
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        // Small badge dot in the top-right corner.
        Rectangle {
            visible: control.badge && control.enabled
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 6
            anchors.rightMargin: 6
            width: 8; height: 8; radius: 4
            color: Theme.color.error
            border.color: Theme.color.cardBackground
            border.width: 1.5
        }
    }

    background: Rectangle {
        radius: width / 2
        color: !control.enabled ? "transparent"
             : control.pressed ? Theme.ripple.colorPressed
             : control.hovered  ? Theme.ripple.colorHover
             : "transparent"
        border.width: 0

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }

    transform: Scale {
        origin.x: control.width / 2
        origin.y: control.height / 2
        xScale: control.pressed ? 0.92 : 1.0
        yScale: control.pressed ? 0.92 : 1.0
        Behavior on xScale { NumberAnimation { duration: Theme.motion.durationInstant; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: Theme.motion.durationInstant; easing.type: Easing.OutCubic } }
    }

    // Tooltip — only loaded when `tooltip` is set, to avoid paying the
    // HoverHandler cost on every IconButton in the app.
    Loader {
        active: control.tooltip.length > 0
        sourceComponent: _tooltipComp
    }
    Component {
        id: _tooltipComp
        ToolTip {
            text: control.tooltip
            delay: 600
            timeout: 4000
            visible: control.hovered && control.enabled
        }
    }
}
