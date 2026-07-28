// =============================================================================
//  PublisherPromotionsPage.qml  (v4 rewrite — Issue 5)
// =============================================================================
//  Book-discount management for the publisher role. The publisher picks one
//  of their catalog books, sets a discount percentage, picks a start + end
//  date, and clicks "Apply discount" to schedule a timed discount. A list
//  of currently-active discounts sits below the form, each removable.
//
//  Data source: page.viewModel (PublisherViewModel).
//    • viewModel.books      — publisher's catalog (QList<BookDto*>)
//    • viewModel.promotions — QVariantList of discount records:
//        { code, description, discount, status, uses, cap,
//          startDate, endDate, period }
//    • viewModel.addPromotion(bookId, description, discountPercent, cap,
//                              startDate, endDate) -> bool
//    • viewModel.removePromotion(code) -> bool
//
//  Note: the existing PublisherViewModel.addPromotion signature already
//  matches a "book-discount" call — the first argument is the entity ID
//  (here the bookId), the third is the percent, and cap is unused (0).
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/navigation"
import BookClub.Services 1.0
import BookClub.ViewModels 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    // -------------------------------------------------------------------------
    //  State
    // -------------------------------------------------------------------------
    // Publisher's catalog (list of BookDto*).
    readonly property var _books: page.viewModel ? page.viewModel.books : []

    // All discount records returned by the VM (we filter to "active" for the
    // list, but expose the full list too in case we want to show expired
    // ones later).
    readonly property var _promos: page.viewModel ? page.viewModel.promotions : []

    // Active-only list for the "Active discounts" panel below.
    readonly property var _activePromos: {
        if (!page._promos) return []
        var out = []
        for (var i = 0; i < page._promos.length; ++i) {
            var s = page._promos[i].status
            if (s === "active" || s === "scheduled") out.push(page._promos[i])
        }
        return out
    }

    // -------------------------------------------------------------------------
    //  Date helpers (YYYY-MM-DD)
    // -------------------------------------------------------------------------
    function _formatDate(date) {
        var y = date.getFullYear()
        var m = String(date.getMonth() + 1).padStart(2, "0")
        var d = String(date.getDate()).padStart(2, "0")
        return "%1-%2-%3".arg(y).arg(m).arg(d)
    }
    function _parseDate(str) {
        if (!str || str.length === 0) return new Date()
        var parts = str.split("-")
        if (parts.length !== 3) return new Date()
        return new Date(parseInt(parts[0]), parseInt(parts[1]) - 1, parseInt(parts[2]))
    }

    function _statusLabel(s) {
        return { active: "Active", scheduled: "Scheduled", expired: "Expired" }[s] || (s || "—")
    }
    function _statusColor(s) {
        return { active: Theme.color.success, scheduled: Theme.color.info, expired: Theme.color.textMuted }[s] || Theme.color.textMuted
    }

    // -------------------------------------------------------------------------
    //  Apply discount — calls viewModel.addPromotion(bookId, desc, pct, 0, start, end)
    // -------------------------------------------------------------------------
    function _applyDiscount() {
        if (!page.viewModel) return

        if (page._books.length === 0) {
            page.toastRequested("error", "No books",
                                "Add a book to your catalog first.")
            return
        }

        var bookId = _bookCombo.currentBookId()
        if (!bookId || bookId.length === 0) {
            page.toastRequested("error", "Pick a book",
                                "Select a book from your catalog.")
            return
        }

        var pct = parseInt(_fDiscount.text, 10)
        if (isNaN(pct) || pct < 1 || pct > 100) {
            page.toastRequested("error", "Invalid discount",
                                "Discount must be between 1 and 100.")
            return
        }

        var start = _fStart.text
        var end = _fEnd.text
        if (!start || !end || start.length === 0 || end.length === 0) {
            page.toastRequested("error", "Pick dates",
                                "Choose a start and end date.")
            return
        }

        // v15i: validate that start date is not after end date
        var startDate = new Date(start)
        var endDate = new Date(end)
        if (startDate > endDate) {
            page.toastRequested("error", "Invalid date range",
                                "End date must be after start date.")
            return
        }

        // v15i: check if this book already has an active discount
        for (var i = 0; i < page._activePromos.length; ++i) {
            if (page._activePromos[i].code === bookId) {
                page.toastRequested("error", "Duplicate discount",
                                    "This book already has an active discount. Remove it first.")
                return
            }
        }

        // The description shown in the active-discount list
        var desc = "%1% off".arg(pct)

        var ok = page.viewModel.addPromotion(bookId, desc, pct, 0, start, end)
        if (ok) {
            page.toastRequested("success", "Discount applied",
                                "The discount has been applied to the selected book.")
            // Reset the form for the next discount.
            _fDiscount.text = ""
        } else {
            page.toastRequested("error", "Failed",
                                "Could not apply the discount. The book may already have one.")
        }
    }

    // -------------------------------------------------------------------------
    //  Layout
    // -------------------------------------------------------------------------
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ---------------- Header ----------------
            SectionHeader {
                Layout.fillWidth: true
                title: "Book discounts"
                subtitle: "Pick a book from your catalog and schedule a timed discount"
            }

            // ---------------- Apply discount card ----------------
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    id: _form
                    width: parent.width
                    spacing: Theme.space.md

                    // ---- Book picker ----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs

                        Text {
                            text: "Book"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        ComboBox {
                            id: _bookCombo
                            Layout.fillWidth: true
                            // model is a list of titles; we resolve the
                            // selected book ID through `_bookCombo.currentIndex`.
                            model: {
                                var titles = []
                                for (var i = 0; i < page._books.length; ++i) {
                                    titles.push(page._books[i].title)
                                }
                                return titles
                            }
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBodyLarge

                            // v12: don't declare `property currentValue` —
                            // ComboBox already has a FINAL `currentValue`
                            // property. Use a function instead.
                            function currentBookId() {
                                var idx = _bookCombo.currentIndex
                                if (idx >= 0 && idx < page._books.length) {
                                    return page._books[idx].id
                                }
                                return ""
                            }

                            background: Rectangle {
                                radius: Theme.radius.md
                                color: Theme.color.fieldBackground
                                border.color: _bookCombo.activeFocus ? Theme.color.accent : Theme.color.border
                                border.width: _bookCombo.activeFocus ? 2 : 1
                                Behavior on border.color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                            }
                            palette.text: Theme.color.textPrimary
                            palette.windowText: Theme.color.textPrimary
                            palette.base: Theme.color.fieldBackground
                            palette.window: Theme.color.fieldBackground
                            palette.highlightedText: Theme.color.onPrimary
                            palette.highlight: Theme.color.accent

                            onCurrentIndexChanged: _updatePriceDisplay()
                            Component.onCompleted: _updatePriceDisplay()
                        }

                        // ---- Current price display ----
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.sm
                            Text {
                                text: "Current price:"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                            }
                            Text {
                                id: _priceDisplay
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBodyLarge
                                font.weight: Theme.font.weightBold
                                text: "—"
                            }
                            Item { Layout.fillWidth: true; height: 1 }
                        }
                    }

                    // ---- Discount % ----
                    InputField {
                        id: _fDiscount
                        Layout.fillWidth: true
                        label: "Discount %"
                        placeholder: "25"
                        leadingIcon: "percent"
                        inputMethodHints: Qt.ImhDigitsOnly
                        helperText: "0–100"
                        onTextEdited: function(newText) {
                            _fDiscount.text = newText
                        }
                    }

                    // ---- Start / End dates ----
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.lg

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Start date"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            InputField {
                                id: _fStart
                                Layout.fillWidth: true
                                placeholder: "YYYY-MM-DD"
                                leadingIcon: "event"
                                readOnly: true
                                text: page._formatDate(new Date())
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: _startPicker.open()
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "End date"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            InputField {
                                id: _fEnd
                                Layout.fillWidth: true
                                placeholder: "YYYY-MM-DD"
                                leadingIcon: "event"
                                readOnly: true
                                text: page._formatDate(new Date(Date.now() + 7 * 24 * 60 * 60 * 1000))
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: _endPicker.open()
                                }
                            }
                        }
                    }

                    // ---- Quick presets + apply ----
                    // v15i: presets now set BOTH start and end dates
                    // properly, and the end date is validated against
                    // the start date in real time.
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md

                        TextButton {
                            text: "Today"
                            onClicked: {
                                // v15j: set start to today and end to today+7
                                _fStart.text = page._formatDate(new Date())
                                _fEnd.text = page._formatDate(new Date(Date.now() + 7 * 24 * 60 * 60 * 1000))
                            }
                        }
                        TextButton {
                            text: "+7 days"
                            onClicked: {
                                _fStart.text = page._formatDate(new Date())
                                _fEnd.text = page._formatDate(new Date(Date.now() + 7 * 24 * 60 * 60 * 1000))
                            }
                        }
                        TextButton {
                            text: "+30 days"
                            onClicked: {
                                _fStart.text = page._formatDate(new Date())
                                _fEnd.text = page._formatDate(new Date(Date.now() + 30 * 24 * 60 * 60 * 1000))
                            }
                        }
                        TextButton {
                            text: "+90 days"
                            onClicked: {
                                _fStart.text = page._formatDate(new Date())
                                _fEnd.text = page._formatDate(new Date(Date.now() + 90 * 24 * 60 * 60 * 1000))
                            }
                        }

                        Item { Layout.fillWidth: true; height: 1 }

                        PrimaryButton {
                            text: "Apply discount"
                            // v15i: disable when no books, no discount %,
                            // or end date is before start date.
                            enabled: page._books.length > 0
                                     && _fDiscount.text.length > 0
                                     && parseInt(_fDiscount.text, 10) > 0
                                     && new Date(_fEnd.text) >= new Date(_fStart.text)
                            onClicked: page._applyDiscount()
                        }
                    }

                    // v15i: real-time validation message
                    Text {
                        Layout.fillWidth: true
                        visible: {
                            var s = new Date(_fStart.text)
                            var e = new Date(_fEnd.text)
                            return e < s
                        }
                        text: "⚠ End date must be after start date"
                        color: Theme.color.error
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightMedium
                    }
                }
            }

            // ---------------- Active discounts list ----------------
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Active discounts"
                        subtitle: "Discounts currently scheduled on your books"
                    }

                    // Header row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md
                        visible: page._activePromos.length > 0

                        Text {
                            Layout.preferredWidth: 220
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            text: "BOOK / DESCRIPTION"
                        }
                        Text {
                            Layout.preferredWidth: 90
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            text: "DISCOUNT"
                        }
                        Text {
                            Layout.preferredWidth: 200
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            text: "PERIOD"
                        }
                        Text {
                            Layout.preferredWidth: 90
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            text: "STATUS"
                        }
                        Item { Layout.fillWidth: true; height: 1 }
                        Text {
                            Layout.preferredWidth: 80
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                            text: ""
                        }
                    }

                    // Discount rows
                    Repeater {
                        model: page._activePromos
                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.md

                                Text {
                                    Layout.preferredWidth: 220
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                    text: modelData.description || modelData.code || "(discount)"
                                }
                                Text {
                                    Layout.preferredWidth: 90
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    text: "%1%".arg(modelData.discount || 0)
                                }
                                Text {
                                    Layout.preferredWidth: 200
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    text: (modelData.startDate || "—") + " → " + (modelData.endDate || "—")
                                }
                                Text {
                                    Layout.preferredWidth: 90
                                    color: page._statusColor(modelData.status)
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    text: page._statusLabel(modelData.status)
                                }
                                Item { Layout.fillWidth: true; height: 1 }
                                TextButton {
                                    Layout.preferredWidth: 80
                                    text: "Remove"
                                    onClicked: {
                                        if (page.viewModel) {
                                            page.viewModel.removePromotion(modelData.code)
                                        }
                                    }
                                }
                            }

                            // Separator line between rows
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: Theme.color.border
                                opacity: 0.5
                                visible: index < page._activePromos.length - 1
                            }
                        }
                    }

                    // Empty state
                    EmptyState {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.space.lg
                        Layout.bottomMargin: Theme.space.lg
                        visible: page._activePromos.length === 0
                        title: "No active discounts"
                        description: "Pick a book above to schedule your first discount."
                        iconName: "local_offer"
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Helpers
    // -------------------------------------------------------------------------
    function _updatePriceDisplay() {
        var idx = _bookCombo.currentIndex
        if (idx >= 0 && idx < page._books.length) {
            var p = page._books[idx].basePrice
            _priceDisplay.text = "$%1".arg(Number(p).toFixed(2))
        } else {
            _priceDisplay.text = "—"
        }
    }

    // -------------------------------------------------------------------------
    //  Calendar popups for the date fields
    // -------------------------------------------------------------------------
    Popup {
        id: _startPicker
        anchors.centerIn: parent
        width: 340
        height: 400
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.md
        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.space.md

            Text {
                text: "Pick start date"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightBold
            }

            CalendarGrid {
                id: _startCal
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialDate: page._parseDate(_fStart.text)
                onDateSelected: function(date) {
                    _fStart.text = page._formatDate(date)
                    _startPicker.close()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "Today"
                    onClicked: {
                        _fStart.text = page._formatDate(new Date())
                        _startPicker.close()
                    }
                }
                SecondaryButton {
                    text: "Close"
                    onClicked: _startPicker.close()
                }
            }
        }
    }

    Popup {
        id: _endPicker
        anchors.centerIn: parent
        width: 340
        height: 400
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.md
        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.space.md

            Text {
                text: "Pick end date"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightBold
            }

            CalendarGrid {
                id: _endCal
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialDate: page._parseDate(_fEnd.text)
                onDateSelected: function(date) {
                    _fEnd.text = page._formatDate(date)
                    _endPicker.close()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "+7 days"
                    onClicked: {
                        var base = page._parseDate(_fStart.text)
                        var end = new Date(base.getTime() + 7 * 24 * 60 * 60 * 1000)
                        _fEnd.text = page._formatDate(end)
                        _endPicker.close()
                    }
                }
                SecondaryButton {
                    text: "+30 days"
                    onClicked: {
                        var base = page._parseDate(_fStart.text)
                        var end = new Date(base.getTime() + 30 * 24 * 60 * 60 * 1000)
                        _fEnd.text = page._formatDate(end)
                        _endPicker.close()
                    }
                }
                SecondaryButton {
                    text: "Close"
                    onClicked: _endPicker.close()
                }
            }
        }
    }
}
