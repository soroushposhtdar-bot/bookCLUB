// =============================================================================
//  ReviewReactionButton.qml
// =============================================================================
//  Pill-shaped reaction chip used by ReviewItem: thumb_up / thumb_down / reply.
//  Renders an icon + optional label + optional count chip. Highlights when
//  `active` is true (uses `accentColor` + `softColor` pair).
//
//  Public API:
//      iconName     : string
//      label        : string   (empty = no text, icon + count only)
//      count        : int      (-1 = hide count chip)
//      active       : bool     (highlighted state — e.g. currentUserHelpful)
//      accentColor  : color
//      softColor    : color    (background tint when active)
//
//  Signals:
//      clicked()
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Item {
    id: root

    property string iconName: ""
    property string label: ""
    property int count: -1           // -1 hides the count chip
    property bool active: false
    property color accentColor: Theme.color.accent
    property color softColor: Theme.color.accentSoft

    signal clicked()

    implicitWidth: _row.implicitWidth + 2 * Theme.space.sm
    implicitHeight: 32

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.pill
        color: root.active ? root.softColor
             : _ma.containsMouse ? Theme.color.fieldFilled
             : "transparent"
        border.color: root.active ? root.accentColor
                    : _ma.containsMouse ? Theme.color.borderStrong
                    : Theme.color.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
    }

    Row {
        id: _row
        anchors.centerIn: parent
        spacing: 6

        AppIcon {
            name: root.iconName
            size: 16
            color: root.active ? root.accentColor
                 : _ma.containsMouse ? Theme.color.textPrimary
                 : Theme.color.textSecondary
            visible: name.length > 0
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        Text {
            text: root.label
            color: root.active ? root.accentColor
                 : _ma.containsMouse ? Theme.color.textPrimary
                 : Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: root.active ? Theme.font.weightSemibold : Theme.font.weightMedium
            visible: root.label.length > 0
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        }

        // Count chip (Helpful: 12, Not-helpful: 3, …)
        Rectangle {
            visible: root.count >= 0
            width: _countText.implicitWidth + 10
            height: 18
            radius: 4
            color: root.active
                   ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.18)
                   : Theme.color.fieldFilled
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: _countText
                anchors.centerIn: parent
                text: root.count > 0 ? root.count.toString() : "0"
                color: root.active ? root.accentColor : Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightSemibold
            }
        }
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
