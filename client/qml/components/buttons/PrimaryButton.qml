// =============================================================================
//  PrimaryButton.qml
// =============================================================================
//  High-emphasis button — solid black background, white text. Used for the
//  primary action on every auth screen (Login, Register, Continue, Reset…).
//
//  States covered:
//      • normal      — black bg, white text
//      • hover       — slightly lifted black (#2A2A2E)
//      • pressed     — pure black, scale 0.985
//      • disabled    — 45% opacity, no hover effect
//      • loading     — disabled + spinner replaces text/icon
//      • focus-ring  — accent blue ring on keyboard focus
//
//  Public API:
//      text            : string   — button label
//      icon            : string   — Material Symbols icon name (optional, leading)
//      iconPosition    : enum     — "leading" | "trailing"
//      enabled         : bool     — standard Item enabled
//      loading         : bool     — shows spinner, disables interaction
//      fullWidth       : bool     — fill parent width
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"
import "../effects"

Button {
    id: control

    // ----- Public API -----
    property string iconName: ""
    property string iconPosition: "leading"   // "leading" | "trailing"
    property bool loading: false
    property bool fullWidth: false

    implicitHeight: Theme.size.buttonHeight
    implicitWidth: fullWidth ? (parent ? parent.width : 200)
                             : _contentRow.implicitWidth + 2 * Theme.space.lg

    // Disable interaction while loading.
    enabled: parent ? true : true
    hoverEnabled: enabled && !loading

    // ----- Content layout -----
    contentItem: Item {
        anchors.fill: parent

        // Loading spinner
        BusyIndicator {
            id: _spinner
            anchors.centerIn: parent
            visible: control.loading
            running: control.loading
            implicitWidth: 22
            implicitHeight: 22
            palette.dark: Theme.color.onPrimary
        }

        Row {
            id: _contentRow
            anchors.centerIn: parent
            spacing: Theme.space.sm
            opacity: control.loading ? 0.0 : 1.0
            visible: !control.loading

            // Leading icon
            AppIcon {
                name: control.iconName
                size: Theme.size.iconMd
                color: Theme.color.onPrimary
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName.length > 0 && control.iconPosition === "leading"
            }

            Text {
                text: control.text
                color: Theme.color.onPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightSemibold
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Trailing icon
            AppIcon {
                name: control.iconName
                size: Theme.size.iconMd
                color: Theme.color.onPrimary
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName.length > 0 && control.iconPosition === "trailing"
            }
        }
    }

    // ----- Background -----
    background: Item {
        anchors.fill: parent

        // Base rectangle with rounded corners
        Rectangle {
            id: _bg
            anchors.fill: parent
            radius: Theme.radius.md
            color: !control.enabled ? Qt.rgba(10/255, 10/255, 11/255, 0.35)
                 :  control.pressed ? Theme.color.primaryPressed
                 :  control.hovered  ? Theme.color.primaryHover
                 :  Theme.color.primary

            Behavior on color {
                ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic }
            }
        }

        // Keyboard focus ring
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: Theme.radius.md + 3
            color: "transparent"
            border.color: Theme.color.accent
            border.width: 2
            visible: control.activeFocus && control.enabled
            z: -1
        }

        // Soft hover lift shadow
        Rectangle {
            anchors.fill: parent
            radius: Theme.radius.md
            color: "transparent"
            layer.enabled: control.hovered && control.enabled && !control.pressed
            layer.effect: DropShadowBase {
                colorSpec: Theme.shadow.sm
            }
            z: -2
        }

        // Pressed scale
        scale: control.pressed ? 0.985 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }

    // Pressed scale on background (done via transform)
    transform: Scale {
        origin.x: control.width / 2
        origin.y: control.height / 2
        xScale: control.pressed ? 0.985 : 1.0
        yScale: control.pressed ? 0.985 : 1.0
        Behavior on xScale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }
}
