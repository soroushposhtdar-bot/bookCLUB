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
import ".."
import "../buttons"
import "../effects"
import "."

Item {
    id: root

    property var book: null
    property bool showAddButton: true
    // v15c: when true (book is purchased), the wishlist heart and the
    // add-to-cart button are NOT rendered at all — the user already owns
    // the book, so neither action makes sense.
    readonly property bool _isPurchased: root.book && root.book.purchased === true

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
            // Issue 5 — clip the hover-scale so the cover never bleeds
            // outside its slot (which previously caused the right side
            // of the cover to render square while the left was rounded
            // because the scaled-up rectangle overflowed the card).
            clip: true

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
            // v15c: hidden entirely when the book is already purchased.
            Item {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 2
                anchors.rightMargin: 2
                width: 36
                height: 36
                visible: !root._isPurchased

                Rectangle {
                    anchors.centerIn: parent
                    width: 30
                    height: 30
                    radius: 15
                    color: Qt.rgba(0/255, 0/255, 0/255, 0.30)
                    visible: _hoverHandler.hovered || (root.book && root.book.inWishlist)
                }

                AppIcon {
                    id: _heartIcon
                    anchors.centerIn: parent
                    name: root.book && root.book.inWishlist ? "favorite" : "favorite_border"
                    size: 18
                    color: root.book && root.book.inWishlist
                           ? Theme.color.error
                           : Theme.color.textInverse
                    visible: _hoverHandler.hovered || (root.book && root.book.inWishlist)
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }

                    // Issue 10 — subtle pop animation when the favorite
                    // state toggles. The scale springs from 0.6 → 1.0 in
                    // ~220 ms with an OutBack easing for a tactile feel.
                    scale: root.book && root.book.inWishlist ? 1.0 : 0.9
                    Behavior on scale {
                        NumberAnimation {
                            duration: Theme.motion.durationBase
                            easing.type: Easing.OutBack
                        }
                    }
                }

                // Ripple burst on toggle (purely cosmetic)
                Rectangle {
                    id: _heartBurst
                    anchors.centerIn: parent
                    width: 30; height: 30; radius: 15
                    color: "transparent"
                    border.color: Theme.color.error
                    border.width: 0
                    opacity: 0
                    scale: 1.0

                    SequentialAnimation {
                        id: _burstAnim
                        running: false
                        ParallelAnimation {
                            NumberAnimation { target: _heartBurst; property: "scale"; from: 1.0; to: 2.0; duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
                            NumberAnimation { target: _heartBurst; property: "opacity"; from: 0.6; to: 0.0; duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
                            NumberAnimation { target: _heartBurst; property: "border.width"; from: 2; to: 0; duration: Theme.motion.durationBase }
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // Issue 10 — trigger the burst animation only when
                        // adding to wishlist (not when removing).
                        if (!(root.book && root.book.inWishlist)) {
                            _burstAnim.restart()
                        }
                        root.wishlistClicked(root.book)
                    }
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
            font.pixelSize: Theme.font.sizeCaption
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
            font.pixelSize: Theme.font.sizeMicro2
            font.weight: Theme.font.weightRegular
            width: parent.width
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        // ----- Rating -----
        RatingStars {
            rating: root.book ? root.book.averageRating : 0
            count: root.book ? root.book.ratingCount : 0
            showNumber: true
            size: 12
        }

        // ----- Price + Add-to-cart -----
        // v15d: rewritten with RowLayout for proper sizing. The old Row
        // used a hardcoded `width: parent.width - 32 - xs` on the price
        // Column which let the cart icon overflow the card on narrow
        // cards. RowLayout + Layout.fillWidth on the price block + a
        // fixed-width button slot keeps everything inside the frame.
        RowLayout {
            width: parent.width
            spacing: Theme.space.xs

            // Price block (fills available width)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0

                // Discounted price row (original struck + discounted)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: root.book && root.book.hasDiscount

                    Text {
                        text: root.book ? root.book.basePriceText : ""
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeMicro2
                        font.strikeout: true
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Text {
                        text: root.book ? root.book.priceText : ""
                        color: Theme.color.primary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightBold
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Item { Layout.fillWidth: true }
                }

                // Single price (no discount)
                Text {
                    Layout.fillWidth: true
                    visible: !(root.book && root.book.hasDiscount)
                    text: root.book ? root.book.priceText : ""
                    color: Theme.color.primary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightBold
                }
            }

            // v15c: "Owned" pill — shown instead of the cart button when
            // the user already owns the book.
            Rectangle {
                visible: root._isPurchased
                Layout.preferredWidth: _purchasedText.implicitWidth + 20
                Layout.preferredHeight: 26
                Layout.alignment: Qt.AlignVCenter
                radius: 13
                color: Theme.color.successSoft
                border.width: 1
                border.color: Theme.color.success

                Text {
                    id: _purchasedText
                    anchors.centerIn: parent
                    text: "Owned"
                    color: Theme.color.success
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightBold
                }
            }

            // Add-to-cart floating button (hidden when purchased)
            // v15d: fixed Layout.preferredWidth so it never overflows.
            Item {
                id: _addBtn
                Layout.preferredWidth: (root.showAddButton && !root._isPurchased) ? 32 : 0
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignVCenter
                visible: root.showAddButton && !root._isPurchased

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
                        size: 16
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
