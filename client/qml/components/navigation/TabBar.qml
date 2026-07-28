// =============================================================================
//  TabBar.qml
// =============================================================================
//  Horizontal segmented tab control. Used by the Library page (My Books /
//  Saved / Shelves) and any other multi-tab view.
//
//  Public API:
//      tabs       : list of strings (tab labels)
//      activeIndex: int (currently selected tab)
//
//  Signals:
//      tabSelected(int index)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../effects"

Item {
    id: root

    property var tabs: []
    property int activeIndex: 0

    signal tabSelected(int index)

    implicitWidth: 400
    implicitHeight: 44

    Rectangle {
        anchors.fill: parent
        color: Theme.color.fieldFilled
        radius: Theme.radius.lg
    }

    Row {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 0

        Repeater {
            model: root.tabs
            delegate: Item {
                width: root.width / root.tabs.length - 4
                height: root.height - 8
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    id: _tabBg
                    anchors.fill: parent
                    radius: Theme.radius.md
                    color: root.activeIndex === index ? Theme.color.cardBackground : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationBase } }

                    layer.enabled: root.activeIndex === index
                    layer.effect: DropShadowBase { colorSpec: Theme.shadow.sm }
                }

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: root.activeIndex === index ? Theme.color.textPrimary : Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: root.activeIndex === index ? Theme.font.weightSemibold : Theme.font.weightMedium
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tabSelected(index)
                }
            }
        }
    }
}
