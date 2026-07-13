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
//  Default child slot: any Item anchored to fill (use the `content` alias).
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../effects"

Rectangle {
    id: root

    property string elevation: "md"
    property int radius: Theme.radius.xl
    property int padding: Theme.space.xxl
    property color backgroundColor: Theme.color.cardBackground
    property bool bordered: false

    radius: root.radius
    color: root.backgroundColor
    border.color: root.bordered ? Theme.color.border : "transparent"
    border.width: root.bordered ? 1 : 0

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

    Item {
        id: _content
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
