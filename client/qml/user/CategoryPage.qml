// =============================================================================
//  CategoryPage.qml
// =============================================================================
//  Dedicated page for a single Home-page section (e.g. "On Sale", "Free to
//  read", "Bestsellers"). Reuses the Search page's grid layout, sort
//  dropdown, and BookCard presentation, but is an INDEPENDENT page — it
//  does not share state with the main SearchPage.
//
//  Public API:
//      categoryKey   : string  — section key ("recommended" | "new" |
//                                 "bestseller" | "trending" | "editors-picks" |
//                                 "discounted" | "free" | "arrivals" |
//                                 "because-you-read" | "recently-viewed")
//      categoryTitle : string  — human-readable title shown in the header
//      categorySubtitle: string
//      books         : list of BookDto*
//
//  Signals:
//      bookDetailRequested(string bookId)
//      backRequested()
//      toastRequested(string variant, string title, string description)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/book"
import "../components/data"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"
import "../components"

Item {
    id: root
    anchors.fill: parent

    property string categoryKey: ""
    property string categoryTitle: ""
    property string categorySubtitle: ""
    property var books: []
    property var viewModel: null  // HomeViewModel (used for re-fetch on pull-to-refresh)

    signal bookDetailRequested(string bookId)
    signal backRequested()
    signal openReaderRequested(string bookId)
    signal wishlistToggleRequested(string bookId)
    signal addToCartRequested(string bookId)
    signal toastRequested(string variant, string title, string description)

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property int _gridColumns: root.width < 760 ? 2
                                       : root.width < 1100 ? 3
                                       : root.width < 1400 ? 4 : 5

    // Internal view state — local to this page, not shared with SearchPage.
    property string _viewMode: "grid"   // "grid" | "list"
    property string _sortMode: "relevance"  // relevance | price_asc | price_desc | rating | newest

    // Filtered + sorted view of `books` — recomputed when books, view mode,
    // or sort mode changes.
    readonly property var _displayBooks: {
        var arr = []
        for (var i = 0; i < root.books.length; i++) arr.push(root.books[i])
        // Sort
        var s = root._sortMode
        arr.sort(function(a, b) {
            if (s === "price_asc")  return a.price - b.price
            if (s === "price_desc") return b.price - a.price
            if (s === "rating")     return b.averageRating - a.averageRating
            if (s === "newest")     return b.createdAt - a.createdAt
            // relevance = rating * count
            return (b.averageRating * b.ratingCount) - (a.averageRating * a.ratingCount)
        })
        return arr
    }

    Rectangle { anchors.fill: parent; color: Theme.color.pageBackground }

    Flickable {
        id: _flickable
        flickDeceleration: Theme.motion.flickDeceleration
        maximumFlickVelocity: Theme.motion.maximumFlickVelocity
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.xl

            Item { width: 1; height: Theme.space.sm }

            // ----- Header -----
            Item {
                width: parent.width
                height: _headerRow.height

                Row {
                    id: _headerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.md

                    IconButton {
                        iconName: "arrow_back"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        onClicked: root.backRequested()
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            text: root.categoryTitle.length > 0 ? root.categoryTitle : "Category"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeHeadline
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: root.categorySubtitle
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightRegular
                            visible: root.categorySubtitle.length > 0
                        }
                    }

                    Item { width: 1; height: 1 }

                    // Result count chip
                    Rectangle {
                        width: _countText.implicitWidth + 16
                        height: 26; radius: 13
                        color: Theme.color.fieldFilled
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: _countText
                            anchors.centerIn: parent
                            text: root.books.length + (root.books.length === 1 ? " book" : " books")
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                    }

                    // Sort dropdown
                    SortDropdown {
                        width: 200; height: 36
                        anchors.verticalCenter: parent.verticalCenter
                        options: [
                            { label: "Relevance", value: "relevance" },
                            { label: "Price ↑",    value: "price_asc" },
                            { label: "Price ↓",    value: "price_desc" },
                            { label: "Top rated",  value: "rating" },
                            { label: "Newest",     value: "newest" }
                        ]
                        currentValue: root._sortMode
                        onChanged: function(value) { root._sortMode = value }
                    }

                    // View toggle
                    ViewToggle {
                        id: _viewToggle
                        width: 76; height: 36
                        mode: root._viewMode
                        onModeChanged: { root._viewMode = _viewToggle.mode }
                    }
                }
            }

            // ----- Content -----
            Item {
                width: parent.width
                height: _contentCol.implicitHeight
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding

                Column {
                    id: _contentCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.xl

                    // Empty state
                    EmptyIllustration {
                        width: parent.width
                        height: 480
                        visible: root.books.length === 0
                        iconName: "search_off"
                        title: "No books in this category yet"
                        description: "Check back later — new books are added regularly."
                        primaryActionLabel: "Back to Home"
                        onPrimaryActionTriggered: root.backRequested()
                    }

                    // Grid view
                    Grid {
                        width: parent.width
                        visible: root.books.length > 0 && root._viewMode === "grid"
                        columns: root._gridColumns
                        spacing: Theme.space.xl

                        Repeater {
                            model: root._displayBooks
                            delegate: BookCard {
                                width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                book: modelData
                                onClicked: root.bookDetailRequested(book.id)
                                onAddToCartClicked: root.addToCartRequested(book.id)
                                onWishlistClicked: root.wishlistToggleRequested(book.id)
                            }
                        }
                    }

                    // List view
                    Column {
                        width: parent.width
                        visible: root.books.length > 0 && root._viewMode === "list"
                        spacing: Theme.space.sm

                        Repeater {
                            model: root._displayBooks
                            delegate: BookRow {
                                width: parent.width
                                book: modelData
                                onClicked: root.bookDetailRequested(book.id)
                                onAddToCartClicked: root.addToCartRequested(book.id)
                                onWishlistClicked: root.wishlistToggleRequested(book.id)
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
