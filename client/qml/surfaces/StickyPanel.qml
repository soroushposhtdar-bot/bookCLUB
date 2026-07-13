// =============================================================================
//  StickyPanel.qml
// =============================================================================
//  Right-side sticky action panel used on the Book Details page. Stays in
//  view while the user scrolls the long description / reviews column.
//
//  Public API:
//      title        : string  — section heading
//      default property alias content : _content.data
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../surfaces"

Card {
    id: root
    elevation: "sm"
    bordered: true
    padding: Theme.space.xl

    property string title: ""

    Column {
        id: _content
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space.lg

        Text {
            text: root.title
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeTitle
            font.weight: Theme.font.weightBold
            visible: root.title.length > 0
        }
    }
}
