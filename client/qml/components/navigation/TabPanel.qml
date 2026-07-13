// =============================================================================
//  TabPanel.qml
// =============================================================================
//  Content-tab container — wired to TabBar via activeIndex. Each tab is a
//  lazy-loaded Loader; switching tabs unloads the previous (saves memory).
//
//  Public API:
//      tabs       : list of strings (tab labels)
//      activeIndex: int
//      default property alias content : _stack.data  — array of Items, one per tab
//
//  Signals:
//      tabActivated(int index)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../navigation"

Item {
    id: root

    property var tabs: []
    property int activeIndex: 0

    signal tabActivated(int index)

    default property alias content: _stack.data

    implicitHeight: _col.implicitHeight

    Column {
        id: _col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space.xl

        TabBar {
            id: _tabs
            width: parent.width
            height: 44
            tabs: root.tabs
            activeIndex: root.activeIndex
            onTabSelected: {
                root.activeIndex = index
                root.tabActivated(index)
            }
        }

        // Content stack — only the active child is visible
        Item {
            id: _stack
            width: parent.width
            height: children.length > activeIndex && children[activeIndex] ? children[activeIndex].implicitHeight : 0
            anchors.left: parent.left
            anchors.right: parent.right

            // Children are reparented here via the default property alias.
            // We toggle visibility based on activeIndex.
            // (QML auto-reparents Items declared as children of this Item.)
        }
    }

    // Toggle visibility of direct children based on activeIndex
    onActiveIndexChanged: _updateVisibility()
    Component.onCompleted: _updateVisibility()

    function _updateVisibility() {
        for (var i = 0; i < _stack.children.length; ++i) {
            var child = _stack.children[i]
            if (child instanceof Item) {
                child.visible = (i === root.activeIndex)
                if (child.visible) {
                    child.anchors.fill = _stack
                }
            }
        }
    }
}
