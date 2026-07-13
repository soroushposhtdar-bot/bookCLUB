// =============================================================================
//  ShelfRow.qml
// =============================================================================
//  List-view shelf row. Used by ShelvesPage in the "list" view mode.
//
//  Layout:
//      • Colored folder icon (uses shelf.color)
//      • Name + description / "N book(s)" column
//      • Favorite star (filled if isFavorite)
//      • Private lock icon (if isPrivate)
//      • Book count chip
//      • Overflow + actions
//  Hover: subtle tint. Click: emits clicked().
//  Right-click: opens the context menu (same actions as ShelfCard).
//
//  Public API:
//      shelf    : ShelfDto*
//      selected : bool
//
//  Signals: identical to ShelfCard.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"
import "../effects"
import "../feedback"

Item {
    id: root

    property var shelf: null
    property bool selected: false

    signal clicked()
    signal renameRequested()
    signal duplicateRequested()
    signal setColorRequested()
    signal toggleFavoriteRequested()
    signal togglePrivateRequested()
    signal moveUpRequested()
    signal moveDownRequested()
    signal deleteRequested()

    implicitWidth: parent ? parent.width : 600
    implicitHeight: 88

    // Background
    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.md
        color: root.selected ? Theme.color.accentSoft
             : _hoverHandler.hovered ? Theme.color.fieldFilled
             : "transparent"
        border.color: root.selected ? Theme.color.accent
                    : _hoverHandler.hovered ? Theme.color.borderStrong
                    : Theme.color.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

        layer.enabled: _hoverHandler.hovered
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.sm }
    }

    HoverHandler { id: _hoverHandler; cursorShape: Qt.ArrowCursor }

    Row {
        anchors.fill: parent
        anchors.margins: Theme.space.md
        spacing: Theme.space.md

        // Colored folder icon
        Rectangle {
            width: 44
            height: 44
            radius: Theme.radius.md
            color: Qt.rgba(_shelfColor.r, _shelfColor.g, _shelfColor.b, 0.16)
            anchors.verticalCenter: parent.verticalCenter

            AppIcon {
                anchors.centerIn: parent
                name: "folder"
                size: 22
                color: _shelfColor
            }
        }

        // Name + description column
        Column {
            width: parent.width - 44 - Theme.space.md - _actions.width - Theme.space.md - _countChip.width - Theme.space.md
            spacing: 2
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.shelf ? root.shelf.name : ""
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightSemibold
                elide: Text.ElideRight
                width: parent.width
            }
            Text {
                text: root.shelf && root.shelf.description && root.shelf.description.length > 0
                      ? root.shelf.description
                      : "%1 book%2".arg(root.shelf ? root.shelf.bookCount : 0)
                                    .arg((root.shelf && root.shelf.bookCount === 1) ? "" : "s")
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                elide: Text.ElideRight
                width: parent.width
            }
        }

        // Book count chip
        Rectangle {
            id: _countChip
            width: _chipText.implicitWidth + 16
            height: 22
            radius: Theme.radius.pill
            color: Theme.color.fieldFilled
            anchors.verticalCenter: parent.verticalCenter

            Row {
                anchors.centerIn: parent
                spacing: 4

                AppIcon {
                    name: "menu_book"
                    size: 12
                    color: Theme.color.textSecondary
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    id: _chipText
                    text: root.shelf ? root.shelf.bookCount : 0
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightSemibold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Actions
        Row {
            id: _actions
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            IconButton {
                iconName: root.shelf && root.shelf.isFavorite ? "star" : "star_outline"
                iconColor: root.shelf && root.shelf.isFavorite
                           ? Theme.color.warning
                           : Theme.color.textMuted
                hoverIconColor: Theme.color.warning
                onClicked: root.toggleFavoriteRequested()
            }
            IconButton {
                iconName: "lock"
                iconColor: Theme.color.textMuted
                hoverIconColor: Theme.color.textPrimary
                visible: root.shelf && root.shelf.isPrivate
                onClicked: root.togglePrivateRequested()
            }
            IconButton {
                iconName: "more_vert"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                onClicked: _ctxMenu.openAt(0, height)
            }
        }
    }

    // Whole-row click (sits behind action buttons)
    MouseArea {
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                _ctxMenu.openAt(mouse.x, mouse.y)
            } else {
                root.clicked()
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Context menu (same actions as ShelfCard)
    // -------------------------------------------------------------------------
    ContextMenu {
        id: _ctxMenu
        parent: root
        actions: [
            { text: "Rename",          iconName: "edit",           action: function() { root.renameRequested() } },
            { text: "Duplicate",       iconName: "content_copy",   action: function() { root.duplicateRequested() } },
            { text: "Set color",       iconName: "palette",        action: function() { root.setColorRequested() } },
            { separator: true },
            { text: "Toggle favorite", iconName: "star",           action: function() { root.toggleFavoriteRequested() } },
            { text: "Toggle private",  iconName: "lock",           action: function() { root.togglePrivateRequested() } },
            { separator: true },
            { text: "Move up",         iconName: "arrow_upward",   action: function() { root.moveUpRequested() } },
            { text: "Move down",       iconName: "arrow_downward", action: function() { root.moveDownRequested() } },
            { separator: true },
            { text: "Delete",          iconName: "delete_outline", destructive: true, action: function() { root.deleteRequested() } }
        ]
    }

    // -------------------------------------------------------------------------
    //  Internal: resolve shelf color
    // -------------------------------------------------------------------------
    readonly property color _shelfColor: root.shelf && root.shelf.color
                                         ? root.shelf.color
                                         : Theme.color.accent
}
