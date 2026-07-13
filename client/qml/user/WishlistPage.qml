// =============================================================================
//  WishlistPage.qml
// =============================================================================
//  Professional wishlist page — grid/list toggle, sort, search, bulk select,
//  move-to-cart, remove. Empty + loading states.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/book"
import "../components/data"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // WishlistViewModel

    signal bookDetailRequested(string bookId)
    signal openCartRequested()
    signal continueShoppingRequested()

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property int _gridColumns: root.width < 760 ? 2 : (root.width < 1100 ? 3 : 5)

    Rectangle { anchors.fill: parent; color: Theme.color.pageBackground }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.xl

            Item { width: 1; height: Theme.space.sm }

            // ----- Header + controls -----
            Item {
                width: parent.width
                height: _headerCol.implicitHeight
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding

                Column {
                    id: _headerCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.md

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                            Text {
                                text: "Wishlist"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeHeadline
                                font.weight: Theme.font.weightBold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Rectangle {
                                width: _countText.implicitWidth + 16; height: 26; radius: 13
                                color: Theme.color.fieldFilled
                                Text {
                                    id: _countText
                                    anchors.centerIn: parent
                                    text: (root.viewModel ? root.viewModel.count : 0) + " saved"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { Layout.fillWidth: true; width: 1; height: 1 }

                            // Bulk-select toggle
                            Rectangle {
                                width: _bulkBtn.implicitWidth + 20; height: 36
                                radius: Theme.radius.md
                                color: root.viewModel && root.viewModel.bulkMode ? Theme.color.accentSoft : Theme.color.cardBackground
                                border.color: root.viewModel && root.viewModel.bulkMode ? Theme.color.accent : Theme.color.border
                                border.width: 1
                                Row {
                                    id: _bulkBtn
                                    anchors.centerIn: parent
                                    spacing: 6
                                    AppIcon {
                                        name: "check_box"
                                        size: 16
                                        color: root.viewModel && root.viewModel.bulkMode ? Theme.color.accent : Theme.color.textSecondary
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "Select"
                                        color: root.viewModel && root.viewModel.bulkMode ? Theme.color.accent : Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (root.viewModel) root.viewModel.bulkMode = !root.viewModel.bulkMode
                                }
                            }

                            SortDropdown {
                                width: 180; height: 36
                                options: [
                                    { label: "Recently added", value: "recent" },
                                    { label: "Title A–Z",      value: "title" },
                                    { label: "Price ↑",        value: "price_asc" },
                                    { label: "Price ↓",        value: "price_desc" },
                                    { label: "Top rated",      value: "rating" }
                                ]
                                currentValue: root.viewModel ? root.viewModel.sortMode : "recent"
                                onChanged: function(value) { if (root.viewModel) root.viewModel.sortMode = value }
                            }

                            ViewToggle {
                                width: 76; height: 36
                                mode: root.viewModel ? root.viewModel.viewMode : "grid"
                                onModeChanged: function(mode) { if (root.viewModel) root.viewModel.viewMode = mode }
                            }
                        }

                        // Search + bulk actions row
                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                                SearchField {
                                    width: 320
                                    height: 40
                                    placeholder: "Search wishlist..."
                                    text: root.viewModel ? root.viewModel.searchQuery : ""
                                    onTextEdited: if (root.viewModel) root.viewModel.searchQuery = newText
                                }

                                Item { Layout.fillWidth: true; width: 1; height: 1 }

                                // Bulk action bar (visible in bulk mode)
                                Row {
                                    spacing: Theme.space.sm
                                    visible: root.viewModel && root.viewModel.bulkMode && root.viewModel.selectedCount > 0

                                        Text {
                                            text: (root.viewModel ? root.viewModel.selectedCount : 0) + " selected"
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        SecondaryButton {
                                            text: "Move to cart"
                                            iconName: "shopping_cart"
                                            iconPosition: "leading"
                                            height: 36
                                            onClicked: if (root.viewModel) root.viewModel.moveSelectedToCart()
                                        }
                                        SecondaryButton {
                                            text: "Remove"
                                            iconName: "delete_outline"
                                            iconPosition: "leading"
                                            height: 36
                                            onClicked: if (root.viewModel) root.viewModel.removeSelected()
                                        }
                                        TextButton {
                                            text: "Cancel"
                                            onClicked: if (root.viewModel) root.viewModel.bulkMode = false
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Summary card (spec-required aggregate stats) -----
                        // Shows total wishlist value, item count, active-discount
                        // count, and the biggest saving. Hidden when the wishlist
                        // is empty (the empty state below handles that case).
                        Card {
                            width: parent.width
                            bordered: true
                            padding: Theme.space.lg
                            visible: root.viewModel && !root.viewModel.isEmpty

                            Row {
                                width: parent.width
                                spacing: Theme.space.xl

                                // Total value
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "Total value"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        text: root.viewModel ? root.viewModel.totalValueText : "$0.00"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                // Divider
                                Rectangle { width: 1; height: 40; color: Theme.color.divider; anchors.verticalCenter: parent.verticalCenter }

                                // Item count
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "Items"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        text: (root.viewModel ? root.viewModel.count : 0).toString()
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                // Divider
                                Rectangle { width: 1; height: 40; color: Theme.color.divider; anchors.verticalCenter: parent.verticalCenter }

                                // Active discounts
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "On sale"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        text: (root.viewModel ? root.viewModel.discountedCount : 0).toString()
                                        color: (root.viewModel && root.viewModel.discountedCount > 0) ? Theme.color.warning : Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightBold
                                    }
                                }

                                // Divider
                                Rectangle { width: 1; height: 40; color: Theme.color.divider; anchors.verticalCenter: parent.verticalCenter }

                                // Max discount
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text {
                                        text: "Biggest saving"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        text: root.viewModel && root.viewModel.maxDiscountPercent > 0
                                              ? "%1% off — %2".arg(root.viewModel.maxDiscountPercent).arg(root.viewModel.maxDiscountBookTitle)
                                              : "—"
                                        color: root.viewModel && root.viewModel.maxDiscountPercent > 0 ? Theme.color.success : Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightSemibold
                                        elide: Text.ElideRight
                                        width: 240
                                    }
                                }

                                Item { Layout.fillWidth: true; width: 1; height: 1 }

                                // Move all to cart button
                                PrimaryButton {
                                    text: "Move all to cart"
                                    iconName: "shopping_cart"
                                    anchors.verticalCenter: parent.verticalCenter
                                    enabled: root.viewModel && root.viewModel.count > 0
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.selectAll()
                                            root.viewModel.moveSelectedToCart()
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Content -----
                        Item {
                            width: parent.width
                            height: _content.implicitHeight
                            anchors.leftMargin: root._horizontalPadding
                            anchors.rightMargin: root._horizontalPadding

                            Column {
                                id: _content
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: root._horizontalPadding
                                anchors.rightMargin: root._horizontalPadding
                                spacing: 0

                                    // Empty state
                                    EmptyIllustration {
                                        width: parent.width
                                        height: 480
                                        visible: root.viewModel && root.viewModel.isEmpty
                                        iconName: "favorite_border"
                                        title: "Your wishlist is empty"
                                        description: "Tap the heart on any book to save it for later."
                                        primaryActionLabel: "Discover books"
                                        onPrimaryActionTriggered: root.continueShoppingRequested()
                                    }

                                    // Grid view
                                    Grid {
                                        width: parent.width
                                        visible: root.viewModel && !root.viewModel.isEmpty && (root.viewModel.viewMode === "grid")
                                        columns: root._gridColumns
                                        spacing: Theme.space.xl

                                        Repeater {
                                            model: root.viewModel && root.viewModel.viewMode === "grid" ? root.viewModel.books : []
                                            delegate: Item {
                                                width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                                height: _gridCard.height

                                                // Selection checkbox overlay (bulk mode)
                                                Rectangle {
                                                    visible: root.viewModel && root.viewModel.bulkMode
                                                    anchors.top: parent.top
                                                    anchors.left: parent.left
                                                    anchors.topMargin: 8
                                                    anchors.leftMargin: 8
                                                    width: 28; height: 28; radius: 14
                                                    color: root.viewModel && root.viewModel.isSelected(modelData.id) ? Theme.color.accent : Theme.color.cardBackground
                                                    border.color: root.viewModel && root.viewModel.isSelected(modelData.id) ? Theme.color.accent : Theme.color.borderStrong
                                                    border.width: 2
                                                    z: 10
                                                    AppIcon {
                                                        anchors.centerIn: parent
                                                        name: "check"
                                                        size: 18
                                                        color: Theme.color.textOnAccent
                                                        visible: root.viewModel && root.viewModel.isSelected(modelData.id)
                                                    }
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: if (root.viewModel) root.viewModel.toggleSelected(modelData.id)
                                                    }
                                                }

                                                BookCard {
                                                    id: _gridCard
                                                    width: parent.width
                                                    book: modelData
                                                    onClicked: {
                                                        if (root.viewModel && root.viewModel.bulkMode) {
                                                            root.viewModel.toggleSelected(book.id)
                                                        } else {
                                                            root.bookDetailRequested(book.id)
                                                        }
                                                    }
                                                    onWishlistClicked: if (root.viewModel) root.viewModel.remove(book.id)
                                                    onAddToCartClicked: if (root.viewModel) root.viewModel.moveToCart(book.id)
                                                }
                                            }
                                        }
                                    }

                                    // List view
                                    Column {
                                        width: parent.width
                                        visible: root.viewModel && !root.viewModel.isEmpty && (root.viewModel.viewMode === "list")
                                        spacing: Theme.space.sm

                                        Repeater {
                                            model: root.viewModel && root.viewModel.viewMode === "list" ? root.viewModel.books : []
                                            delegate: BookRow {
                                                width: parent.width
                                                book: modelData
                                                selected: root.viewModel && root.viewModel.isSelected(modelData.id)
                                                onClicked: {
                                                    if (root.viewModel && root.viewModel.bulkMode) {
                                                        root.viewModel.toggleSelected(book.id)
                                                    } else {
                                                        root.bookDetailRequested(book.id)
                                                    }
                                                }
                                                onWishlistClicked: if (root.viewModel) root.viewModel.remove(book.id)
                                                onAddToCartClicked: if (root.viewModel) root.viewModel.moveToCart(book.id)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { width: 1; height: Theme.space.xxl }
                    }
                }
