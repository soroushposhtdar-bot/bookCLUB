// =============================================================================
//  HomePage.qml
// =============================================================================
//  Premium user dashboard — 12 horizontally-scrollable sections, skeleton
//  loading, hover-animated cards, lazy wave-2 loading.
//
//  Section list (in render order):
//      1.  Hero banner (greeting + decorative book spines)
//      2.  Continue Reading (only if any progress exists)
//      3.  Recommended for you (favorite genres)
//      4.  Because you read …
//      5.  New releases
//      6.  Bestsellers
//      7.  Trending now
//      8.  Editor's picks
//      9.  Discounted books
//      10. Free books
//      11. New arrivals
//      12. Recently viewed
//      13. Browse by genre (chip grid)
//      14. Featured publishers (chip grid)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../layouts"
import "../components/surfaces"
import "../components/buttons"
import "../components/book"
import "../components/data"
import "../components/feedback"
import "../components/progress"
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // HomeViewModel

    signal bookDetailRequested(string bookId)
    signal seeAllRequested(string section)
    signal searchWithGenreRequested(string genre)
    signal searchWithPublisherRequested(string publisher)
    signal openReaderRequested(string bookId)
    signal openCartRequested()
    signal openWishlistRequested()
    signal toastRequested(string variant, string title, string description)
    // Issue 10 — emitted when the user clicks the heart on any BookCard.
    // The UserShell wires this to BookService.toggleWishlist() so the
    // favorite state is synchronized with the backend immediately.
    signal wishlistToggleRequested(string bookId)
    signal addToCartRequested(string bookId)

    readonly property int _horizontalPadding: Theme.space.xxxl

    Rectangle { anchors.fill: parent; color: Theme.color.pageBackground }

    Flickable {
        flickDeceleration: Theme.motion.flickDeceleration
        maximumFlickVelocity: Theme.motion.maximumFlickVelocity
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight + Theme.space.xxxl
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 6
                radius: 3
                color: Theme.color.textMuted
                opacity: parent.pressed ? 0.7 : (parent.hovered ? 0.5 : 0.3)
            }
        }

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.xxl

            // ----- Hero banner -----
            // The primary CTA opens the user's most-recent continue-reading
            // book (if any), falling back to the first bestseller.
            HeroBanner {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                greeting: root.viewModel ? root.viewModel.greeting : ""
                subtext: "Pick up where you left off, or discover something new."
                // Issue 5 — populate the banner with the current in-progress
                // book so it no longer renders empty.
                continueReadingBook: root.viewModel && root.viewModel.continueReading && root.viewModel.continueReading.length > 0
                                     ? root.viewModel.continueReading[0]
                                     : null
                inProgressCount: root.viewModel ? root.viewModel.continueReading.length : 0
                onPrimaryAction: {
                    // Issue 5 — never crash the app. Wrap every property
                    // access in try/catch + length checks so a missing or
                    // malformed DTO doesn't take the whole page down.
                    var bookId = ""
                    try {
                        if (root.viewModel && root.viewModel.continueReading && root.viewModel.continueReading.length > 0) {
                            bookId = root.viewModel.continueReading[0].id || ""
                        } else if (root.viewModel && root.viewModel.bestsellers && root.viewModel.bestsellers.length > 0) {
                            bookId = root.viewModel.bestsellers[0].id || ""
                        }
                    } catch (e) {
                        console.warn("HeroBanner primary action failed:", e)
                        bookId = ""
                    }
                    if (bookId && bookId.length > 0) {
                        root.openReaderRequested(bookId)
                    } else {
                        root.toastRequested("info", "No books yet", "Browse the catalog to find your first read.")
                    }
                }
            }

            // ----- Continue Reading -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Continue reading"
                subtitle: "Pick up where you left off"
                books: root.viewModel ? root.viewModel.continueReading : []
                loading: root.viewModel && root.viewModel.loadingWave1
                showSeeAll: false
                onBookClicked: root.openReaderRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
            }

            // ----- Recommended -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Recommended for you"
                subtitle: "Based on your favorite genres"
                books: root.viewModel ? root.viewModel.recommended : []
                loading: root.viewModel && root.viewModel.loadingWave1
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("recommended")
            }

            // ----- Because you read -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: root.viewModel && root.viewModel.becauseYouRead && root.viewModel.becauseYouRead.length > 0
                       ? "Because you read " + root.viewModel.becauseYouRead[0].title
                       : "Because you read"
                subtitle: "More mysteries worth your time"
                books: root.viewModel ? root.viewModel.becauseYouRead : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("because-you-read")
            }

            // ----- New releases -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "New releases"
                subtitle: "Fresh on the shelves"
                books: root.viewModel ? root.viewModel.newReleases : []
                loading: root.viewModel && root.viewModel.loadingWave1
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("new")
            }

            // ----- Bestsellers -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Bestsellers"
                subtitle: "What everyone's reading right now"
                books: root.viewModel ? root.viewModel.bestsellers : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("bestseller")
            }

            // ----- Trending -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Trending now"
                subtitle: "Heating up this week"
                books: root.viewModel ? root.viewModel.trending : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("trending")
            }

            // ----- Editor's picks -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Editor's picks"
                subtitle: "Curated by our team"
                books: root.viewModel ? root.viewModel.editorsPicks : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("editors-picks")
            }

            // ----- Discounted -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "On sale"
                subtitle: "Limited-time discounts"
                books: root.viewModel ? root.viewModel.discounted : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("discounted")
            }

            // ----- Free books -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Free to read"
                subtitle: "Classics and community favorites"
                books: root.viewModel ? root.viewModel.freeBooks : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("free")
            }

            // ----- New arrivals -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "New arrivals"
                subtitle: "Just landed in the catalog"
                books: root.viewModel ? root.viewModel.newArrivals : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                onSeeAllClicked: root.seeAllRequested("arrivals")
            }

            // ----- Recently viewed -----
            SectionCarousel {
                width: parent.width - 2 * root._horizontalPadding
                anchors.horizontalCenter: parent.horizontalCenter
                title: "Recently viewed"
                subtitle: "Books you've looked at"
                books: root.viewModel ? root.viewModel.recentlyViewed : []
                loading: root.viewModel && root.viewModel.loadingWave2
                onBookClicked: root.bookDetailRequested(book.id)
                onWishlistClicked: root.wishlistToggleRequested(book.id)
                onAddToCartClicked: root.addToCartRequested(book.id)
                showSeeAll: false
            }

            // ----- Browse by genre (chip grid) -----
            Item {
                width: parent.width - 2 * root._horizontalPadding
                height: _genreSection.implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter

                Column {
                    id: _genreSection
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader {
                        width: parent.width
                        title: "Browse by genre"
                        subtitle: "Find your next obsession"
                    }

                    Grid {
                        width: parent.width
                        columns: root.width < 760 ? 2 : (root.width < 1100 ? 4 : 6)
                        spacing: Theme.space.md

                        Repeater {
                            model: root.viewModel ? root.viewModel.popularGenres : []
                            delegate: GenreChip {
                                label: modelData
                                width: (parent.width - Math.max(0, parent.columns - 1) * parent.spacing) / Math.max(1, parent.columns)  // LOW-005: guard against NaN
                                onClicked: root.searchWithGenreRequested(modelData)
                            }
                        }
                    }
                }
            }

            // ----- Featured publishers -----
            Item {
                width: parent.width - 2 * root._horizontalPadding
                height: _pubSection.implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter

                Column {
                    id: _pubSection
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader {
                        width: parent.width
                        title: "Featured publishers"
                        subtitle: "Discover houses worth following"
                    }

                    Grid {
                        width: parent.width
                        columns: root.width < 760 ? 2 : (root.width < 1100 ? 4 : 6)
                        spacing: Theme.space.md

                        Repeater {
                            model: root.viewModel ? root.viewModel.featuredPublishers : []
                            delegate: Item {
                                width: (parent.width - Math.max(0, parent.columns - 1) * parent.spacing) / Math.max(1, parent.columns)  // LOW-005: guard against NaN
                                height: 64

                                Rectangle {
                                    anchors.fill: parent
                                    radius: Theme.radius.lg
                                    color: _pubMa.containsMouse ? Theme.color.fieldFilled : Theme.color.cardBackground
                                    border.color: Theme.color.border
                                    border.width: 1
                                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                                }

                                Row {
                                    anchors.centerIn: parent
                                    spacing: Theme.space.md

                                    Rectangle {
                                        width: 40; height: 40; radius: 10
                                        color: Theme.color.primary
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.charAt(0).toUpperCase()
                                            color: Theme.color.onPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBodyLarge
                                            font.weight: Theme.font.weightBold
                                        }
                                    }

                                    Text {
                                        text: modelData
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                        width: parent.parent.width - 40 - Theme.space.md - 32
                                    }
                                }

                                MouseArea {
                                    id: _pubMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.searchWithPublisherRequested(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (root.viewModel) root.viewModel.refresh()
        // QA-109: keyboard navigation — focus the page so a keyboard user
        // can immediately Tab through the home sections / quick-action
        // buttons. There's no primary text field on HomePage, so we focus
        // the root to make the activeFocusOnTab chain start cleanly from
        // the first interactive child (the first sidebar item, search bar,
        // etc.) rather than from the previously-focused page.
        root.forceActiveFocus()
    }
}
