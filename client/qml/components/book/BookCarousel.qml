// =============================================================================
//  BookCarousel.qml
// =============================================================================
//  Horizontal scrolling carousel of BookCards. Used by every Home page
//  section (Recommended, New Releases, Bestsellers, Free).
//
//  Public API:
//      books        : list of BookDto*
//      cardWidth    : int (default Theme.size.bookCardWidth)
//      spacing      : int
//
//  Signals (forwarded from BookCard):
//      bookClicked(var book)
//      addToCartClicked(var book)
//      wishlistClicked(var book)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../book"
import "../buttons"

Flickable {
    id: root

    property var books: []
    property int cardWidth: Theme.size.bookCardWidth
    property int spacing: Theme.space.lg

    signal bookClicked(var book)
    signal addToCartClicked(var book)
    signal wishlistClicked(var book)

    contentWidth: _row.implicitWidth
    contentHeight: height
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickDeceleration: 8000

    implicitHeight: 340

    Row {
        id: _row
        spacing: root.spacing
        leftPadding: 0
        rightPadding: Theme.space.lg

        Repeater {
            model: root.books
            delegate: BookCard {
                width: root.cardWidth
                book: modelData
                onClicked: root.bookClicked(book)
                onAddToCartClicked: root.addToCartClicked(book)
                onWishlistClicked: root.wishlistClicked(book)
            }
        }
    }

    // Right-edge fade hint when more content is scrollable
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 32
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Theme.color.pageBackground }
        }
        visible: root.contentWidth > root.width + 1
        z: 10
    }
}
