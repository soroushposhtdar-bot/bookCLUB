// =============================================================================
//  StickyPanel.qml
// =============================================================================
//  Right-side sticky action panel used on the Book Details page. Stays in
//  view while the user scrolls the long description / reviews column.
//
//  Public API:
//      title        : string  — section heading
//      default property alias content : _content.data
//                                        — children placed inside the panel
//                                          are appended BELOW the title.
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

    // v15 fix: expose the Column as the default content slot so children
    // declared inside StickyPanel { ... } go INTO the Column (below the
    // title), not alongside it. Previously, children were reparented to
    // Card's _content Item (via Card's default alias), which placed them
    // at (0,0) — overlapping the title Text. This is what caused the
    // price to render on top of the "Buy this book" heading.
    default property alias content: _content.data

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
