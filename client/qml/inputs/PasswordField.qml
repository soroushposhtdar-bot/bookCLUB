// =============================================================================
//  PasswordField.qml
// =============================================================================
//  Specialised InputField for password entry.
//
//  Additions over InputField:
//      • Always uses echoMode = Password
//      • Visibility toggle (eye icon) in trailing slot — handled internally
//        by binding trailingIcon to the current visibility state and toggling
//        on trailingClicked.
//      • Optional strength meter rendered under the field
//      • Optional strength label ("Weak", "Fair", "Good", "Strong")
//
//  Public API additions:
//      showStrengthMeter : bool   — render the strength meter bar
//      strengthScore     : int    — 0..4 (caller-driven; ViewModel computes)
//      strengthLabel     : string — "Weak" / "Fair" / "Good" / "Strong"
//
//  Strength meter is purely cosmetic — strength score is computed by the
//  bound ViewModel (which can reuse bookclub::common::ValidationUtils).
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    // ----- Re-exposed InputField API -----
    property string label: "Password"
    property string placeholder: "Enter your password"
    property string text: ""
    property string leadingIcon: "lock"
    property string helperText: ""
    property string errorText: ""
    property string successText: ""
    property string warningText: ""
    property bool   required: true
    property string state: "default"
    property bool   enabled: true
    property int    maximumLength: 64

    // ----- Password-specific API -----
    property bool showStrengthMeter: false
    property int  strengthScore: 0      // 0..4
    property string strengthLabel: ""

    // ----- Signals -----
    signal textEdited(string newText)
    signal accepted()
    signal editingFinished()

    // ----- Internal: visibility state -----
    property bool _visible: false

    implicitWidth: 380
    implicitHeight: _column.implicitHeight

    Column {
        id: _column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        InputField {
            id: _core
            width: parent.width
            label: root.label
            placeholder: root.placeholder
            text: root.text
            leadingIcon: root.leadingIcon
            helperText: root.helperText
            errorText: root.errorText
            successText: root.successText
            warningText: root.warningText
            required: root.required
            state: root.state
            enabled: root.enabled
            maximumLength: root.maximumLength
            echoMode: root._visible ? TextInput.Normal : TextInput.Password

            // Visibility toggle in trailing slot
            showClearButton: false
            trailingIcon: root._visible ? "visibility_off" : "visibility"
            onTrailingClicked: root._visible = !root._visible

            onTextEdited: {
                root.text = newText
                root.textEdited(newText)
            }
            onAccepted: root.accepted()
            onEditingFinished: root.editingFinished()
        }

        // Strength meter (optional)
        Item {
            width: parent.width
            height: root.showStrengthMeter && root.text.length > 0
                    ? _strengthColumn.implicitHeight + Theme.space.md
                    : 0
            visible: height > 0

            Column {
                id: _strengthColumn
                anchors.fill: parent
                spacing: Theme.space.xs

                // Bar
                Row {
                    width: parent.width
                    spacing: Theme.space.xxs

                    Repeater {
                        model: 4
                        Rectangle {
                            width: (parent.width - 3 * Theme.space.xxs) / 4
                            height: 4
                            radius: 2
                            color: {
                                if (index >= root.strengthScore) return Theme.color.divider
                                switch (root.strengthScore) {
                                    case 1: return Theme.color.error
                                    case 2: return Theme.color.warning
                                    case 3: return Theme.color.accent
                                    case 4: return Theme.color.success
                                    default: return Theme.color.divider
                                }
                            }
                            Behavior on color { ColorAnimation { duration: Theme.motion.durationBase } }
                        }
                    }
                }

                Text {
                    text: root.strengthLabel
                    color: switch (root.strengthScore) {
                        case 1: return Theme.color.error
                        case 2: return Theme.color.warning
                        case 3: return Theme.color.accent
                        case 4: return Theme.color.success
                        default: return Theme.color.textMuted
                    }
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightMedium
                }
            }
        }
    }

    function forceActiveFocus() { _core.forceActiveFocus() }
    function clear() { _core.clear(); root.text = "" }
}
