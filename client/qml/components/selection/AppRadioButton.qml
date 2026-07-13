// =============================================================================
//  AppRadioButton.qml
// =============================================================================
//  Circular radio button with optional label. Used in auth for choosing
//  security questions from a list, and "Remember me for 30 days" options
//  where multiple options exist.
//
//  Radio groups are managed by the caller via the `group` attached property
//  (or simply by binding `checked` from the parent ViewModel).
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Item {
    id: root

    property bool checked: false
    property bool enabled: true
    property string label: ""
    property int dotSize: 18

    signal toggled(bool checked)

    implicitWidth: _row.implicitWidth
    implicitHeight: Math.max(_outer.height, _label.implicitHeight)

    MouseArea {
        id: _ma
        anchors.fill: parent
        enabled: root.enabled
        onClicked: {
            if (!root.checked) {
                root.checked = true
                root.toggled(true)
            }
        }
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        hoverEnabled: true
    }

    Row {
        id: _row
        spacing: Theme.space.sm

        Rectangle {
            id: _outer
            width: root.dotSize
            height: root.dotSize
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            color: "transparent"
            border.color: !root.enabled ? Theme.color.border
                        :  root.checked ? Theme.color.primary
                        :  _ma.containsMouse ? Theme.color.accent
                        :  Theme.color.borderStrong
            border.width: 1.5

            Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }

            Rectangle {
                id: _inner
                width: parent.width * 0.55
                height: parent.height * 0.55
                radius: width / 2
                anchors.centerIn: parent
                color: root.enabled ? Theme.color.primary : Theme.color.textMuted
                scale: root.checked ? 1.0 : 0.0
                Behavior on scale { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutBack } }
            }
        }

        Text {
            id: _label
            text: root.label
            color: root.enabled ? Theme.color.textPrimary : Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Rectangle {
        anchors.fill: _outer
        anchors.margins: -3
        radius: _outer.radius + 3
        color: "transparent"
        border.color: Theme.color.accent
        border.width: 2
        visible: root.activeFocus && root.enabled
        z: -1
    }
}
