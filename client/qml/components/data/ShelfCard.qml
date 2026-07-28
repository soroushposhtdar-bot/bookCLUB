// =============================================================================
//  ShelfCard.qml  (v15k — fully rewritten, clean three-dot menu)
// =============================================================================
//  Grid-view shelf card. Used by ShelvesPage in "grid" view mode.
//
//  The three-dot menu uses a plain Menu with direct MenuItem entries.
//  No Instantiator, no JavaScript closures, no ContextMenu wrapper.
//  The menu is parented to Overlay.overlay so it renders above the
//  Flickable's clip region.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../../theme"
import ".."
import "../buttons"
import "../effects"

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

        layer.enabled: _hoverHandler.hovered
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }

        Rectangle {
            visible: root.selected
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 3
            color: Theme.color.accent
        }
    }

    HoverHandler { id: _hoverHandler; cursorShape: Qt.ArrowCursor }

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
                width: 44; height: 44; radius: Theme.radius.md
                color: Qt.rgba(_shelfColor.r, _shelfColor.g, _shelfColor.b, 0.16)
                anchors.verticalCenter: parent.verticalCenter
                AppIcon { anchors.centerIn: parent; name: "folder"; size: 22; color: _shelfColor }
            }

            Item { width: 1; height: 1; Layout.fillWidth: true }

            // Favorite star
            IconButton {
                iconName: root.shelf && root.shelf.isFavorite ? "star" : "star_outline"
                iconColor: root.shelf && root.shelf.isFavorite ? Theme.color.warning : Theme.color.textMuted
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

            // Three-dot menu
            IconButton {
                iconName: "more_vert"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                onClicked: _menu.popup()
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
            width: parent.width
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        // Book count chip
        Row {
            width: parent.width
            spacing: Theme.space.sm
            Rectangle {
                width: _chipText.implicitWidth + 16; height: 22; radius: Theme.radius.pill
                color: Theme.color.fieldFilled
                anchors.verticalCenter: parent.verticalCenter
                Row {
                    anchors.centerIn: parent; spacing: 4
                    AppIcon { name: "menu_book"; size: 12; color: Theme.color.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                    Text { id: _chipText; text: root.shelf ? root.shelf.bookCount : 0; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightSemibold; anchors.verticalCenter: parent.verticalCenter }
                }
            }
            Item { width: 1; height: 1; Layout.fillWidth: true }
        }
    }

    // Whole-card click area
    MouseArea {
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                _menu.x = mouse.x
                _menu.y = mouse.y
                _menu.popup()
            } else {
                root.clicked()
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Three-dot menu — v15k: fully rewritten.
    //  Uses Menu.popup() with no x/y (opens at cursor position), which is
    //  the simplest and most reliable way to show a menu in Qt 6.
    //  Each MenuItem has a direct onTriggered handler — no closures.
    // -------------------------------------------------------------------------
    Menu {
        id: _menu

        MenuItem { text: "Rename"; onTriggered: root.renameRequested() }
        MenuItem { text: "Duplicate"; onTriggered: root.duplicateRequested() }
        MenuItem { text: "Set color"; onTriggered: root.setColorRequested() }
        MenuSeparator {}
        MenuItem { text: "Toggle favorite"; onTriggered: root.toggleFavoriteRequested() }
        MenuItem { text: "Toggle private"; onTriggered: root.togglePrivateRequested() }
        MenuSeparator {}
        MenuItem { text: "Move up"; onTriggered: root.moveUpRequested() }
        MenuItem { text: "Move down"; onTriggered: root.moveDownRequested() }
        MenuSeparator {}
        MenuItem { text: "Delete"; onTriggered: root.deleteRequested() }
    }

    // -------------------------------------------------------------------------
    //  Internal
    // -------------------------------------------------------------------------
    readonly property color _shelfColor: root.shelf && root.shelf.color
                                         ? root.shelf.color
                                         : Theme.color.accent
}
