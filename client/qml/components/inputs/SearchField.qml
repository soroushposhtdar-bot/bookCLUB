// =============================================================================
//  SearchField.qml
// =============================================================================
//  Search-styled input — leading "search" icon, optional trailing "close"
//  clear button. Same border/focus language as InputField.
//
//  Public API:
//      placeholder : string
//      text        : string
//      enabled     : bool
//
//  Signals:
//      textEdited(string)
//      accepted()
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property string placeholder: "Search"
    property string text: ""
    property bool enabled: true

    signal textEdited(string newText)
    signal accepted()

    implicitWidth: 280
    implicitHeight: Theme.size.fieldHeight

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: Theme.radius.md
        color: !root.enabled ? Theme.color.fieldDisabled : Theme.color.fieldFilled
        border.color: _tf.activeFocus ? Theme.color.accent : Theme.color.border
        border.width: _tf.activeFocus ? 2 : 1

        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
        Behavior on border.width { NumberAnimation { duration: Theme.motion.durationFast } }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.space.md
        anchors.rightMargin: Theme.space.md
        spacing: Theme.space.sm

        AppIcon {
            name: "search"
            size: Theme.size.iconMd
            color: _tf.activeFocus ? Theme.color.accent : Theme.color.textMuted
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        TextField {
            id: _tf
            width: parent.width - Theme.space.md * 2 - Theme.size.iconMd - Theme.space.sm
                   - (root.text.length > 0 ? Theme.size.iconMd : 0)
            height: parent.height
            text: root.text
            placeholderText: root.placeholder
            placeholderTextColor: Theme.color.textMuted
            color: root.enabled ? Theme.color.textPrimary : Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBodyLarge
            verticalAlignment: TextInput.AlignVCenter
            background: Item {}
            selectByMouse: true
            enabled: root.enabled

            onTextEdited: {
                root.text = text
                root.textEdited(text)
            }
            onAccepted: root.accepted()
        }

        IconButton {
            iconName: "close"
            iconSize: Theme.size.iconMd
            iconColor: Theme.color.textMuted
            hoverIconColor: Theme.color.textPrimary
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text.length > 0 && root.enabled
            onClicked: {
                root.text = ""
                _tf.text = ""
                root.textEdited("")
                _tf.forceActiveFocus()
            }
        }
    }

    function forceActiveFocus() { _tf.forceActiveFocus() }
    function clear() { root.text = ""; _tf.text = "" }
}
