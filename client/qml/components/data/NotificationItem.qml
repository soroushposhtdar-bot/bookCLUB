// =============================================================================
//  NotificationItem.qml
// =============================================================================
//  Single row in the Notifications Center list.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property var notification: null   // NotificationDto*

    signal clicked()
    signal markReadRequested()

    implicitWidth: parent ? parent.width : 480
    implicitHeight: 76

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: Theme.radius.md
        color: root.notification && !root.notification.read
               ? Theme.color.accentSoft
               : "transparent"
        border.color: Theme.color.divider
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motion.durationBase } }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true
            onClicked: root.clicked()
        }
    }

    Row {
        anchors.fill: parent
        anchors.margins: Theme.space.md
        spacing: Theme.space.md

        // Accent icon
        Item {
            width: 44
            height: 44
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: root.notification ? root.notification.accentColor : Theme.color.accent
                opacity: 0.16
            }

            AppIcon {
                anchors.centerIn: parent
                name: root.notification ? root.notification.iconName : "notifications"
                size: 22
                color: root.notification ? root.notification.accentColor : Theme.color.accent
            }
        }

        // Body
        Column {
            width: parent.width - 44 - Theme.space.md - (root.notification && !root.notification.read ? 12 : 0)
            spacing: 2
            anchors.verticalCenter: parent.verticalCenter

            Row {
                width: parent.width
                spacing: Theme.space.sm

                Text {
                    text: root.notification ? root.notification.title : ""
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightSemibold
                    elide: Text.ElideRight
                    width: parent.width - _time.implicitWidth - Theme.space.sm
                }

                Text {
                    id: _time
                    text: root.notification ? root.notification.relativeTime : ""
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                text: root.notification ? root.notification.body : ""
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightRegular
                wrapMode: Text.WordWrap
                width: parent.width
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        // Unread dot
        Item {
            width: 12
            height: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: root.notification && !root.notification.read

            Rectangle {
                anchors.centerIn: parent
                width: 8
                height: 8
                radius: 4
                color: Theme.color.accent
            }
        }
    }
}
