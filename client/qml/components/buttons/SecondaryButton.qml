// =============================================================================
//  SecondaryButton.qml
// =============================================================================
//  Medium-emphasis button — white background with 1px border, dark text.
//  Used for "Back", "Cancel", secondary actions next to a PrimaryButton.
//
//  Visual states mirror PrimaryButton (normal / hover / pressed / disabled /
//  loading / focus-ring) — see PrimaryButton.qml for the full description.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

Button {
    id: control

    property string iconName: ""
    property string iconPosition: "leading"
    property bool loading: false
    property bool fullWidth: false

    implicitHeight: Theme.size.buttonHeight
    implicitWidth: fullWidth ? (parent ? parent.width : 200)
                             : _contentRow.implicitWidth + 2 * Theme.space.lg

    hoverEnabled: enabled && !loading

    contentItem: Item {
        anchors.fill: parent

        BusyIndicator {
            anchors.centerIn: parent
            visible: control.loading
            running: control.loading
            implicitWidth: 22
            implicitHeight: 22
            palette.dark: Theme.color.textPrimary
        }

        Row {
            id: _contentRow
            anchors.centerIn: parent
            spacing: Theme.space.sm
            opacity: control.loading ? 0.0 : 1.0
            visible: !control.loading

            AppIcon {
                name: control.iconName
                size: Theme.size.iconMd
                color: Theme.color.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName.length > 0 && control.iconPosition === "leading"
            }

            Text {
                text: control.text
                color: control.enabled ? Theme.color.textPrimary : Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightMedium
                anchors.verticalCenter: parent.verticalCenter
            }

            AppIcon {
                name: control.iconName
                size: Theme.size.iconMd
                color: Theme.color.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                visible: control.iconName.length > 0 && control.iconPosition === "trailing"
            }
        }
    }

    background: Item {
        anchors.fill: parent

        Rectangle {
            id: _bg
            anchors.fill: parent
            radius: Theme.radius.md
            color: !control.enabled ? Theme.color.fieldDisabled
                 :  control.pressed ? Theme.color.fieldFilled
                 :  control.hovered  ? Theme.color.fieldFilled
                 :  Theme.color.cardBackground
            border.color: control.enabled ? Theme.color.borderStrong : Theme.color.border
            border.width: 1

            Behavior on color {
                ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic }
            }
        }

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
    }

    transform: Scale {
        origin.x: control.width / 2
        origin.y: control.height / 2
        xScale: control.pressed ? 0.985 : 1.0
        yScale: control.pressed ? 0.985 : 1.0
        Behavior on xScale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }
}
