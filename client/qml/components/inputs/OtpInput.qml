// =============================================================================
//  OtpInput.qml
// =============================================================================
//  Multi-segment one-time-code / verification code input.
//
//  Renders N digit cells; typing auto-advances to the next, backspace returns
//  to the previous. Paste of a full code fills all cells. Keyboard friendly.
//
//  Public API:
//      length        : int    — number of cells (default 6)
//      value         : string — concatenated code (read/write)
//      state         : string — "default" | "focus" | "error" | "success"
//      secure         : bool   — mask entered digits (rare; for security codes)
//      cellSize      : int    — square cell edge in pixels
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

FocusScope {
    id: root

    property int length: 6
    property string value: ""
    property string state: "default"   // default | focus | error | success
    property bool secure: false
    property int cellSize: 52
    property int cellSpacing: 12

    signal completed(string code)
    signal edited(string code)

    implicitWidth: length * cellSize + (length - 1) * cellSpacing
    implicitHeight: cellSize

    // ----- Internal model -----
    onLengthChanged: _rebuildCells()
    Component.onCompleted: _rebuildCells()

    function _rebuildCells() {
        _repeater.model = root.length
    }

    function setValue(v) {
        var safe = (v || "").substring(0, root.length)
        root.value = safe
        for (var i = 0; i < root.length; i++) {
            _repeater.itemAt(i).char = i < safe.length ? safe[i] : ""
        }
        _recheckComplete()
    }

    function clear() {
        root.value = ""
        for (var i = 0; i < root.length; i++) {
            _repeater.itemAt(i).char = ""
        }
        if (_repeater.count > 0) _repeater.itemAt(0).activate()
    }

    function forceActiveFocus() {
        // Find first empty cell and focus it
        for (var i = 0; i < root.length; i++) {
            var cell = _repeater.itemAt(i)
            if (cell && cell.char.length === 0) { cell.activate(); return }
        }
        if (_repeater.count > 0) _repeater.itemAt(0).activate()
    }

    function _recheckComplete() {
        if (root.value.length === root.length) {
            root.completed(root.value)
        }
    }

    // ----- Layout -----
    Row {
        id: _row
        spacing: root.cellSpacing

        Repeater {
            id: _repeater
            model: root.length

            // ----- One cell -----
            Item {
                id: _cell
                width: root.cellSize
                height: root.cellSize
                property string char: ""

                function activate() { _tf.forceActiveFocus() }

                Rectangle {
                    id: _bg
                    anchors.fill: parent
                    radius: Theme.radius.md
                    color: Theme.color.fieldBackground
                    border.color: {
                        if (root.state === "error")   return Theme.color.error
                        if (root.state === "success") return Theme.color.success
                        if (_tf.activeFocus)          return Theme.color.accent
                        return Theme.color.border
                    }
                    border.width: _tf.activeFocus ? 2 : 1
                    Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
                    Behavior on border.width { NumberAnimation { duration: Theme.motion.durationFast } }
                }

                // Focus glow
                Rectangle {
                    anchors.fill: _bg
                    anchors.margins: -2
                    radius: _bg.radius + 2
                    color: "transparent"
                    border.color: Qt.rgba(26/255, 115/255, 232/255, 0.18)
                    border.width: 4
                    visible: _tf.activeFocus
                    z: -1
                }

                TextField {
                    id: _tf
                    anchors.fill: parent
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightSemibold
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    echoMode: root.secure ? TextInput.Password : TextInput.Normal
                    passwordCharacter: "●"
                    maximumLength: 1
                    inputMethodHints: Qt.ImhDigitsOnly
                    background: Item {}
                    text: _cell.char

                    onTextChanged: {
                        if (text.length > 1) {
                            // Paste handling: text is multiple chars → distribute
                            var pasteText = text
                            var idx = index
                            for (var i = 0; i < pasteText.length && idx + i < root.length; i++) {
                                var cell = _repeater.itemAt(idx + i)
                                if (cell) cell.char = pasteText[i]
                            }
                            // Move focus to the cell after the last filled
                            var lastIdx = Math.min(idx + pasteText.length, root.length) - 1
                            if (lastIdx + 1 < root.length) _repeater.itemAt(lastIdx + 1).activate()
                            else if (_repeater.itemAt(lastIdx)) _repeater.itemAt(lastIdx).activate()
                            // Rebuild value
                            var v = ""
                            for (var j = 0; j < root.length; j++) {
                                var c = _repeater.itemAt(j)
                                if (c) v += c.char
                            }
                            root.value = v
                            root.edited(v)
                            _recheckComplete()
                            return
                        }
                        if (text === _cell.char) return
                        _cell.char = text
                        // Rebuild root.value
                        var v2 = ""
                        for (var k = 0; k < root.length; k++) {
                            var c2 = _repeater.itemAt(k)
                            if (c2) v2 += c2.char
                        }
                        root.value = v2
                        root.edited(v2)
                        if (text.length === 1 && index + 1 < root.length) {
                            _repeater.itemAt(index + 1).activate()
                        }
                        _recheckComplete()
                    }

                    Keys.onPressed: {
                        if (event.key === Qt.Key_Backspace && _tf.text.length === 0 && index > 0) {
                            // Move to previous cell and clear it
                            var prev = _repeater.itemAt(index - 1)
                            if (prev) { prev.char = ""; prev.activate() }
                            event.accepted = true
                        } else if (event.key === Qt.Key_Left && index > 0) {
                            _repeater.itemAt(index - 1).activate()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Right && index + 1 < root.length) {
                            _repeater.itemAt(index + 1).activate()
                            event.accepted = true
                        }
                    }
                }
            }
        }
    }

    onStateChanged: {
        if (state === "error") {
            // Brief shake animation
            _shake.start()
        }
    }

    SequentialAnimation {
        id: _shake
        PropertyAnimation { target: _row; property: "x"; to: 6;  duration: Theme.motion.durationInstant }
        PropertyAnimation { target: _row; property: "x"; to: -6; duration: Theme.motion.durationInstant }
        PropertyAnimation { target: _row; property: "x"; to: 4;  duration: Theme.motion.durationInstant }
        PropertyAnimation { target: _row; property: "x"; to: -4; duration: Theme.motion.durationInstant }
        PropertyAnimation { target: _row; property: "x"; to: 0;  duration: Theme.motion.durationInstant }
    }
}
