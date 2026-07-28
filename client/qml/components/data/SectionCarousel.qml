// =============================================================================
//  SectionCarousel.qml
// =============================================================================
//  Wraps a SectionHeader + horizontally-scrolling BookCarousel into a single
//  reusable block. Supports an optional `loading` state that shows a row of
//  skeleton book cards instead of real content.
//
//  Public API:
//      title       : string
//      subtitle    : string
//      books       : list of BookDto*
//      loading     : bool   — show skeleton cards instead
//      skeletonCount : int  — how many skeleton cards to show (default 6)
//      showSeeAll  : bool
//
//  Signals:
//      bookClicked(var book)
//      addToCartClicked(var book)
//      wishlistClicked(var book)
//      seeAllClicked()
// =============================================================================
import QtQuick 2.15
import "../../theme"
import ".."
import "../data"
import "../book"
import "../progress"

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property var books: []
    property bool loading: false
    property int skeletonCount: 6
    property bool showSeeAll: true

    signal bookClicked(var book)
    signal addToCartClicked(var book)
    signal wishlistClicked(var book)
    signal seeAllClicked()

    implicitWidth: parent ? parent.width : 800
    implicitHeight: _col.implicitHeight

    Column {
        id: _col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.space.lg

        SectionHeader {
            width: parent.width
            title: root.title
            subtitle: root.subtitle
            showSeeAll: root.showSeeAll
            onSeeAllClicked: root.seeAllClicked()
        }

        // Loading: skeleton row
        Flickable {
            width: parent.width
            height: 340
            visible: root.loading
            contentWidth: _skelRow.width
            contentHeight: height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickDeceleration: Theme.motion.flickDeceleration
        maximumFlickVelocity: Theme.motion.maximumFlickVelocity

            Row {
                id: _skelRow
                spacing: Theme.space.lg
                Repeater {
                    model: root.skeletonCount
                    delegate: Item {
                        width: Theme.size.bookCardWidth
                        height: 340
                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.sm
                            SkeletonLoader {
                                width: parent.width
                                height: width * Theme.size.bookCoverRatio
                                radius: Theme.radius.md
                            }
                            SkeletonLoader { width: parent.width * 0.85; height: 14; radius: 4 }
                            SkeletonLoader { width: parent.width * 0.55; height: 12; radius: 4 }
                            SkeletonLoader { width: parent.width * 0.40; height: 12; radius: 4 }
                        }
                    }
                }
            }
        }

        // Loaded: real carousel
        BookCarousel {
            width: parent.width
            books: root.books
            visible: !root.loading && root.books.length > 0
            onBookClicked: root.bookClicked(book)
            onAddToCartClicked: root.addToCartClicked(book)
            onWishlistClicked: root.wishlistClicked(book)
        }

        // Empty state (no books + not loading) — Issue 5.
        // Previously this section collapsed to zero height, leaving a
        // confusing gap on the Home page. Now we render a subtle inline
        // hint so the user knows the section exists but has no content yet.
        Item {
            visible: !root.loading && root.books.length === 0
            width: parent.width
            height: visible ? _emptyRow.height + Theme.space.md : 0

            Row {
                id: _emptyRow
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.space.sm
                opacity: 0.65

                AppIcon {
                    name: "auto_stories"
                    size: Theme.size.iconMd
                    color: Theme.color.textMuted
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "Nothing here yet — your reading activity will show up in this section."
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeSmall
                    font.weight: Theme.font.weightRegular
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.WordWrap
                    width: root.width - Theme.size.iconMd - Theme.space.sm
                }
            }
        }
    }
}
