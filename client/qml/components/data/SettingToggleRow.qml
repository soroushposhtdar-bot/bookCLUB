// =============================================================================
//  SettingToggleRow.qml
// =============================================================================
//  Single setting row — leading icon, title + description, trailing toggle.
//  Used throughout the Settings page.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../selection"

Item {
    id: root
    implicitWidth: parent ? parent.width : 480
    implicitHeight: 56

    property string iconName: "settings"
    property string title: ""
    property string description: ""
    property bool checked: false
    property bool destructive: false

    signal toggled(bool checked)

    Row {
        anchors.fill: parent
        anchors.leftMargin: 0
        anchors.rightMargin: 0
        spacing: Theme.space.md

        Rectangle {
            width: 40; height: 40; radius: 10
            color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.10)
            anchors.verticalCenter: parent.verticalCenter
            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: 22
                color: root.destructive ? Theme.color.error : Theme.color.accent
            }
        }

        Column {
            width: parent.width - 40 - Theme.space.md - _toggle.width - Theme.space.md
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.title
                color: root.destructive ? Theme.color.error : Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: Theme.font.weightMedium
            }
            Text {
                visible: root.description.length > 0
                text: root.description
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }

        Item { Layout.fillWidth: true; width: 1; height: 1 }

        AppToggleButton {
            id: _toggle
            checked: root.checked
            onToggled: root.toggled(checked)
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
