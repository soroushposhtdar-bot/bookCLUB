// =============================================================================
//  EmptyIllustration.qml
// =============================================================================
//  Richer empty state with a large decorative icon, headline, body, and
//  optional primary + secondary CTAs. Used everywhere a list could be empty.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property string iconName: "auto_stories"
    property string title: "Nothing here yet"
    property string description: ""
    property string primaryActionLabel: ""
    property string secondaryActionLabel: ""

    signal primaryActionTriggered()
    signal secondaryActionTriggered()

    implicitWidth: parent ? parent.width : 480
    implicitHeight: 320

    Column {
        anchors.centerIn: parent
        spacing: Theme.space.lg

        // Large decorative icon
        Item {
            width: 96
            height: 96
            anchors.horizontalCenter: parent.horizontalCenter

            // Soft halo
            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.08)
            }
            Rectangle {
                anchors.centerIn: parent
                width: 72; height: 72
                radius: 36
                color: Theme.color.fieldFilled
                AppIcon {
                    anchors.centerIn: parent
                    name: root.iconName
                    size: 36
                    color: Theme.color.textMuted
                }
            }
        }

        Text {
            text: root.title
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeHeadline
            font.weight: Theme.font.weightSemibold
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            visible: root.description.length > 0
            text: root.description
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            width: Math.min(420, root.width - 2 * Theme.space.xxl)
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: Theme.space.md
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.primaryActionLabel.length > 0 || root.secondaryActionLabel.length > 0

            PrimaryButton {
                text: root.primaryActionLabel
                visible: root.primaryActionLabel.length > 0
                onClicked: root.primaryActionTriggered()
            }
            SecondaryButton {
                text: root.secondaryActionLabel
                visible: root.secondaryActionLabel.length > 0
                onClicked: root.secondaryActionTriggered()
            }
        }
    }
}
