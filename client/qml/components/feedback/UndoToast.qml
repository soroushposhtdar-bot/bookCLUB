// =============================================================================
//  UndoToast.qml
// =============================================================================
//  Toast variant with an embedded "Undo" button. Shown briefly after a
//  destructive action so the user can revert. Auto-dismisses after `duration`.
//
//  Public API:
//      title       : string
//      description : string
//      undoLabel   : string (default "Undo")
//      duration    : int    (ms; default 5000)
//
//  Signals:
//      undoTriggered()
//      dismissed()
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../buttons"
import "../effects"

Rectangle {
    id: root

    property string title: "Item deleted"
    property string description: ""
    property string undoLabel: "Undo"
    property int duration: 5000

    signal undoTriggered()
    signal dismissed()

    width: 400
    height: _row.implicitHeight + 2 * Theme.space.lg
    radius: Theme.radius.md
    color: Theme.color.primary
    border.color: "transparent"

    layer.enabled: true
    layer.effect: DropShadowBase { colorSpec: Theme.shadow.lg }

    Row {
        id: _row
        anchors.fill: parent
        anchors.margins: Theme.space.lg
        spacing: Theme.space.md

        AppIcon {
            name: "delete"
            size: Theme.size.iconLg
            color: Theme.color.onPrimary
            anchors.verticalCenter: parent.verticalCenter
        }

        Column {
            width: parent.width - Theme.size.iconLg - Theme.space.md - _undoBtn.width - Theme.space.md
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.title
                color: Theme.color.onPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: Theme.font.weightSemibold
                elide: Text.ElideRight
                width: parent.width
            }
            Text {
                text: root.description
                color: Qt.rgba(Theme.color.cardBackground.r, Theme.color.cardBackground.g, Theme.color.cardBackground.b, 0.78)
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                elide: Text.ElideRight
                width: parent.width
                visible: root.description.length > 0
            }
        }

        // Undo button
        Text {
            id: _undoBtn
            text: root.undoLabel
            color: Theme.color.accent
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightBold
            anchors.verticalCenter: parent.verticalCenter

            MouseArea {
                anchors.fill: parent
                anchors.margins: -8
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.undoTriggered()
                    root.dismissed()
                }
            }

            Rectangle {
                anchors.baseline: parent.baseline
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -2
                height: 2
                color: parent.color
                visible: _undoMa.containsMouse
            }
            MouseArea {
                id: _undoMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.undoTriggered()
                    root.dismissed()
                }
            }
        }
    }

    // Auto-dismiss
    Timer {
        id: _timer
        interval: root.duration
        running: root.visible
        repeat: false
        onTriggered: root.dismissed()
    }

    // Entrance / exit
    scale: 0.92
    opacity: 0.0
    Behavior on scale { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutBack } }
    Behavior on opacity { NumberAnimation { duration: Theme.motion.durationFast } }
    Component.onCompleted: { scale = 1.0; opacity = 1.0 }
}
