// =============================================================================
//  ViewToggle.qml
// =============================================================================
//  Grid / list view mode switcher. Two icon buttons; the active one is
//  highlighted with the primary color.
//
//  Public API:
//      mode : string — "grid" | "list"
//
//  Signals:
//      modeChanged(string mode)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../effects"

Item {
    id: root
    width: 76
    height: 36

    property string mode: "grid"

    signal modeChanged(string mode)

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.md
        color: Theme.color.fieldFilled
        border.color: Theme.color.border
        border.width: 1
    }

    Row {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 0

        Rectangle {
            width: (parent.width - 6) / 2
            height: parent.height
            radius: Theme.radius.sm
            color: root.mode === "grid" ? Theme.color.cardBackground : "transparent"
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            layer.enabled: root.mode === "grid"
            layer.effect: DropShadowBase { colorSpec: Theme.shadow.sm }

            AppIcon {
                anchors.centerIn: parent
                name: "grid_view"
                size: 18
                color: root.mode === "grid" ? Theme.color.textPrimary : Theme.color.textSecondary
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: { root.mode = "grid"; root.modeChanged("grid") }
            }
        }

        Rectangle {
            width: (parent.width - 6) / 2
            height: parent.height
            radius: Theme.radius.sm
            color: root.mode === "list" ? Theme.color.cardBackground : "transparent"
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
            layer.enabled: root.mode === "list"
            layer.effect: DropShadowBase { colorSpec: Theme.shadow.sm }

            AppIcon {
                anchors.centerIn: parent
                name: "view_list"
                size: 18
                color: root.mode === "list" ? Theme.color.textPrimary : Theme.color.textSecondary
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: { root.mode = "list"; root.modeChanged("list") }
            }
        }
    }
}
