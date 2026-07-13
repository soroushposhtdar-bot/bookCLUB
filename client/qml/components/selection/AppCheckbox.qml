// =============================================================================
//  AppCheckbox.qml
// =============================================================================
//  Square checkbox with Material Symbols check glyph, optional label.
//  Two-state (checked/unchecked); tristate not supported (not needed for auth).
//
//  States:
//      • unchecked  — transparent bg, border
//      • checked    — primary bg, white check
//      • hover      — soft accent tint
//      • focus      — accent ring
//      • disabled   — 50% opacity
//
//  Animations: 140ms color/scale transitions on check toggle.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    property string label: ""
    property string helperText: ""
    property int boxSize: 18

    signal toggled(bool checked)

    implicitWidth: _row.implicitWidth
    implicitHeight: Math.max(_box.height, _labelCol.implicitHeight)

    MouseArea {
        id: _ma
        anchors.fill: parent
        enabled: root.enabled
        onClicked: {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true
    }

    Row {
        id: _row
        spacing: Theme.space.sm

        // Checkbox box
        Rectangle {
            id: _box
            width: root.boxSize
            height: root.boxSize
            radius: 4
            anchors.verticalCenter: parent.verticalCenter
            color: !root.enabled ? Theme.color.fieldDisabled
                 :  root.checked ? Theme.color.primary
                 :  _ma.containsMouse ? Theme.color.accentSoft
                 :  "transparent"
            border.color: !root.enabled ? Theme.color.border
                        :  root.checked ? Theme.color.primary
                        :  _ma.containsMouse ? Theme.color.accent
                        :  Theme.color.borderStrong
            border.width: 1.5

            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }

            AppIcon {
                name: "check"
                size: 14
                color: Theme.color.onPrimary
                anchors.centerIn: parent
                visible: root.checked
                scale: root.checked ? 1.0 : 0.0
                Behavior on scale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutBack } }
            }
        }

        // Label + helper
        Column {
            id: _labelCol
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            visible: root.label.length > 0 || root.helperText.length > 0

            Text {
                text: root.label
                color: root.enabled ? Theme.color.textPrimary : Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: Theme.font.weightRegular
                visible: root.label.length > 0
            }

            Text {
                text: root.helperText
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                visible: root.helperText.length > 0
            }
        }
    }

    // Focus ring
    Rectangle {
        anchors.fill: _box
        anchors.margins: -3
        radius: _box.radius + 3
        color: "transparent"
        border.color: Theme.color.accent
        border.width: 2
        visible: root.activeFocus && root.enabled
        z: -1
    }

    function toggle() {
        if (enabled) {
            checked = !checked
            toggled(checked)
        }
    }
}
