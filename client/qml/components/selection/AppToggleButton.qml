// =============================================================================
//  AppToggleButton.qml
// =============================================================================
//  Pill-shaped toggle switch (iOS/Material style). Used in auth for
//  "Remember me" when a binary persistence toggle is preferred over a checkbox.
//
//  States:
//      • off        — gray track, white knob on left
//      • on         — primary track, white knob on right
//      • hover      — slight color shift
//      • disabled   — 40% opacity
//      • focus      — accent ring around the track
//
//  Animation: 220ms eased knob slide + track color transition.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../effects"

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    property string label: ""

    signal toggled(bool checked)

    implicitWidth: label.length > 0 ? _row.implicitWidth : _track.width
    implicitHeight: Math.max(_track.height, _label.implicitHeight)

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

        Rectangle {
            id: _track
            width: 40
            height: 22
            radius: height / 2
            anchors.verticalCenter: parent.verticalCenter
            color: !root.enabled ? Qt.rgba(0, 0, 0, 0.10)
                 :  root.checked ? Theme.color.primary
                 :  _ma.containsMouse ? Theme.color.borderStrong
                 :  Theme.color.border

            Behavior on color { ColorAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }

            Rectangle {
                id: _knob
                width: 18
                height: 18
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                color: "white"
                x: root.checked ? parent.width - width - 2 : 2

                Behavior on x { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }

                layer.enabled: true
                layer.effect: DropShadowBase {
                    colorSpec: { "color": "rgba(0,0,0,0.18)", "blur": 4, "offsetY": 1 }
                }
            }
        }

        Text {
            id: _label
            text: root.label
            color: root.enabled ? Theme.color.textPrimary : Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            anchors.verticalCenter: parent.verticalCenter
            visible: root.label.length > 0
        }
    }

    Rectangle {
        anchors.fill: _track
        anchors.margins: -3
        radius: _track.radius + 3
        color: "transparent"
        border.color: Theme.color.accent
        border.width: 2
        visible: root.activeFocus && root.enabled
        z: -1
    }
}
