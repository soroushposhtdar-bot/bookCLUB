// =============================================================================
//  InputField.qml
// =============================================================================
//  Universal text input for the Authentication module. Supports:
//      • Leading icon (Material Symbols name)
//      • Trailing icon (Material Symbols name) + click handler
//      • Floating-style label that sits *above* the field (design system rule)
//      • Helper text (muted) — always-visible guidance
//      • Validation feedback — error / success / warning inline message
//      • Required-field asterisk
//      • Clear-text trailing button (optional)
//
//  Public API:
//      label           : string   — field label (rendered above)
//      placeholder     : string   — placeholder text
//      text            : string   — bound to field value (read/write)
//      leadingIcon     : string   — Material Symbols name
//      trailingIcon    : string   — Material Symbols name (manual, e.g. "search")
//      helperText      : string   — muted guidance under the field
//      errorText       : string   — overrides helperText in error state
//      successText     : string   — overrides helperText in success state
//      required        : bool     — show "*" next to label
//      showClearButton : bool     — show "close" trailing icon when text present
//      state           : string   — "default" | "focus" | "error" | "success" | "warning" | "disabled"
//      enabled         : bool
//      echoMode        : enum     — TextField.Normal / Password / PasswordEchoOnEdit / NoEcho
//      inputMethodHints: flags    — passed through
//      validator       : Validator— optional QML Validator attached
//      maximumLength   : int      — character cap, -1 = unlimited
//
//  Signals:
//      textEdited(string newText)   — emitted on every edit
//      accepted()                   — Enter / Return pressed
//      trailingClicked()            — trailing icon clicked
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    // ----- Public API -----
    property string label: ""
    property string placeholder: ""
    property string text: ""
    property string leadingIcon: ""
    property string trailingIcon: ""
    property string helperText: ""
    property string errorText: ""
    property string successText: ""
    property string warningText: ""
    property bool   required: false
    property bool   showClearButton: false
    property string state: "default"   // default | focus | error | success | warning | disabled
    property int    echoMode: TextInput.Normal
    property int    inputMethodHints: Qt.ImhNone
    property var    validator: null
    property int    maximumLength: -1
    property bool   passwordMode: false  // hint to keyboard
    property bool   selectAllOnFocus: false
    property bool   readOnly: false      // display-only (e.g. combobox-style fields)

    // Compute effective state
    readonly property string _effectiveState: {
        if (!enabled) return "disabled"
        if (errorText.length > 0) return "error"
        if (successText.length > 0) return "success"
        if (warningText.length > 0) return "warning"
        if (_textField.activeFocus) return "focus"
        return "default"
    }

    // Border color from state
    readonly property color _borderColor: {
        switch (_effectiveState) {
            case "focus":    return Theme.color.accent
            case "error":    return Theme.color.error
            case "success":  return Theme.color.success
            case "warning":  return Theme.color.warning
            case "disabled": return Theme.color.border
            default:         return Theme.color.border
        }
    }

    // ----- Signals -----
    signal textEdited(string newText)
    signal accepted()
    signal trailingClicked()
    signal editingFinished()

    // ----- Layout -----
    implicitWidth: 380
    implicitHeight: _column.implicitHeight

    Column {
        id: _column
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space.sm

        // ----- Label row -----
        Item {
            width: parent.width
            height: _label.visible ? _label.implicitHeight : 0
            visible: root.label.length > 0

            Row {
                id: _label
                spacing: 2
                Text {
                    text: root.label
                    color: !root.enabled ? Theme.color.textMuted : Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightMedium
                }
                Text {
                    text: "*"
                    color: Theme.color.error
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightMedium
                    visible: root.required
                }
            }
        }

        // ----- Field container -----
        Item {
            id: _fieldContainer
            width: parent.width
            height: Theme.size.fieldHeight

            // Background rectangle
            Rectangle {
                id: _bg
                anchors.fill: parent
                radius: Theme.radius.md
                color: !root.enabled ? Theme.color.fieldDisabled
                     : root._effectiveState === "focus" ? Theme.color.fieldBackground
                     : Theme.color.fieldBackground
                border.color: root._borderColor
                border.width: root._effectiveState === "focus" ? 2 : 1

                Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                Behavior on border.width { NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
            }

            // Focus glow
            Rectangle {
                anchors.fill: _bg
                anchors.margins: -2
                radius: _bg.radius + 2
                color: "transparent"
                border.color: Qt.rgba(26/255, 115/255, 232/255, 0.18)
                border.width: 4
                visible: root._effectiveState === "focus"
                z: -1
                Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }
            }

            // Content row
            Row {
                id: _row
                anchors.fill: parent
                anchors.leftMargin: Theme.space.md
                anchors.rightMargin: Theme.space.md
                spacing: Theme.space.sm

                // Leading icon
                Item {
                    width: root.leadingIcon.length > 0 ? Theme.size.iconMd : 0
                    height: parent.height
                    visible: root.leadingIcon.length > 0

                    AppIcon {
                        name: root.leadingIcon
                        size: Theme.size.iconMd
                        color: !root.enabled ? Theme.color.textMuted
                              : root._effectiveState === "focus" ? Theme.color.accent
                              : root._effectiveState === "error" ? Theme.color.error
                              : root._effectiveState === "success" ? Theme.color.success
                              : Theme.color.textSecondary
                        anchors.centerIn: parent
                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                    }
                }

                // TextField
                TextField {
                    id: _textField
                    width: parent.width - _row.spacing * 2
                            - (root.leadingIcon.length > 0 ? Theme.size.iconMd : 0)
                            - (_trailingItem.width)
                    height: parent.height
                    text: root.text
                    placeholderText: root.placeholder
                    placeholderTextColor: Theme.color.textMuted
                    color: !root.enabled ? Theme.color.textMuted : Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBodyLarge
                    verticalAlignment: TextInput.AlignVCenter
                    echoMode: root.echoMode
                    inputMethodHints: root.inputMethodHints
                    maximumLength: root.maximumLength
                    selectByMouse: true
                    enabled: root.enabled
                    readOnly: root.readOnly
                    passwordCharacter: "●"
                    background: Item { anchors.fill: parent }

                    // Update bound text property on each edit
                    onTextEdited: {
                        root.text = text
                        root.textEdited(text)
                    }
                    onEditingFinished: root.editingFinished()
                    onAccepted: root.accepted()
                    onFocusChanged: {
                        if (focus && root.selectAllOnFocus) selectAll()
                    }

                    // Active focus → component knows we're focused (drives _effectiveState)
                    onActiveFocusChanged: root.state = root.state  // trigger re-eval
                }

                // Trailing item (icon button or clear button)
                Item {
                    id: _trailingItem
                    width: {
                        if (root.showClearButton && root.text.length > 0 && root.enabled)
                            return Theme.size.iconMd
                        if (root.trailingIcon.length > 0)
                            return Theme.size.iconMd
                        return 0
                    }
                    height: parent.height

                    IconButton {
                        anchors.centerIn: parent
                        iconName: root.showClearButton && root.text.length > 0 && root.enabled ? "close" : root.trailingIcon
                        iconSize: Theme.size.iconMd
                        iconColor: Theme.color.textMuted
                        hoverIconColor: Theme.color.textPrimary
                        visible: parent.width > 0
                        onClicked: {
                            if (root.showClearButton && root.text.length > 0) {
                                root.text = ""
                                _textField.text = ""
                                root.textEdited("")
                            } else {
                                root.trailingClicked()
                            }
                        }
                    }
                }
            }
        }

        // ----- Helper / validation text -----
        Item {
            width: parent.width
            height: _helperRow.implicitHeight
            visible: _helperRow.implicitHeight > 0

            Row {
                id: _helperRow
                spacing: Theme.space.xs

                property string _message: {
                    if (root._effectiveState === "error" && root.errorText.length > 0)   return root.errorText
                    if (root._effectiveState === "success" && root.successText.length > 0) return root.successText
                    if (root._effectiveState === "warning" && root.warningText.length > 0) return root.warningText
                    return root.helperText
                }
                property color _color: {
                    switch (root._effectiveState) {
                        case "error":   return Theme.color.error
                        case "success": return Theme.color.success
                        case "warning": return Theme.color.warning
                        default:        return Theme.color.textMuted
                    }
                }
                property string _icon: {
                    switch (root._effectiveState) {
                        case "error":   return "error_outline"
                        case "success": return "check_circle"
                        case "warning": return "warning_amber"
                        default:        return ""
                    }
                }

                AppIcon {
                    name: parent._icon
                    size: Theme.size.iconSm
                    color: parent._color
                    visible: parent._icon.length > 0
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: parent._message
                    color: parent._color
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightRegular
                    visible: text.length > 0
                    wrapMode: Text.WordWrap
                    width: root.width - (parent._icon.length > 0 ? Theme.size.iconSm + Theme.space.xs : 0)
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // ----- Public functions -----
    function forceActiveFocus() {
        _textField.forceActiveFocus()
    }

    function clear() {
        text = ""
        _textField.text = ""
    }
}
