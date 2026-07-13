// =============================================================================
//  SortDropdown.qml
// =============================================================================
//  Compact sort selector with a leading "Sort" label and a dropdown.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"

Item {
    id: root
    width: 200
    height: 36

    property var options: []   // list of { label, value }
    property string currentValue: ""
    property string currentLabel: options.length > 0 ? options[0].label : ""

    signal changed(string value)

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.md
        color: Theme.color.cardBackground
        border.color: Theme.color.border
        border.width: 1
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 6

        Text {
            text: "Sort:"
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            anchors.verticalCenter: parent.verticalCenter
        }

        ComboBox {
            id: _combo
            width: parent.width - 36 - 6
            height: parent.height
            model: root.options.map(o => o.label)
            background: Item {}
            contentItem: Text {
                text: _combo.displayText
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: Theme.font.weightMedium
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            onActivated: {
                root.currentValue = root.options[index].value
                root.currentLabel = root.options[index].label
                root.changed(root.currentValue)
            }
        }

        AppIcon {
            name: "expand_more"
            size: 18
            color: Theme.color.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
