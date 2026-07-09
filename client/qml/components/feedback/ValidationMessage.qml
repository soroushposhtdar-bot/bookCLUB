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
import "../"

Item {
    id: root

    property string type: "error"
    property string text: ""
    property string icon: ""

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

    Row {
        id: _row
        spacing: Theme.space.xs

        AppIcon {
            name: root._icon
            size: Theme.size.iconSm
            color: root._color
            anchors.verticalCenter: parent.verticalCenter
            visible: root._icon.length > 0
        }

        Text {
            text: root.text
            color: root._color
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightMedium
            wrapMode: Text.WordWrap
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
