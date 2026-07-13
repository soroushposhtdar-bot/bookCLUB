// =============================================================================
//  ShelfCard.qml
// =============================================================================
//  Grid-view shelf card. Used by ShelvesPage in the "grid" view mode.
//
//  Layout:
//      • Colored folder icon (uses shelf.color)
//      • Favorite star (filled if isFavorite) — top-right
//      • Private lock icon (if isPrivate) — next to favorite
//      • Shelf name (bold)
//      • Description or "N book(s)" (muted)
//      • Book count chip at the bottom
//  Hover: lifts the card and adds a soft shadow.
//  Click: emits clicked(). Right-click: opens the context menu.
//
//  Public API:
//      shelf    : ShelfDto* (id, name, description, color, isPrivate,
//                             isFavorite, bookCount, bookIds)
//      selected : bool — accent left border + tinted background
//
//  Signals:
//      clicked()
//      renameRequested()
//      duplicateRequested()
//      setColorRequested()
//      toggleFavoriteRequested()
//      togglePrivateRequested()
//      moveUpRequested()
//      moveDownRequested()
//      deleteRequested()
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

    implicitWidth: parent ? parent.width : 240
    implicitHeight: _bg.height

    // -------------------------------------------------------------------------
    //  Card surface
    // -------------------------------------------------------------------------
    Rectangle {
        id: _bg
        anchors.left: parent.left
        anchors.right: parent.right
        height: _contentCol.implicitHeight + 2 * Theme.space.lg
        radius: Theme.radius.lg
        color: root.selected ? Theme.color.accentSoft : Theme.color.cardBackground
        border.color: root.selected ? Theme.color.accent
                    : _hoverHandler.hovered ? Theme.color.borderStrong
                    : Theme.color.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

        // Hover lift shadow.
        layer.enabled: _hoverHandler.hovered
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }

        // Selected accent stripe down the left edge.
        Rectangle {
            visible: root.selected
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            color: Theme.color.accent
        }
    }

    HoverHandler {
        id: _hoverHandler
        cursorShape: Qt.ArrowCursor
    }

    // Lift animation
    transform: Scale {
        origin.x: root.width / 2
        origin.y: root.height / 2
        xScale: _hoverHandler.hovered ? 1.015 : 1.0
        yScale: _hoverHandler.hovered ? 1.015 : 1.0
        Behavior on xScale { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
    }

    // -------------------------------------------------------------------------
    //  Content
    // -------------------------------------------------------------------------
    Column {
        id: _contentCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.space.lg
        spacing: Theme.space.md

        // Header row: icon + actions
        Row {
            width: parent.width
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

            // Filler
            Item { width: 1; height: 1; Layout.fillWidth: true }

            // Favorite star
            IconButton {
                iconName: root.shelf && root.shelf.isFavorite ? "star" : "star_outline"
                iconColor: root.shelf && root.shelf.isFavorite
                           ? Theme.color.warning
                           : Theme.color.textMuted
                hoverIconColor: Theme.color.warning
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.toggleFavoriteRequested()
            }

            // Private lock
            IconButton {
                iconName: "lock"
                iconColor: Theme.color.textMuted
                hoverIconColor: Theme.color.textPrimary
                visible: root.shelf && root.shelf.isPrivate
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.togglePrivateRequested()
            }

            // Overflow
            IconButton {
                iconName: "more_vert"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                onClicked: _ctxMenu.openAt(0, height)
            }
        }

        // Shelf name
        Text {
            text: root.shelf ? root.shelf.name : ""
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBodyLarge
            font.weight: Theme.font.weightSemibold
            width: parent.width
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        // Description / book count
        Text {
            text: root.shelf && root.shelf.description && root.shelf.description.length > 0
                  ? root.shelf.description
                  : "%1 book%2".arg(root.shelf ? root.shelf.bookCount : 0)
                                .arg((root.shelf && root.shelf.bookCount === 1) ? "" : "s")
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightRegular
            width: parent.width
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        // Spacer
        Item { width: 1; height: 4; Layout.fillWidth: true }

        // Book count chip
        Row {
            width: parent.width
            spacing: Theme.space.sm

            Rectangle {
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

            Item { width: 1; height: 1; Layout.fillWidth: true }
        }
    }

    // Whole-card click area (sits behind interactive elements)
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
    //  Context menu
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
