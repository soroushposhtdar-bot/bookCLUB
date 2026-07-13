// =============================================================================
//  TextButton.qml
// =============================================================================
//  Low-emphasis text-only button — used for inline links inside body copy
//  ("Forgot password?", "Create account", "Resend code").
//
//  Always uses the accent color, no background. Hover lifts the color
//  slightly and underlines the text. Pressed state scales 0.99.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

Button {
    id: control

    property string iconName: ""
    property bool underlineOnHover: true
    property color color: Theme.color.accent
    property color hoverColor: Theme.color.accentHover

    padding: Theme.space.xs
    implicitHeight: 28
    implicitWidth: _row.implicitWidth + 2 * padding

    hoverEnabled: enabled
    background: Item { anchors.fill: parent }

    contentItem: Row {
        id: _row
        spacing: Theme.space.xs

        AppIcon {
            name: control.iconName
            size: Theme.size.iconSm
            color: control.hovered ? control.hoverColor : control.color
            visible: control.iconName.length > 0
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        Text {
            text: control.text
            color: !control.enabled ? Theme.color.textMuted
                 : control.hovered  ? control.hoverColor
                 : control.color
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }

            Rectangle {
                anchors.baseline: parent.baseline
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -1
                height: 1
                color: parent.color
                visible: control.underlineOnHover && control.hovered
            }
        }
    }
}
