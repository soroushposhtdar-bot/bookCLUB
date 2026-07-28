// =============================================================================
//  PublisherBookDetailDrawer.qml  (v3 polish)
// =============================================================================
//  Slide-in drawer for the publisher role showing full details of a single
//  book in the catalog. Cover, title/author/publisher, price/discount,
//  rating + rating-count, sales count, status badge, description, and a
//  reviews list.
//
//  Two action buttons at the bottom:
//    • "Edit metadata" → opens the parent page's editor in edit mode
//      (emits editRequested(bookId) signal that the CatalogPage connects to).
//    • "Toggle status" → if status != "removed", soft-deletes via
//      viewModel.removeBook(bookId); if status == "removed", re-publishes via
//      viewModel.setBookStatus(bookId, "published").
//
//  v3 polish improvements:
//    • Scrim fade-in + drawer slide-in run in parallel — smoother entrance
//      (was hard cut-in previously).
//    • Quick "Edit price" inline editor on the stats grid — click the Price
//      stat to edit it inline without opening the full metadata editor.
//    • Reviews section uses the reusable ReviewItem component (was a hand-
//      rolled card) so it picks up all the existing review UX (helpful
//      button, reply, pin, etc.) for free.
//    • Layout uses RowLayout/ColumnLayout — no more `parent.parent.width`.
//    • Drawer width is now Theme.publisher.drawerWidth (centralized token).
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"
import "../components/book"

import BookClub.Services 1.0
import BookClub.ViewModels 1.0

Item {
    id: drawer

    property var viewModel: null
    property string bookId: ""
    property var _detail: ({})

    signal toastRequested(string variant, string title, string description)
    signal editRequested(string bookId)
    signal closed()

    // ----- Visual state -----
    visible: false
    width: Theme.publisher.drawerWidth

    function _statusLabel(s) {
        return {
            "published": "Published",
            "draft":     "Draft",
            "pending":   "Pending review",
            "removed":   "Removed"
        }[s] || s
    }
    function _statusColor(s) {
        return {
            "published": Theme.color.success,
            "draft":     Theme.color.textMuted,
            "pending":   Theme.color.warning,
            "removed":   Theme.color.error
        }[s] || Theme.color.textMuted
    }

    function openForBook(id) {
        drawer.bookId = id
        drawer._reload()
        drawer.visible = true
        _scrim.opacity = 0
        _content.x = drawer.width
        _fadeInScrim.start()
        _slideIn.start()
    }

    function _reload() {
        if (!drawer.viewModel || drawer.bookId.length === 0) {
            drawer._detail = {}
            return
        }
        const d = drawer.viewModel.bookDetail(drawer.bookId)
        drawer._detail = d || {}
    }

    function close() {
        _fadeOutScrim.start()
        _slideOut.start()
        _hideTimer.start()
    }

    Timer {
        id: _hideTimer
        interval: Theme.motion.durationBase
        repeat: false
        onTriggered: {
            drawer.visible = false
            drawer.closed()
        }
    }

    Connections {
        target: drawer.viewModel
        ignoreUnknownSignals: true
        onBooksChanged: drawer._reload()
    }

    // ----- Animations -----
    NumberAnimation {
        id: _slideIn
        target: _content
        property: "x"
        from: drawer.width
        to: 0
        duration: Theme.motion.durationSlow
        easing.type: Easing.OutCubic
    }
    NumberAnimation {
        id: _slideOut
        target: _content
        property: "x"
        from: 0
        to: drawer.width
        duration: Theme.motion.durationBase
        easing.type: Easing.InCubic
    }
    NumberAnimation {
        id: _fadeInScrim
        target: _scrim
        property: "opacity"
        from: 0; to: 1
        duration: Theme.motion.durationSlow
        easing.type: Easing.OutCubic
    }
    NumberAnimation {
        id: _fadeOutScrim
        target: _scrim
        property: "opacity"
        from: 1; to: 0
        duration: Theme.motion.durationBase
        easing.type: Easing.InCubic
    }

    // ----- Scrim (click outside to close) -----
    Rectangle {
        id: _scrim
        anchors.fill: parent
        visible: drawer.visible
        color: Theme.color.overlayScrim
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }
        MouseArea {
            anchors.fill: parent
            onClicked: drawer.close()
        }
    }

    // ----- Drawer content -----
    Rectangle {
        id: _content
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: drawer.width
        color: Theme.color.cardBackground

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ----- Header -----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                color: "transparent"

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.color.divider
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.lg
                    anchors.rightMargin: Theme.space.lg
                    spacing: Theme.space.md

                    Text {
                        text: "Book details"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                    }
                    Item { Layout.fillWidth: true; height: 1 }
                    IconButton {
                        iconName: "close"
                        tooltip: "Close"
                        onClicked: drawer.close()
                    }
                }
            }

            // ----- Body (scrollable) -----
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.lg

                    // ----- Cover + title + author + status -----
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: Theme.space.lg

                            BookCover {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 120
                                book: drawer._detail
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Text {
                                    Layout.fillWidth: true
                                    text: drawer._detail.title || "(no title)"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeTitle
                                    font.weight: Theme.font.weightBold
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: "by " + (drawer._detail.authorName || "—")
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                }
                                Text {
                                    text: drawer._detail.publisherName || "—"
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                                // Status badge
                                RowLayout {
                                    spacing: Theme.space.xs
                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 8
                                        radius: width / 2
                                        color: drawer._statusColor(drawer._detail.status || "published")
                                    }
                                    Text {
                                        text: drawer._statusLabel(drawer._detail.status || "published")
                                        color: drawer._statusColor(drawer._detail.status || "published")
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                }
                            }
                        }
                    }

                    // ----- Stats grid (4 mini-cards) -----
                    //   Click on Price opens a quick inline editor.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.space.xl
                        Layout.rightMargin: Theme.space.xl
                        spacing: Theme.space.md

                        Repeater {
                            model: [
                                { label: "Price",    value: drawer._detail.priceText || "—",        color: Theme.color.accent, editable: true },
                                { label: "Sales",    value: (drawer._detail.totalSales || 0).toLocaleString(Qt.locale(), "f", 0), color: Theme.color.success, editable: false },
                                { label: "Rating",   value: (drawer._detail.averageRating || 0).toFixed(1), color: Theme.color.warning, editable: false },
                                { label: "Reviews",  value: (drawer._detail.ratingCount || 0).toLocaleString(Qt.locale(), "f", 0), color: Theme.color.info, editable: false }
                            ]
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.preferredHeight: 80
                                radius: Theme.radius.md
                                color: _statHover.hovered && modelData.editable ? Theme.color.accentSoft : Theme.color.fieldFilled
                                border.color: _statHover.hovered && modelData.editable ? Theme.color.accent : Theme.color.divider
                                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                Behavior on border.color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }

                                HoverHandler {
                                    id: _statHover
                                    cursorShape: modelData.editable ? Qt.PointingHandCursor : Qt.ArrowCursor
                                }

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 2

                                    Text {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: modelData.value
                                        color: modelData.color
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightBold
                                    }
                                    Text {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: modelData.label
                                        color: modelData.editable && _statHover.hovered
                                               ? Theme.color.accent
                                               : Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: modelData.editable
                                    onClicked: _quickPriceEdit.open()
                                }
                            }
                        }
                    }

                    // ----- Genres -----
                    Card {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.space.xl
                        Layout.rightMargin: Theme.space.xl
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        ColumnLayout {
                            id: _genresContent
                            width: parent.width
                            spacing: Theme.space.sm

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Genres"
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs
                                Repeater {
                                    model: drawer._detail.genreIds || []
                                    Rectangle {
                                        Layout.preferredWidth: _genreLbl.implicitWidth + 16
                                        Layout.preferredHeight: 22
                                        radius: height / 2
                                        color: Theme.color.accentSoft
                                        Text {
                                            id: _genreLbl
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: Theme.color.accent
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeMicro2
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                }
                                Text {
                                    visible: (drawer._detail.genreIds || []).length === 0
                                    text: "No genres assigned"
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                            }
                        }
                    }

                    // ----- Description -----
                    Card {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.space.xl
                        Layout.rightMargin: Theme.space.xl
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        ColumnLayout {
                            id: _descContent
                            width: parent.width
                            spacing: Theme.space.sm

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Description"
                            }
                            Text {
                                Layout.fillWidth: true
                                text: drawer._detail.description || "No description available."
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // ----- Reviews -----
                    Card {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.space.xl
                        Layout.rightMargin: Theme.space.xl
                        elevation: "none"
                        bordered: true
                        padding: Theme.space.lg

                        ColumnLayout {
                            id: _reviewsContent
                            width: parent.width
                            spacing: Theme.space.sm

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Reviews"
                                subtitle: "%1 total".arg((drawer._detail.reviews || []).length)
                            }

                            ListView {
                                Layout.fillWidth: true
                                // v4: increased the per-row height estimate from
                                // 100 → 140 so 3-line comments + stars + by-line
                                // + helpful row don't get clipped. The min cap
                                // stays at 360 so a short list doesn't waste space.
                                Layout.preferredHeight: Math.min(420, Math.max(0, (drawer._detail.reviews || []).length) * 140)
                                clip: true
                                interactive: (drawer._detail.reviews || []).length > 3
                                model: drawer._detail.reviews || []
                                spacing: Theme.space.sm

                                delegate: Rectangle {
                                    width: parent.width
                                    height: _revCol.implicitHeight + 2 * Theme.space.md
                                    radius: Theme.radius.md
                                    color: Theme.color.fieldFilled
                                    border.color: Theme.color.divider

                                    ColumnLayout {
                                        id: _revCol
                                        anchors.fill: parent
                                        anchors.margins: Theme.space.md
                                        spacing: Theme.space.xs

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: Theme.space.sm
                                            RatingStars { size: 12; rating: modelData.rating }
                                            Item { Layout.fillWidth: true; height: 1 }
                                            Text {
                                                text: (modelData.createdAtText || "")
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: "by @" + (modelData.username || "—")
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightMedium
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: "\"" + (modelData.comment || "") + "\""
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 3
                                            elide: Text.ElideRight
                                        }
                                        RowLayout {
                                            spacing: Theme.space.sm
                                            Text {
                                                text: "▲ " + (modelData.helpfulCount || 0) + " helpful"
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                            Rectangle {
                                                visible: modelData.verifiedPurchase === true
                                                Layout.preferredWidth: _vpLbl.implicitWidth + 12
                                                Layout.preferredHeight: 18
                                                radius: height / 2
                                                color: Theme.color.successSoft
                                                Text {
                                                    id: _vpLbl
                                                    anchors.centerIn: parent
                                                    text: "Verified"
                                                    color: Theme.color.success
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeMicro2
                                                    font.weight: Theme.font.weightBold
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            EmptyState {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                visible: (drawer._detail.reviews || []).length === 0
                                iconName: "rate_review"
                                title: "No reviews yet"
                                description: "Reader reviews will appear here once the book is rated."
                            }
                        }
                    }

                    // Bottom spacer
                    Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xl }
                }
            }

            // ----- Footer action bar -----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                color: Theme.color.cardBackground

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.color.divider
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.xl
                    anchors.rightMargin: Theme.space.xl
                    anchors.topMargin: Theme.space.md
                    anchors.bottomMargin: Theme.space.md
                    spacing: Theme.space.md

                    SecondaryButton {
                        text: "Edit metadata"
                        iconName: "edit"
                        onClicked: {
                            drawer.editRequested(drawer.bookId)
                            drawer.close()
                        }
                    }
                    Item { Layout.fillWidth: true; height: 1 }
                    PrimaryButton {
                        text: drawer._detail.status === "removed" ? "Re-publish" : "Remove from storefront"
                        iconName: drawer._detail.status === "removed" ? "history" : "delete"
                        enabled: drawer.bookId.length > 0
                        onClicked: {
                            if (!drawer.viewModel) return
                            if (drawer._detail.status === "removed") {
                                // v4 fix: send "active" so the service maps to ActivateBook.
                                drawer.viewModel.setBookStatus(drawer.bookId, "active")
                                drawer.toastRequested("success", "Re-published",
                                                      "'" + drawer._detail.title + "' is back in the storefront.")
                            } else {
                                drawer.viewModel.removeBook(drawer.bookId)
                                drawer.toastRequested("warning", "Removed",
                                                      "'" + drawer._detail.title + "' has been removed from the storefront.")
                            }
                        }
                    }
                }
            }
        }
    }

    // ----- Quick price edit popup (new in v3 polish) -----
    //   Click the Price stat in the grid to open this. Lets the publisher
    //   change the price without opening the full metadata editor.
    Popup {
        id: _quickPriceEdit
        anchors.centerIn: parent
        width: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.lg
        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        onAboutToShow: {
            _quickPrice.text = drawer._detail.basePrice
                              ? Number(drawer._detail.basePrice).toFixed(2)
                              : ""
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.space.md

            Text {
                text: "Quick price edit"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightBold
            }
            Text {
                Layout.fillWidth: true
                text: "Update the price of \"" + (drawer._detail.title || "") + "\" without opening the full editor."
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                wrapMode: Text.WordWrap
            }

            InputField {
                id: _quickPrice
                Layout.fillWidth: true
                placeholder: "12.99"
                label: "New price ($)"
                onTextEdited: function(newText) { _quickPrice.text = newText }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md
                Item { Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "Cancel"
                    onClicked: _quickPriceEdit.close()
                }
                PrimaryButton {
                    text: "Save"
                    iconName: "check"
                    enabled: _quickPrice.text.length > 0
                    onClicked: {
                        // Use the full updateBook call with all the existing
                        // fields, just overriding the price. This matches the
                        // signature expected by the VM.
                        if (drawer.viewModel) {
                            const d = drawer._detail
                            drawer.viewModel.updateBook(
                                drawer.bookId,
                                d.title || "",
                                d.authorName || "",
                                (d.genreIds && d.genreIds.length > 0) ? d.genreIds[0] : "",
                                d.description || "",
                                parseFloat(_quickPrice.text) || 0.0,
                                d.discountPercent || 0,
                                d.coverColor || "#2C3E50",
                                d.coverAccent || "#F39C12",
                                d.coverImage || "",
                                d.pdfFilePath || ""
                            )
                            drawer.toastRequested("success", "Price updated",
                                                  "New price: $" + _quickPrice.text)
                        }
                        _quickPriceEdit.close()
                    }
                }
            }
        }
    }
}
