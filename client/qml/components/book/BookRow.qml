// =============================================================================
//  BookRow.qml
// =============================================================================
//  Horizontal list-row variant of BookCard — used in list views (Library,
//  Wishlist, Search results when grid is off). Compact one-line layout:
//  mini cover | title + author + rating | price + actions.
//
//  Public API:
//      book          : BookDto*
//      showActions   : bool  — show add-to-cart + wishlist buttons
//      selected      : bool  — highlight as selected (bulk-select mode)
//
//  Signals:
//      clicked(var book)
//      addToCartClicked(var book)
//      wishlistClicked(var book)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"
import "../book"
import "../effects"

Item {
    id: root

    property var book: null
    property bool showActions: true
    property bool selected: false

    signal clicked(var book)
    signal addToCartClicked(var book)
    signal wishlistClicked(var book)

    implicitWidth: parent ? parent.width : 600
    implicitHeight: 96

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: Theme.radius.md
        color: root.selected ? Theme.color.accentSoft
             : _hoverHandler.hovered ? Theme.color.fieldFilled
             : "transparent"
        border.color: root.selected ? Theme.color.accent
                    : _hoverHandler.hovered ? Theme.color.borderStrong
                    : Theme.color.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
    }

    Row {
        anchors.fill: parent
        anchors.margins: Theme.space.md
        spacing: Theme.space.md

        // Mini cover
        Item {
            width: 56
            height: 80
            anchors.verticalCenter: parent.verticalCenter
            BookCover {
                anchors.fill: parent
                book: root.book
                cornerRadius: Theme.radius.sm
            }
        }

        // Info column
        Column {
            width: parent.width - 56 - Theme.space.md - _priceCol.width - Theme.space.md - _actions.width - Theme.space.md
            spacing: 2
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.book ? root.book.title : ""
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightSemibold
                elide: Text.ElideRight
                width: parent.width
            }
            Text {
                text: root.book ? root.book.authorName : ""
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                elide: Text.ElideRight
                width: parent.width
            }
            RatingStars {
                rating: root.book ? root.book.averageRating : 0
                count: root.book ? root.book.ratingCount : 0
                showNumber: true
                size: 12
            }
        }

        // Price column
        Column {
            id: _priceCol
            width: 90
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Row {
                spacing: 4
                anchors.horizontalCenter: parent.horizontalCenter
                visible: root.book && root.book.hasDiscount
                Text {
                    text: root.book ? root.book.basePriceText : ""
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.strikeout: true
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.book ? root.book.priceText : ""
                    color: Theme.color.primary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBodyLarge
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Text {
                visible: !(root.book && root.book.hasDiscount)
                text: root.book ? root.book.priceText : ""
                color: Theme.color.primary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightBold
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        // Actions
        Row {
            id: _actions
            spacing: 0
            visible: root.showActions
            anchors.verticalCenter: parent.verticalCenter

            IconButton {
                iconName: root.book && root.book.inWishlist ? "favorite" : "favorite_border"
                iconColor: root.book && root.book.inWishlist ? Theme.color.error : Theme.color.textSecondary
                hoverIconColor: Theme.color.error
                onClicked: root.wishlistClicked(root.book)
            }
            IconButton {
                iconName: "add_shopping_cart"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.primary
                onClicked: root.addToCartClicked(root.book)
            }
        }
    }

    // Whole-row click (only fires when not clicking an action button)
    MouseArea {
        anchors.fill: parent
        z: -1
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked(root.book)
    }

    HoverHandler { id: _hoverHandler; enabled: true }
}
