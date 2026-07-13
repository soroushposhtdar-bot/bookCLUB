// =============================================================================
//  CartPage.qml
// =============================================================================
//  Shopping cart page.
//
//  Layout:
//      • Two-column: cart items list on the left, sticky order summary on the
//        right (subtotal / discount / total / checkout button).
//      • Empty state when there are no items.
//      • Checkout success → emits checkoutSuccessRequested, which the router
//        uses to push a brief success page that then routes the user to the
//        library.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/book"
import "../components/data"
import "../components/feedback"
import "../components/progress"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // CartViewModel

    signal backRequested()
    signal checkoutSuccessRequested()
    signal continueShoppingRequested()

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property bool _isEmpty: root.viewModel && root.viewModel.isEmpty
    readonly property bool _isBusy: root.viewModel && root.viewModel.isBusy

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // ----- Empty state -----
    EmptyState {
        anchors.fill: parent
        visible: root._isEmpty
        iconName: "shopping_cart"
        title: "Your cart is empty"
        description: "Browse the catalog and add books to your cart to see them here."
        actionLabel: "Discover books"
        onActionTriggered: root.continueShoppingRequested()
    }

    // ----- Loading overlay -----
    LoadingOverlay {
        anchors.fill: parent
        visible: root._isBusy && !root._isEmpty
    }

    // ----- Cart contents -----
    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height
        visible: !root._isEmpty

        Column {
            id: _column
            width: parent.width
            spacing: Theme.space.xl

            Item { width: 1; height: Theme.space.sm }

            // Header
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.md

                Text {
                    text: "Your cart"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle {
                    width: _countText.implicitWidth + 16
                    height: 26
                    radius: 13
                    color: Theme.color.fieldFilled
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        id: _countText
                        anchors.centerIn: parent
                        text: (root.viewModel ? root.viewModel.itemCount : 0) + " items"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightMedium
                    }
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                TextButton {
                    text: "Continue shopping"
                    iconName: "arrow_back"
                    onClicked: root.continueShoppingRequested()
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // ----- Two-column layout -----
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.xl

                // ----- Items list -----
                Column {
                    width: parent.width - 360 - Theme.space.xl
                    spacing: Theme.space.md

                    Repeater {
                        model: root.viewModel ? root.viewModel.items : []

                        delegate: Card {
                            width: parent.width
                            elevation: "none"
                            bordered: true
                            padding: Theme.space.lg

                            Row {
                                width: parent.width
                                spacing: Theme.space.lg

                                // Mini cover
                                Item {
                                    width: 64
                                    height: 96
                                    BookCover {
                                        anchors.fill: parent
                                        book: modelData
                                        cornerRadius: Theme.radius.sm
                                    }
                                }

                                // Title + author
                                Column {
                                    width: parent.width - 64 - Theme.space.lg - _priceCol.width - Theme.space.lg - _removeBtn.width - Theme.space.md
                                    spacing: Theme.space.xs
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        text: modelData.title
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBodyLarge
                                        font.weight: Theme.font.weightSemibold
                                        width: parent.width
                                        elide: Text.ElideRight
                                        maximumLineCount: 2
                                        wrapMode: Text.WordWrap
                                    }
                                    Text {
                                        text: modelData.authorName
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }

                                    Rectangle {
                                        visible: modelData.hasDiscount
                                        width: _discText.implicitWidth + 12
                                        height: 20
                                        radius: 10
                                        color: Theme.color.errorSoft

                                        Text {
                                            id: _discText
                                            anchors.centerIn: parent
                                            text: "On sale"
                                            color: Theme.color.error
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeMicro2
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                }

                                // Price column
                                Column {
                                    id: _priceCol
                                    width: 90
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter

                                    Text {
                                        text: modelData.unitPriceText
                                        color: Theme.color.primary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBodyLarge
                                        font.weight: Theme.font.weightBold
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                    Text {
                                        visible: modelData.hasDiscount
                                        text: modelData.basePriceText
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.strikeout: true
                                        anchors.horizontalCenter: parent.horizontalCenter
                                    }
                                }

                                // Remove button
                                IconButton {
                                    id: _removeBtn
                                    iconName: "delete_outline"
                                    iconColor: Theme.color.textMuted
                                    hoverIconColor: Theme.color.error
                                    width: 40
                                    height: 40
                                    anchors.verticalCenter: parent.verticalCenter
                                    onClicked: {
                                        if (root.viewModel) root.viewModel.removeItem(modelData.bookId)
                                    }
                                }
                            }
                        }
                    }

                    // Clear all
                    Row {
                        width: parent.width
                        layoutDirection: Qt.RightToLeft
                        TextButton {
                            text: "Clear cart"
                            color: Theme.color.error
                            hoverColor: Theme.color.error
                            iconName: "delete"
                            onClicked: {
                                if (root.viewModel) root.viewModel.clear()
                            }
                        }
                    }
                }

                // ----- Order summary (sticky) -----
                Card {
                    width: 360
                    elevation: "sm"
                    padding: Theme.space.xl
                    anchors.top: parent.top

                    Column {
                        width: parent.width
                        spacing: Theme.space.lg

                        Text {
                            text: "Order summary"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }

                        // Subtotal
                        Row {
                            width: parent.width
                            Text {
                                text: "Subtotal"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { width: 1; height: 1; Layout.fillWidth: true }
                            Text {
                                text: root.viewModel ? root.viewModel.subtotalText : "$0.00"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightMedium
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        // Discount
                        Row {
                            width: parent.width
                            visible: root.viewModel && root.viewModel.discountTotal > 0

                            Text {
                                text: "Discount"
                                color: Theme.color.success
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { width: 1; height: 1; Layout.fillWidth: true }
                            Text {
                                text: root.viewModel ? root.viewModel.discountText : "$0.00"
                                color: Theme.color.success
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightSemibold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Divider {
                            width: parent.width
                            orientation: "horizontal"
                        }

                        // Total
                        Row {
                            width: parent.width
                            Text {
                                text: "Total"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBodyLarge
                                font.weight: Theme.font.weightSemibold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { width: 1; height: 1; Layout.fillWidth: true }
                            Text {
                                text: root.viewModel ? root.viewModel.totalText : "$0.00"
                                color: Theme.color.primary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeHeadline
                                font.weight: Theme.font.weightBold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        // Savings note
                        Text {
                            visible: root.viewModel && root.viewModel.discountTotal > 0
                            text: root.viewModel ? root.viewModel.savingsText : ""
                            color: Theme.color.success
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }

                        // Checkout
                        PrimaryButton {
                            width: parent.width
                            text: "Proceed to checkout"
                            iconName: "checkout"
                            iconPosition: "trailing"
                            enabled: !root._isBusy && !(root.viewModel && root.viewModel.isEmpty)
                            loading: root._isBusy
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.checkout()
                                }
                            }
                        }

                        // Trust note
                        Row {
                            width: parent.width
                            spacing: Theme.space.xs
                            layoutDirection: Qt.LeftToRight

                            AppIcon {
                                name: "lock"
                                size: 14
                                color: Theme.color.textMuted
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "Secure checkout — your payment is encrypted."
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                wrapMode: Text.WordWrap
                                width: parent.width - 18
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // Watch checkout success
    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onCheckoutSucceeded: {
            root.checkoutSuccessRequested()
        }
        onCheckoutFailed: {
            // Toast shown by App.qml router via global toast manager
        }
    }
}
