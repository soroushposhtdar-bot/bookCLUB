// =============================================================================
//  StatCard.qml
// =============================================================================
//  Compact stat display: icon, value, label, optional delta.
//
//  Public API:
//      iconName : string
//      value    : string  — formatted value ("2,310", "$1,420")
//      label    : string  — what the value means
//      delta    : string  — optional ("+12% vs last week")
//      deltaUp  : bool    — green if up, red if down
//      accent   : color   — icon-circle background tint
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../surfaces"

Card {
    id: root
    elevation: "none"
    bordered: true
    padding: Theme.space.lg

    property string iconName: "trending_up"
    property string value: "0"
    property string label: ""
    property string delta: ""
    property bool deltaUp: true
    property color accent: Theme.color.accent

    Row {
        anchors.fill: parent
        spacing: Theme.space.md

        // Icon circle
        Rectangle {
            width: 44; height: 44; radius: 12
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.14)
            anchors.verticalCenter: parent.verticalCenter

            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: 22
                color: root.accent
            }
        }

        Column {
            width: parent.width - 44 - Theme.space.md
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.value
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightBold
            }
            Text {
                text: root.label
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
            }
            Text {
                visible: root.delta.length > 0
                text: (root.deltaUp ? "▲ " : "▼ ") + root.delta
                color: root.deltaUp ? Theme.color.success : Theme.color.error
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightMedium
            }
        }
    }
}
