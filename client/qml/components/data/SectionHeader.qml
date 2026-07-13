// =============================================================================
//  SectionHeader.qml
// =============================================================================
//  Section title + optional "See all" link. Used between dashboard sections.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property bool showSeeAll: false

    signal seeAllClicked()

    implicitWidth: 400
    implicitHeight: 40

    Row {
        anchors.fill: parent
        spacing: Theme.space.md

        Column {
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.title
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightBold
            }
            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightRegular
            }
        }

        Item { width: 1; height: 1; Layout.fillWidth: true }

        TextButton {
            text: "See all"
            iconName: ""   // arrow rendered separately below for trailing placement
            visible: root.showSeeAll
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.seeAllClicked()
        }

        AppIcon {
            name: "arrow_forward"
            size: Theme.size.iconSm
            color: Theme.color.accent
            visible: root.showSeeAll
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
