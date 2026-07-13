// =============================================================================
//  ErrorState.qml
// =============================================================================
//  Error placeholder with retry CTA. Shown when a page fails to load data.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property string title: "Something went wrong"
    property string description: "We couldn't load this content. Please try again."
    property string retryLabel: "Try again"

    signal retry()

    implicitWidth: parent ? parent.width : 480
    implicitHeight: 320

    Column {
        anchors.centerIn: parent
        spacing: Theme.space.lg

        Item {
            width: 80
            height: 80
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: Theme.color.errorSoft
            }
            AppIcon {
                anchors.centerIn: parent
                name: "error_outline"
                size: 40
                color: Theme.color.error
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
            text: root.description
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            width: Math.min(380, root.width - 2 * Theme.space.xxl)
            anchors.horizontalCenter: parent.horizontalCenter
        }

        PrimaryButton {
            text: root.retryLabel
            iconName: "refresh"
            iconPosition: "leading"
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: root.retry()
        }
    }
}
