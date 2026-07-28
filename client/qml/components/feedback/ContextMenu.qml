// =============================================================================
//  ContextMenu.qml
// =============================================================================
//  Right-click context menu. Anchors itself to the parent and opens at the
//  click position. Each action is a { text, iconName, action, destructive }
//  object; separators are { separator: true }.
//
//  Public API:
//      actions : var (list of action objects)
//
//  Signals:
//      actionTriggered(int index)
//
//  Usage from any item:
//      MouseArea {
//          acceptedButtons: Qt.LeftButton | Qt.RightButton
//          onClicked: if (mouse.button === Qt.RightButton) _menu.openAt(mouse.x, mouse.y)
//      }
//      ContextMenu { id: _menu; actions: [...] }
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import ".."
import "../effects"

Menu {
    id: root

    property var actions: []

    // Rebuild menu items when `actions` changes
    Instantiator {
        model: root.actions
        delegate: MenuItem {
            text: modelData.separator ? "" : modelData.text
            enabled: !modelData.separator
            height: modelData.separator ? 9 : 38
            icon.source: modelData.iconName && !modelData.separator ? "image://icon/" + modelData.iconName : ""

            onTriggered: {
                if (modelData.action) modelData.action()
                root.actionTriggered(index)
            }

            contentItem: Item {
                anchors.fill: parent
                visible: !modelData.separator
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    spacing: 10
                    AppIcon {
                        name: modelData.iconName || ""
                        size: 18
                        color: modelData.destructive ? Theme.color.error : Theme.color.textSecondary
                        visible: name.length > 0
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: modelData.text
                        color: modelData.destructive ? Theme.color.error : Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightMedium
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            background: Rectangle {
                color: parent.highlighted ? Theme.color.fieldFilled : "transparent"
                radius: Theme.radius.sm
            }
        }
        onObjectAdded: {
            // v15f: use function syntax instead of arrow syntax for
            // broader Qt version compatibility. Arrow functions in
            // Instantiator signal handlers can silently fail on some
            // Qt 6.x builds, causing menu items to never be inserted.
            root.insertItem(index, object)
        }
        onObjectRemoved: {
            root.removeItem(object)
        }
    }

    background: Rectangle {
        radius: Theme.radius.md
        color: Theme.color.cardBackground
        border.color: Theme.color.border
        border.width: 1
        layer.enabled: true
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.lg }
    }

    function openAt(x, y) {
        // v15f: use popup() instead of open() so the menu is positioned
        // relative to its parent at (x, y). Menu.open() ignores x/y on
        // some Qt 6.x builds and pops up at the screen center instead.
        root.x = x
        root.y = y
        root.popup()
    }

    signal actionTriggered(int index)
}
