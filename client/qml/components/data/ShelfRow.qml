// =============================================================================
//  ShelfRow.qml  (v15k — fully rewritten, clean three-dot menu)
// =============================================================================
//  List-view shelf row. Used by ShelvesPage in "list" view mode.
//  Same clean Menu approach as ShelfCard.
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

    implicitWidth: parent ? parent.width : 600
    implicitHeight: 88

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

        Rectangle {
            width: 44; height: 44; radius: Theme.radius.md
            color: Qt.rgba(_shelfColor.r, _shelfColor.g, _shelfColor.b, 0.16)
            anchors.verticalCenter: parent.verticalCenter
            AppIcon { anchors.centerIn: parent; name: "folder"; size: 22; color: _shelfColor }
        }

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

        Rectangle {
            id: _countChip
            width: _chipText.implicitWidth + 16; height: 22; radius: Theme.radius.pill
            color: Theme.color.fieldFilled
            anchors.verticalCenter: parent.verticalCenter
            Row {
                anchors.centerIn: parent; spacing: 4
                AppIcon { name: "menu_book"; size: 12; color: Theme.color.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                Text { id: _chipText; text: root.shelf ? root.shelf.bookCount : 0; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightSemibold; anchors.verticalCenter: parent.verticalCenter }
            }
        }

        Row {
            id: _actions
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter
            IconButton {
                iconName: root.shelf && root.shelf.isFavorite ? "star" : "star_outline"
                iconColor: root.shelf && root.shelf.isFavorite ? Theme.color.warning : Theme.color.textMuted
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
                onClicked: _menu.popup()
            }
        }
    }

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

    // v15k: clean Menu — same as ShelfCard.
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

    readonly property color _shelfColor: root.shelf && root.shelf.color
                                         ? root.shelf.color
                                         : Theme.color.accent
}
