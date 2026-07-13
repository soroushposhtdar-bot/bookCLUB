// =============================================================================
//  Breadcrumb.qml
// =============================================================================
//  Navigation trail (Home / Library / My Shelves / Weekend Reads). Clicking
//  any segment emits segmentClicked(index).
//
//  Public API:
//      segments : list of strings
//      current  : int — index of the active (last-clickable) segment; the
//                       final segment is rendered as plain text (no chevron)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"

Row {
    id: root
    spacing: 4

    property var segments: []
    property int current: 0

    signal segmentClicked(int index)

    Repeater {
        model: root.segments
        delegate: Row {
            spacing: 4
            height: 24

            Text {
                text: modelData
                color: index === root.segments.length - 1
                       ? Theme.color.textPrimary
                       : (index === root.current ? Theme.color.accent : Theme.color.textSecondary)
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
                font.weight: index === root.segments.length - 1 ? Theme.font.weightSemibold : Theme.font.weightRegular
                anchors.verticalCenter: parent.verticalCenter

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4
                    cursorShape: index < root.segments.length - 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (index < root.segments.length - 1) root.segmentClicked(index)
                    }
                }
            }

            AppIcon {
                name: "chevron_right"
                size: 18
                color: Theme.color.textMuted
                visible: index < root.segments.length - 1
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
