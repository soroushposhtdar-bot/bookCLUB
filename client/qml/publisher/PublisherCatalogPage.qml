// =============================================================================
//  PublisherCatalogPage.qml
// =============================================================================
//  Catalog management for the publisher role. Lists every published title
//  with status, price, sales, and quick edit / unpublish actions.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `books` (QList<QObject*>) plus `addBook(...)` / `updateBook(...)` /
//  `removeBook(bookId)`. We mirror the VM's book list into a local
//  `_allBooks` ListModel so we can apply status / search filtering without
//  round-tripping through the VM on every keystroke. Whenever the VM's
//  `books` property changes (refresh, add, update, remove) we re-seed
//  `_allBooks` from the VM and re-apply the active filter.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs as Dialogs
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
    id: page
    anchors.fill: parent

    property var viewModel: null   // PublisherViewModel

    // Set by the shell when the user clicks "Edit metadata" in the book-detail
    // drawer. We watch it via onPendingEditBookIdChanged below and open the
    // editor in edit mode for that book.
    property string pendingEditBookId: ""

    signal toastRequested(string variant, string title, string description)
    signal openBookDetail(string bookId)   // emitted when a row is clicked

    // ----- Local mirror of the VM's books (full set, no filtering) -----
    ListModel { id: _allBooks }

    // ----- Filtered subset bound to the table -----
    ListModel { id: _filteredBooks }

    // ----- Filters -----
    property string _statusFilter: "all"   // all | published | draft | pending | removed
    property string _searchText: ""

    function _statusLabel(s) {
        return { "published": "Published", "draft": "Draft", "pending": "Pending review", "removed": "Removed" }[s] || s
    }
    function _statusColor(s) {
        return { "published": Theme.color.success, "draft": Theme.color.textMuted, "pending": Theme.color.warning, "removed": Theme.color.error }[s] || Theme.color.textMuted
    }

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshFromVM() {
        if (!page.viewModel) return
        _allBooks.clear()
        const books = page.viewModel.books || []
        for (let i = 0; i < books.length; ++i) {
            const b = books[i]
            _allBooks.append({
                id:              b.id              || "",
                title:           b.title           || "",
                authorName:      b.authorName      || "",
                priceText:       b.priceText       || "",
                totalSales:      b.totalSales      || 0,
                averageRating:   b.averageRating   || 0,
                ratingCount:     b.ratingCount     || 0,
                createdAtText:   b.createdAtText   || "",
                coverColor:      b.coverColor      || Theme.color.primary,
                coverAccent:     b.coverAccent     || Theme.color.accent,
                status:          b.status          || "published",
                genreIds:        b.genreIds        || [],
                description:     b.description     || "",
                basePrice:       b.basePrice       || 0,
                discountPercent: b.discountPercent || 0,
                book:            b
            })
        }
        page._applyFilter()
    }

    function _applyFilter() {
        _filteredBooks.clear()
        const status = page._statusFilter
        const q = page._searchText.trim().toLowerCase()
        for (let i = 0; i < _allBooks.count; ++i) {
            const row = _allBooks.get(i)
            if (status !== "all" && row.status !== status) continue
            if (q.length > 0) {
                const hay = (row.title + " " + row.authorName).toLowerCase()
                if (hay.indexOf(q) < 0) continue
            }
            _filteredBooks.append(row)
        }
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onBooksChanged: page._refreshFromVM()
    }

    Component.onCompleted: page._refreshFromVM()

    // ----- Watch for pending-edit requests from the book-detail drawer -----
    //   When the shell sets pendingEditBookId to a non-empty value, we find
    //   the matching book in _allBooks and open the editor in edit mode.
    //   After opening, we clear the property so the same book can be edited
    //   again later without re-triggering.
    onPendingEditBookIdChanged: {
        if (page.pendingEditBookId.length === 0) return
        // Find the book in the VM's books list.
        if (!page.viewModel) return
        const books = page.viewModel.books || []
        for (let i = 0; i < books.length; ++i) {
            const b = books[i]
            if (b.id === page.pendingEditBookId) {
                _bookEditor.openEdit(b, b.id)
                break
            }
        }
        // Clear the pending ID so the same book can be re-edited.
        page.pendingEditBookId = ""
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Top toolbar: search + filter + add new -----
            Card {
                width: parent.width
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                Row {
                    width: parent.width
                    spacing: Theme.space.md

                    SearchField {
                        width: 280
                        placeholder: "Search catalog…"
                        text: page._searchText
                        onTextEdited: {
                            page._searchText = text
                            page._applyFilter()
                        }
                        onAccepted: {
                            page._searchText = text
                            page._applyFilter()
                            page.toastRequested("info", "Search",
                                                 "Filtered catalog by '" + text + "'")
                        }
                    }

                    Item { width: 1; Layout.fillWidth: true; height: 1 }

                    Repeater {
                        model: [
                            { key: "all",       label: "All" },
                            { key: "published", label: "Published" },
                            { key: "draft",     label: "Drafts" },
                            { key: "pending",   label: "Pending" },
                            { key: "removed",   label: "Removed" }
                        ]
                        GenreChip {
                            label: modelData.label
                            selected: page._statusFilter === modelData.key
                            onClicked: {
                                page._statusFilter = modelData.key
                                page._applyFilter()
                            }
                        }
                    }

                    PrimaryButton {
                        text: "Add new title"
                        iconName: "add"
                        onClicked: _bookEditor.openCreate()
                    }
                }
            }

            // ----- Catalog table -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Your catalog"
                        subtitle: "%1 titles".arg(_filteredBooks.count)
                    }

                    // ----- Header row -----
                    Row {
                        width: parent.width
                        height: 36
                        spacing: 0

                        Repeater {
                            model: [
                                { label: "Title",         w: 0.34 },
                                { label: "Status",        w: 0.14 },
                                { label: "Price",         w: 0.10 },
                                { label: "Units (30d)",   w: 0.12 },
                                { label: "Rating",        w: 0.12 },
                                { label: "Updated",       w: 0.10 },
                                { label: "Actions",       w: 0.08 }
                            ]
                            Text {
                                width: parent.parent.width * modelData.w
                                text: modelData.label
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightBold
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: Theme.space.sm
                            }
                        }
                    }

                    Rectangle { width: parent.width; height: 1; color: Theme.color.divider }

                    // ----- Rows -----
                    ListView {
                        width: parent.width
                        height: contentHeight
                        clip: true
                        interactive: false
                        model: _filteredBooks
                        spacing: 0

                        delegate: Column {
                            width: parent.width

                            Rectangle {
                                width: parent.width
                                height: 72
                                color: _rowHover.hovered ? Theme.color.fieldFilled : "transparent"
                                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                                HoverHandler { id: _rowHover; cursorShape: Qt.PointingHandCursor }

                                // Click anywhere on the row (except the action
                                // buttons) opens the book-detail drawer.
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: page.openBookDetail(model.id)
                                }

                            Row {
                                width: parent.width
                                height: 72
                                spacing: 0

                                // Title cell — cover + title + author
                                Item {
                                    width: parent.parent.width * 0.34
                                    height: parent.height
                                    Row {
                                        anchors.fill: parent
                                        anchors.leftMargin: Theme.space.sm
                                        spacing: Theme.space.md

                                        BookCover {
                                            width: 36; height: 50
                                            book: model.book
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Column {
                                            spacing: 2
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                text: model.title
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                font.weight: Theme.font.weightMedium
                                                elide: Text.ElideRight
                                                width: parent.parent.parent.width * 0.34 - 36 - Theme.space.md - Theme.space.sm * 2 - 12
                                            }
                                            Text {
                                                text: model.authorName
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                            }
                                        }
                                    }
                                }

                                // Status cell
                                Item {
                                    width: parent.parent.width * 0.14
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.space.sm
                                        spacing: 6
                                        Rectangle {
                                            width: 6; height: 6; radius: 3
                                            color: page._statusColor(model.status)
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: page._statusLabel(model.status)
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                        }
                                    }
                                }

                                // Price cell
                                Text {
                                    width: parent.parent.width * 0.10
                                    height: parent.height
                                    text: model.priceText
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: Theme.space.sm
                                }

                                // Units (30d) — synthesize from totalSales
                                Text {
                                    width: parent.parent.width * 0.12
                                    height: parent.height
                                    text: model.totalSales.toString()
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: Theme.space.sm
                                }

                                // Rating
                                Item {
                                    width: parent.parent.width * 0.12
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.space.sm
                                        spacing: 4
                                        RatingStars { size: 12; rating: model.averageRating }
                                        Text {
                                            text: model.averageRating.toFixed(1)
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                // Updated (use createdAtText as a stand-in)
                                Text {
                                    width: parent.parent.width * 0.10
                                    height: parent.height
                                    text: model.createdAtText
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: Theme.space.sm
                                }

                                // Actions
                                Item {
                                    width: parent.parent.width * 0.08
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.space.sm
                                        spacing: 4

                                        // Edit (always available — opens the dialog in "edit" mode)
                                        IconButton {
                                            iconName: "edit"
                                            onClicked: _bookEditor.openEdit(model.book, model.id)
                                        }

                                        // For "removed" books: show re-publish (restore) button.
                                        // For all other books: show the remove (soft-delete) button.
                                        IconButton {
                                            iconName: model.status === "removed" ? "history" : "delete"
                                            iconColor: model.status === "removed" ? Theme.color.success : Theme.color.textSecondary
                                            hoverIconColor: model.status === "removed" ? Theme.color.success : Theme.color.error
                                            onClicked: {
                                                if (!page.viewModel) return
                                                if (model.status === "removed") {
                                                    // Re-publish: flip status back to "published".
                                                    page.viewModel.setBookStatus(model.id, "published")
                                                    page.toastRequested("success", "Re-published",
                                                                         "'" + model.title + "' is back in the storefront.")
                                                } else {
                                                    // Soft-delete: marks the book as "removed".
                                                    page.viewModel.removeBook(model.id)
                                                    page.toastRequested("info", "Removed",
                                                                         "'" + model.title + "' has been removed from the catalog.")
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            }   // ← closes the hover Rectangle wrapper

                            Rectangle { width: parent.width; height: 1; color: Theme.color.divider }
                        }
                    }

                    // Empty state (kept here even though the catalog always has rows)
                    EmptyState {
                        width: parent.width
                        height: 200
                        visible: _filteredBooks.count === 0
                        iconName: "library_books"
                        title: "No titles yet"
                        description: "Add your first title to start selling."
                        actionLabel: "Add new title"
                        onActionTriggered: _bookEditor.openCreate()
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Book editor — modal popup for creating OR editing a title.
    //
    //  Two modes:
    //    • open()         → "create" mode: empty fields, calls addBook(...)
    //    • openEdit(book) → "edit" mode: pre-fills from an existing BookDto,
    //                       calls updateBook(bookId, ...)
    //
    //  The dialog tracks its own mode + editing bookId via internal properties.
    // -------------------------------------------------------------------------
    Popup {
        id: _bookEditor
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 64)
        height: Math.min(640, parent.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        // ----- Mode state -----
        property string mode: "create"   // "create" | "edit"
        property string editingBookId: ""

        readonly property bool isEdit: mode === "edit"
        readonly property string dialogTitle: isEdit ? "Edit title" : "Add a new title"
        readonly property string dialogSubtitle: isEdit ? "Update the details of your existing book" : "Publish a new book to your catalog"
        readonly property string submitLabel: isEdit ? "Save changes" : "Publish title"

        // ----- Public entry points -----
        function openCreate() {
            // "create" mode — reset all fields and show.
            mode = "create"
            editingBookId = ""
            _reset()
            open()
        }
        function openEdit(book, bookId) {
            // "edit" mode — pre-fill fields from the existing book.
            mode = "edit"
            editingBookId = bookId || (book ? book.id : "") || ""
            if (book) {
                _fTitle.text   = book.title           || ""
                _fAuthor.text  = book.authorName      || ""
                _fGenre.text   = (book.genreIds && book.genreIds.length > 0) ? book.genreIds[0] : ""
                _fDesc.text    = book.description     || ""
                _fPrice.text   = book.basePrice       ? Number(book.basePrice).toFixed(2)       : ""
                _fDiscount.text= book.discountPercent ? String(book.discountPercent)            : "0"
                _fCoverColor.text   = book.coverColor  || ""
                _fCoverAccent.text  = book.coverAccent || ""
                _fCoverImage.text   = book.coverImage  || ""
                _fPdfFile.text      = book.pdfFilePath || ""
            } else {
                _reset()
            }
            open()
        }

        function _reset() {
            _fTitle.text = ""
            _fAuthor.text = ""
            _fGenre.text = ""
            _fDesc.text = ""
            _fPrice.text = ""
            _fDiscount.text = ""
            _fCoverColor.text = ""
            _fCoverAccent.text = ""
            _fCoverImage.text = ""
            _fPdfFile.text = ""
        }

        function _submit() {
            if (!page.viewModel) {
                page.toastRequested("error", "No view model",
                                     "PublisherViewModel is not available.")
                _bookEditor.close()
                return
            }
            if (_bookEditor.isEdit) {
                // Edit mode → call updateBook(bookId, ...)
                page.viewModel.updateBook(
                    _bookEditor.editingBookId,
                    _fTitle.text,
                    _fAuthor.text,
                    _fGenre.text,
                    _fDesc.text,
                    parseFloat(_fPrice.text) || 0.0,
                    parseInt(_fDiscount.text) || 0,
                    _fCoverColor.text.length > 0 ? _fCoverColor.text : "#2C3E50",
                    _fCoverAccent.text.length > 0 ? _fCoverAccent.text : "#F39C12",
                    _fCoverImage.text,
                    _fPdfFile.text
                )
                page.toastRequested("success", "Changes saved",
                                     "'" + _fTitle.text + "' has been updated.")
            } else {
                // Create mode → call addBook(...)
                page.viewModel.addBook(
                    _fTitle.text,
                    _fAuthor.text,
                    _fGenre.text,
                    _fDesc.text,
                    parseFloat(_fPrice.text) || 0.0,
                    parseInt(_fDiscount.text) || 0,
                    _fCoverColor.text.length > 0 ? _fCoverColor.text : "#2C3E50",
                    _fCoverAccent.text.length > 0 ? _fCoverAccent.text : "#F39C12",
                    _fCoverImage.text,
                    _fPdfFile.text
                )
                page.toastRequested("success", "Title published",
                                     "'" + _fTitle.text + "' is now in your catalog.")
            }
            _bookEditor._reset()
            _bookEditor.close()
        }

        background: Card {
            elevation: "xl"
            bordered: false
            radius: Theme.radius.lg
        }

        Column {
            anchors.fill: parent
            spacing: Theme.space.md

            SectionHeader {
                width: parent.width
                title: _bookEditor.dialogTitle
                subtitle: _bookEditor.dialogSubtitle
            }

            ScrollView {
                width: parent.width
                height: parent.height - 60
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text { text: "Title"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                    InputField { id: _fTitle; width: parent.width; placeholder: "The Midnight Library" }

                    Text { text: "Author"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _fAuthor; width: parent.width; placeholder: "Matt Haig" }

                    Text { text: "Genre"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _fGenre; width: parent.width; placeholder: "Fiction" }

                    Text { text: "Description"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _fDesc; width: parent.width; placeholder: "A short blurb for the listing" }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Price ($)"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fPrice; width: parent.width; placeholder: "12.99" }
                        }

                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Discount %"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fDiscount; width: parent.width; placeholder: "0" }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Cover color"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fCoverColor; width: parent.width; placeholder: "#2C3E50" }
                        }

                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Cover accent"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fCoverAccent; width: parent.width; placeholder: "#F39C12" }
                        }
                    }

                    // ----- Cover image upload (spec §3-2a) -----
                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Column {
                            width: parent.width
                            spacing: Theme.space.sm
                            Text { text: "Cover image (optional — overrides color gradient)"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                InputField {
                                    id: _fCoverImage
                                    width: parent.width - 100 - Theme.space.sm
                                    placeholder: "file:///path/to/cover.jpg"
                                    text: ""
                                }
                                SecondaryButton {
                                    width: 100
                                    text: "Browse"
                                    onClicked: _coverImageDialog.open()
                                }
                            }
                        }
                    }

                    // ----- PDF file upload (spec §3-2a) -----
                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Column {
                            width: parent.width
                            spacing: Theme.space.sm
                            Text { text: "PDF file (optional — defaults to mock PDF)"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                InputField {
                                    id: _fPdfFile
                                    width: parent.width - 100 - Theme.space.sm
                                    placeholder: "file:///path/to/book.pdf"
                                    text: ""
                                }
                                SecondaryButton {
                                    width: 100
                                    text: "Browse"
                                    onClicked: _pdfFileDialog.open()
                                }
                            }
                        }
                    }
                }
            }

            Row {
                width: parent.width
                spacing: Theme.space.md

                Item { width: 1; Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "Cancel"
                    onClicked: _bookEditor.close()
                }
                PrimaryButton {
                    text: _bookEditor.submitLabel
                    iconName: "check"
                    enabled: _fTitle.text.length > 0 && _fAuthor.text.length > 0
                    onClicked: _bookEditor._submit()
                }
            }
        }
    }

    // ----- Cover image file picker -----
    Dialogs.FileDialog {
        id: _coverImageDialog
        title: "Choose cover image"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.webp *.gif)"]
        onAccepted: {
            if (_coverImageDialog.selectedFile) {
                _fCoverImage.text = _coverImageDialog.selectedFile.toString()
            }
        }
    }

    // ----- PDF file picker -----
    Dialogs.FileDialog {
        id: _pdfFileDialog
        title: "Choose PDF file"
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: {
            if (_pdfFileDialog.selectedFile) {
                _fPdfFile.text = _pdfFileDialog.selectedFile.toString()
            }
        }
    }
}
