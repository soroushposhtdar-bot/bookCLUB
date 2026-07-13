// =============================================================================
//  BookCard.qml
// =============================================================================
//  Compact book card used in horizontal carousels and grid layouts.
//
//  Public API:
//      book         : BookDto* — must expose title/authorName/price/cover etc.
//      showAddButton: bool (render a quick "add to cart" floating button)
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
import "../effects"
import "./"

Item {
    id: root

    property var book: null
    property bool showAddButton: true

    signal clicked(var book)
    signal addToCartClicked(var book)
    signal wishlistClicked(var book)

    implicitWidth: Theme.size.bookCardWidth
    implicitHeight: _col.implicitHeight

    Column {
        id: _col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space.sm

        // ----- Cover -----
        Item {
            id: _coverWrap
            width: parent.width
            height: width * Theme.size.bookCoverRatio

            BookCover {
                id: _cover
                anchors.fill: parent
                book: root.book
                cornerRadius: Theme.radius.md
            }

            scale: _hoverHandler.hovered ? 1.025 : 1.0
            Behavior on scale { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }

            layer.enabled: _hoverHandler.hovered
            layer.effect: DropShadowBase { colorSpec: Theme.shadow.md }

            // Discount badge
            Rectangle {
                visible: root.book && root.book.hasDiscount
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: 8
                anchors.leftMargin: 8
                width: _discText.implicitWidth + 16
                height: 22
                radius: Theme.radius.pill
                color: Theme.color.error

                Text {
                    id: _discText
                    anchors.centerIn: parent
                    text: root.book ? "-%1%".arg(root.book.discountPercent) : ""
                    color: Theme.color.textInverse
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightBold
                }
            }

            // Free badge
            Rectangle {
                visible: root.book && root.book.isFree
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 8
                anchors.rightMargin: 8
                width: _freeText.implicitWidth + 16
                height: 22
                radius: Theme.radius.pill
                color: Theme.color.success

                Text {
                    id: _freeText
                    anchors.centerIn: parent
                    text: "FREE"
                    color: Theme.color.textInverse
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightBold
                }
            }

            // Wishlist heart button (top-right)
            Item {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 2
                anchors.rightMargin: 2
                width: 36
                height: 36

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: "rgba(0, 0, 0, 0.30)"
                    visible: _hoverHandler.hovered || (root.book && root.book.inWishlist)
                }

                AppIcon {
                    anchors.centerIn: parent
                    name: root.book && root.book.inWishlist ? "favorite" : "favorite_border"
                    size: 18
                    color: root.book && root.book.inWishlist
                           ? Theme.color.error
                           : Theme.color.textInverse
                    visible: _hoverHandler.hovered || (root.book && root.book.inWishlist)
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.wishlistClicked(root.book)
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.clicked(root.book)
                z: -1
            }

            HoverHandler {
                id: _hoverHandler
                enabled: true
            }
        }

        // ----- Title -----
        Text {
            text: root.book ? root.book.title : ""
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightSemibold
            width: parent.width
            elide: Text.ElideRight
            maximumLineCount: 1
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.clicked(root.book)
            }
        }

        // ----- Author -----
        Text {
            text: root.book ? root.book.authorName : ""
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightRegular
            width: parent.width
            elide: Text.ElideRight
        }

        // ----- Rating -----
        RatingStars {
            rating: root.book ? root.book.averageRating : 0
            count: root.book ? root.book.ratingCount : 0
            showNumber: true
            size: 13
        }

        // ----- Price + Add-to-cart -----
        Row {
            width: parent.width
            spacing: Theme.space.sm

            // Price block
            Column {
                width: parent.width - (root.showAddButton ? _addBtn.width + Theme.space.sm : 0)
                anchors.verticalCenter: parent.verticalCenter

                Row {
                    spacing: 4
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
                }
            }

            // Add-to-cart floating button
            Item {
                id: _addBtn
                width: root.showAddButton ? 36 : 0
                height: 36
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: _addMa.pressed ? Theme.color.primaryPressed
                         : _addMa.containsMouse ? Theme.color.primaryHover
                         : Theme.color.primary
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }

                    AppIcon {
                        anchors.centerIn: parent
                        name: "add_shopping_cart"
                        size: 18
                        color: Theme.color.onPrimary
                    }

                    MouseArea {
                        id: _addMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.addToCartClicked(root.book)
                    }

                    scale: _addMa.pressed ? 0.92 : 1.0
                    Behavior on scale { NumberAnimation { duration: Theme.motion.durationInstant; easing.type: Easing.OutCubic } }
                }
            }
        }
    }
}
