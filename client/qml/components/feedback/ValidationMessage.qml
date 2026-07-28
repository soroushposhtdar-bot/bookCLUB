// =============================================================================
//  ValidationMessage.qml
// =============================================================================
//  Inline feedback row — icon + text — shown under form fields to communicate
//  validation state. Reused inside InputField but also available standalone
//  for cross-field validation messages (e.g. "Passwords do not match").
//
//  Public API:
//      type    : string — "error" | "success" | "warning" | "info"
//      text    : string
//      icon    : string — override default icon (Material Symbols name)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import ".."

Item {
    id: root

    property string type: "error"
    property string text: ""
    property string icon: ""

    // BUG FIX (Issue 27): the Row's inner Text had `wrapMode: WordWrap`
    // but no width boundary, so it didn't know where to wrap — long
    // server error messages rendered on a single line and overflowed
    // the form panel (clipped by AuthLayout's `clip: true`). We now
    // track the icon's presence and compute a wrapping width for the
    // Text so long messages wrap properly.
    implicitWidth: _row.implicitWidth
    implicitHeight: _row.implicitHeight

    visible: text.length > 0

    readonly property color _color: {
        switch (type) {
            case "error":   return Theme.color.error
            case "success": return Theme.color.success
            case "warning": return Theme.color.warning
            case "info":    return Theme.color.info
            default:        return Theme.color.textMuted
        }
    }

    readonly property string _icon: {
        if (root.icon.length > 0) return root.icon
        switch (type) {
            case "error":   return "error_outline"
            case "success": return "check_circle"
            case "warning": return "warning_amber"
            case "info":    return "info_outline"
            default:        return ""
        }
    }

    readonly property bool _hasIcon: root._icon.length > 0

    Row {
        id: _row
        spacing: Theme.space.xs
        width: root.width

        AppIcon {
            id: _iconItem
            name: root._icon
            size: Theme.size.iconSm
            color: root._color
            anchors.verticalCenter: parent.verticalCenter
            visible: root._hasIcon
        }

        Text {
            text: root.text
            color: root._color
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightMedium
            wrapMode: Text.WordWrap
            // BUG FIX (Issue 27): give the Text a real wrapping width.
            // Total width minus icon width minus spacing. Falls back to
            // the parent width when there's no icon.
            width: root.width
                   - (root._hasIcon ? _iconItem.width + Theme.space.xs : 0)
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
