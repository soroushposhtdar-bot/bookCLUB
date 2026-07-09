// =============================================================================
//  Toast.qml
// =============================================================================
//  Single toast notification — icon + title + (optional) description + action.
//  Created and managed by ToastManager (see ToastManager.qml).
//
//  Variants (color + default icon):
//      • success  → check_circle, green
//      • error    → error_outline, red
//      • warning  → warning_amber, orange
//      • info     → info_outline, blue
//
//  Public API:
//      variant     : string
//      title       : string
//      description : string
//      actionLabel : string   (optional button label)
//      duration    : int      (ms; 0 = sticky)
//
//  Signals:
//      actionClicked()
//      dismissed()
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../buttons"
import "../effects"

Rectangle {
    id: root

    property string variant: "info"
    property string title: ""
    property string description: ""
    property string actionLabel: ""
    property int duration: 4000

    signal actionClicked()
    signal dismissed()

    width: 360
    height: _column.implicitHeight + 2 * Theme.space.lg
    radius: Theme.radius.md
    color: Theme.color.cardBackground
    border.color: Theme.color.border
    border.width: 1

    layer.enabled: true
    layer.effect: DropShadowBase {
        colorSpec: Theme.shadow.lg
    }

    readonly property color _accentColor: {
        switch (variant) {
            case "success": return Theme.color.success
            case "error":   return Theme.color.error
            case "warning": return Theme.color.warning
            case "info":    return Theme.color.info
            default:        return Theme.color.info
        }
    }

    readonly property string _iconName: {
        switch (variant) {
            case "success": return "check_circle"
            case "error":   return "error_outline"
            case "warning": return "warning_amber"
            case "info":    return "info_outline"
            default:        return "info_outline"
        }
    }

    // ----- Content -----
    Column {
        id: _column
        anchors.fill: parent
        anchors.margins: Theme.space.lg
        spacing: Theme.space.sm

        Row {
            width: parent.width
            spacing: Theme.space.md

            AppIcon {
                name: root._iconName
                                size: Theme.size.iconLg
                                color: root._accentColor
                                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                width: parent.width - Theme.size.iconLg - Theme.space.md - (root.actionLabel.length > 0 ? _actionBtn.width + Theme.space.md : 0)
                spacing: 2
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    text: root.title
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightSemibold
                    elide: Text.ElideRight
                    width: parent.width
                    visible: root.title.length > 0
                }

                Text {
                    text: root.description
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeSmall
                    font.weight: Theme.font.weightRegular
                    wrapMode: Text.WordWrap
                    width: parent.width
                    visible: root.description.length > 0
                }
            }

            // Action button (optional)
            Text {
                id: _actionBtn
                text: root.actionLabel
                color: Theme.color.accent
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: Theme.font.weightSemibold
                anchors.verticalCenter: parent.verticalCenter
                visible: root.actionLabel.length > 0

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.actionClicked()
                }

                Rectangle {
                    anchors.baseline: parent.baseline
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: -1
                    height: 1
                    color: parent.color
                    visible: parent.visible
                }
            }
        }
    }

    // Close button (always available)
    IconButton {
        id: _close
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.space.sm
        anchors.rightMargin: Theme.space.sm
        iconName: "close"
        iconSize: Theme.size.iconSm
        iconColor: Theme.color.textMuted
        onClicked: root.dismissed()
    }

    // Auto-dismiss timer
    Timer {
        id: _timer
        interval: root.duration
        running: root.duration > 0 && root.visible
        repeat: false
        onTriggered: root.dismissed()
    }

    // Entrance / exit animations
    scale: 0.95
    opacity: 0.0

    Behavior on scale  { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutBack } }
    Behavior on opacity { NumberAnimation { duration: Theme.motion.durationFast } }

    Component.onCompleted: {
        scale = 1.0
        opacity = 1.0
    }
}
