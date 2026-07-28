// =============================================================================
//  Card.qml
// =============================================================================
//  Surface container — rounded white panel with optional soft drop shadow.
//  Foundation for dialogs, hero panels, form panels, success cards.
//
//  Public API:
//      elevation : string — "none" | "sm" | "md" | "lg" | "xl"
//      radius    : int    — corner radius (default Theme.radius.xl)
//      padding   : int    — internal content padding
//      backgroundColor : color
//
//  Default child slot: any Item placed inside the Card is reparented to
//  the _content Item. The Card auto-sizes its implicitHeight based on the
//  content's implicitHeight + 2 * padding. If an explicit height is set
//  on the Card, it takes precedence.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../effects"

Rectangle {
    id: root

    property string elevation: "md"
    property int padding: Theme.space.xxl
    property color backgroundColor: Theme.color.cardBackground
    property bool bordered: false

    // Use Rectangle's built-in radius property directly (default: Theme.radius.xl).
    radius: Theme.radius.xl
    color: root.backgroundColor
    border.color: root.bordered ? Theme.color.border : "transparent"
    border.width: root.bordered ? 1 : 0

    // BUG FIX (empty-profile-page): auto-size the Card's implicitHeight
    // based on the content's actual height + 2 * padding.
    //
    // v5 fix: use `childrenRect.height` instead of `implicitHeight`. The
    // previous binding (`_content.implicitHeight`) was always 0 because
    // `Item.implicitHeight` does NOT auto-compute from children — it
    // defaults to 0 and stays 0 unless something explicitly sets it. This
    // meant every Card without an explicit height rendered at only
    // `2 * padding` pixels tall, causing content to overflow or be clipped
    // across the publisher pages (catalog table, dashboard cards, etc.).
    //
    // `childrenRect.height` is the bounding-box height of all children,
    // which IS computed automatically by QtQuick. This makes Cards
    // auto-size correctly when their content uses `width: parent.width`
    // and lets its height be driven by its own children.
    implicitHeight: _content.childrenRect.height + 2 * root.padding

    layer.enabled: elevation !== "none"
    layer.effect: DropShadowBase {
        colorSpec: {
            switch (root.elevation) {
                case "sm": return Theme.shadow.sm
                case "md": return Theme.shadow.md
                case "lg": return Theme.shadow.lg
                case "xl": return Theme.shadow.xl
                default:   return Theme.shadow.md
            }
        }
    }

    // Default content slot — children of Card are reparented here.
    default property alias content: _content.data

    // v5: _content anchors to all 4 sides of the Card (with padding margins)
    // so that when the Card has an explicit height (e.g. Layout.preferredHeight),
    // _content fills it and children with `anchors.fill: parent` work correctly.
    // When the Card has NO explicit height, Card.implicitHeight (bound above to
    // childrenRect.height + 2*padding) drives the Card's height, and children
    // should use `width: parent.width` (not anchors.fill) to avoid circular deps.
    Item {
        id: _content
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
