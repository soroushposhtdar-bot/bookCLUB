// =============================================================================
//  EmptyState.qml
// =============================================================================
//  Friendly empty-state placeholder — icon, title, description, optional CTA.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property string iconName: "auto_stories"
    property string title: "Nothing here yet"
    property string description: ""
    property string actionLabel: ""
    property int iconSize: 64

    signal actionTriggered()

    implicitWidth: 400
    implicitHeight: _col.implicitHeight

    Column {
        id: _col
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.space.lg

        Item {
            width: root.iconSize
            height: root.iconSize
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Theme.color.fieldFilled
            }

            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: root.iconSize * 0.5
                color: Theme.color.textMuted
            }
        }

        Text {
            text: root.title
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeTitle
            font.weight: Theme.font.weightSemibold
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            visible: root.description.length > 0
            text: root.description
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightRegular
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(380, root.width - 2 * Theme.space.xxl)
        }

        SecondaryButton {
            visible: root.actionLabel.length > 0
            text: root.actionLabel
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: root.actionTriggered()
        }
    }
}
