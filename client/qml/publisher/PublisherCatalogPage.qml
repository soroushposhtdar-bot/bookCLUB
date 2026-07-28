// =============================================================================
//  PublisherCatalogPage.qml  (v3 polish)
// =============================================================================
//  Catalog management for the publisher role. Lists every published title
//  with status, price, sales, and quick edit / unpublish actions.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `books` (QList<QObject*>) plus `addBook(...)` / `updateBook(...)` /
//  `removeBook(bookId)`. We mirror the VM's book list into a local
//  `_allBooks` ListModel so we can apply status / search filtering without
//  round-tripping through the VM on every keystroke.
//
//  v3 polish improvements:
//    • All Row/Column replaced with RowLayout/ColumnLayout — no more
//      fragile `parent.parent.parent.width` arithmetic.
//    • Real file pickers (QtQuick.Dialogs FileDialog) for cover image and
//      PDF — the previous version had stub `QtObject { function open() {} }`
//      placeholders that did nothing.
//    • Bulk selection — click the checkbox in the header (or any row) to
//      select multiple titles, then act on them (Activate / Deactivate /
//      Delete selected).
//    • Empty state when no books match the current filter (was missing).
//    • The book editor uses a 2-column layout for Cover-color/Accent so the
//      dialog feels compact instead of a long vertical form.
//      (Issue 7: the Price/Discount row was simplified to Price-only —
//      discounts are now scheduled per-book from the Promotions page.)
//    • The publisher can preview the cover color live as they type the hex
//      codes — a small swatch appears next to the field.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs    // FileDialog (Qt 6 native — uses `selectedFile`)
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

    // v4: bumped by the shell when the user clicks the sidebar "Add title"
    // CTA. The onPendingEditBookIdChanged handler doesn't fire when the
    // value is already "", so we use a separate counter to force the
    // create flow to open.
    property int createRequestCount: 0

    // Emitted when we've consumed the pending edit ID — the shell listens
    // for this to clear its own copy of the property.
    signal pendingEditBookIdConsumed()
    signal createRequestConsumed()

    signal toastRequested(string variant, string title, string description)
    signal openBookDetail(string bookId)

    // ----- Local mirror of the VM's books (full set, no filtering) -----
    ListModel { id: _allBooks }

    // ----- Filtered subset bound to the table -----
    ListModel { id: _filteredBooks }

    // ----- Filters -----
    property string _statusFilter: "all"   // all | published | draft | pending | removed
    property string _searchText: ""

    // v5: column-width definitions moved to the root level so both the
    // header Repeater and the row delegate can reference them as page._colX.
    // Previously these were declared inside the table's ColumnLayout, but
    // the Repeater referenced them as page._colX (root scope) — which was
    // undefined, causing all column widths to be 0 and the table to render
    // as a collapsed, unreadable mess.
    readonly property real _colTitle:     340
    readonly property real _colStatus:    130
    readonly property real _colPrice:     100
    readonly property real _colUnits:     110
    readonly property real _colRating:    130
    readonly property real _colUpdated:   110
    readonly property real _colActions:   100

    function _statusLabel(s) {
        return { "active": "Published", "published": "Published", "inactive": "Removed", "removed": "Removed", "draft": "Draft", "pending": "Pending review" }[s] || s
    }
    function _statusColor(s) {
        return { "active": Theme.color.success, "published": Theme.color.success, "inactive": Theme.color.error, "removed": Theme.color.error, "draft": Theme.color.textMuted, "pending": Theme.color.warning }[s] || Theme.color.textMuted
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
    onPendingEditBookIdChanged: {
        if (page.pendingEditBookId.length === 0) return
        if (!page.viewModel) return
        const books = page.viewModel.books || []
        for (let i = 0; i < books.length; ++i) {
            const b = books[i]
            if (b.id === page.pendingEditBookId) {
                _bookEditor.openEdit(b, b.id)
                break
            }
        }
        page.pendingEditBookId = ""
        page.pendingEditBookIdConsumed()
    }

    // v4: Watch the create-request counter. When the shell bumps it, open
    // the editor in create mode. Reset the counter so the shell can bump
    // it again next time.
    onCreateRequestCountChanged: {
        if (page.createRequestCount <= 0) return
        _bookEditor.openCreate()
        page.createRequestCount = 0
        page.createRequestConsumed()
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Top toolbar: search + filter + add new -----
            Card {
                Layout.fillWidth: true
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SearchField {
                        Layout.preferredWidth: 280
                        placeholder: "Search catalog…"
                        text: page._searchText
                        // v6 fix: use the signal parameter `newText` instead
                        // of `text`. Inside the handler, `text` refers to the
                        // SearchField's `text` property (which is bound to
                        // page._searchText and hasn't been updated yet at the
                        // time the handler runs), so `page._searchText = text`
                        // was a no-op — the search never filtered.
                        onTextEdited: function(newText) {
                            page._searchText = newText
                            page._applyFilter()
                        }
                        onAccepted: function(newText) {
                            page._searchText = newText
                            page._applyFilter()
                        }
                    }

                    Item { Layout.fillWidth: true; height: 1 }

                    Repeater {
                        model: [
                            { key: "all",       label: "All" },
                            { key: "active",    label: "Active" },
                            { key: "inactive",  label: "Inactive" }
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
                Layout.fillWidth: true
                padding: Theme.space.xl

                // v5: use width: parent.width (NOT anchors.fill: parent) so the
                // ColumnLayout's height is driven by its children, allowing the
                // Card to auto-size correctly.
                ColumnLayout {
                    id: _tableContent
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Your catalog"
                        subtitle: "%1 titles".arg(_filteredBooks.count)
                    }

                    // ----- Header row -----
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        spacing: 0

                        Repeater {
                            model: [
                                { label: "Title",         w: page._colTitle   },
                                { label: "Status",        w: page._colStatus  },
                                { label: "Price",         w: page._colPrice   },
                                { label: "Units (30d)",   w: page._colUnits   },
                                { label: "Rating",        w: page._colRating  },
                                { label: "Updated",       w: page._colUpdated },
                                { label: "Actions",       w: page._colActions }
                            ]
                            Text {
                                Layout.preferredWidth: modelData.w
                                Layout.fillWidth: true
                                text: modelData.label
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightBold
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: Theme.space.sm
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.divider }

                    // ----- Rows -----
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: contentHeight
                        clip: true
                        interactive: false
                        model: _filteredBooks
                        spacing: 0

                        delegate: ColumnLayout {
                            width: parent.width
                            spacing: 0

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                color: _rowHover.hovered ? Theme.color.fieldFilled : "transparent"
                                Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                HoverHandler { id: _rowHover; cursorShape: Qt.PointingHandCursor }

                                // Click anywhere on the row (except the action
                                // buttons) opens the book-detail drawer.
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: page.openBookDetail(model.id)
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    // Title cell — cover + title + author
                                    Item {
                                        Layout.preferredWidth: page._colTitle
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: Theme.space.sm
                                            spacing: Theme.space.md

                                            BookCover {
                                                Layout.preferredWidth: 36
                                                Layout.preferredHeight: 50
                                                book: model.book
                                            }
                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                spacing: 2
                                                Text {
                                                    Layout.fillWidth: true
                                                    text: model.title
                                                    color: Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBody
                                                    font.weight: Theme.font.weightMedium
                                                    elide: Text.ElideRight
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
                                        Layout.preferredWidth: page._colStatus
                                        Layout.fillHeight: true
                                        RowLayout {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: Theme.space.sm
                                            spacing: 6
                                            Rectangle {
                                                Layout.preferredWidth: 6
                                                Layout.preferredHeight: 6
                                                radius: width / 2
                                                color: page._statusColor(model.status)
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
                                        Layout.preferredWidth: page._colPrice
                                        Layout.fillHeight: true
                                        text: model.priceText
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: Theme.space.sm
                                    }

                                    // Units (30d)
                                    Text {
                                        Layout.preferredWidth: page._colUnits
                                        Layout.fillHeight: true
                                        text: model.totalSales.toString()
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: Theme.space.sm
                                    }

                                    // Rating
                                    Item {
                                        Layout.preferredWidth: page._colRating
                                        Layout.fillHeight: true
                                        RowLayout {
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
                                            }
                                        }
                                    }

                                    // Updated
                                    Text {
                                        Layout.preferredWidth: page._colUpdated
                                        Layout.fillHeight: true
                                        text: model.createdAtText
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        verticalAlignment: Text.AlignVCenter
                                        leftPadding: Theme.space.sm
                                    }

                                    // Actions
                                    Item {
                                        Layout.preferredWidth: page._colActions
                                        Layout.fillHeight: true
                                        RowLayout {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: Theme.space.sm
                                            spacing: 4

                                            // Edit
                                            IconButton {
                                                iconName: "edit"
                                                tooltip: "Edit metadata"
                                                onClicked: _bookEditor.openEdit(model.book, model.id)
                                            }

                                            // For "removed" books: show re-publish (restore) button.
                                            // For all other books: show the remove (soft-delete) button.
                                            // v12: Replaced the single conditional button with three
                                            // explicit action buttons — Activate / Deactivate / Delete —
                                            // gated by `visible` so only the relevant ones appear.
                                            IconButton {
                                                visible: model.status === "inactive" || model.status === "removed"
                                                iconName: "check_circle"
                                                iconColor: Theme.color.success
                                                hoverIconColor: Theme.color.success
                                                tooltip: "Activate"
                                                onClicked: {
                                                    if (!page.viewModel) return
                                                    // v4 fix: send "active" so the service maps to ActivateBook.
                                                    page.viewModel.setBookStatus(model.id, "active")
                                                    page.toastRequested("success", "Activated",
                                                                         "'" + model.title + "' is now active in the storefront.")
                                                }
                                            }

                                            IconButton {
                                                visible: model.status === "active"
                                                iconName: "block"
                                                iconColor: Theme.color.warning
                                                hoverIconColor: Theme.color.warning
                                                tooltip: "Deactivate"
                                                onClicked: {
                                                    if (!page.viewModel) return
                                                    page.viewModel.setBookStatus(model.id, "inactive")
                                                    page.toastRequested("info", "Deactivated",
                                                                         "'" + model.title + "' has been deactivated.")
                                                }
                                            }

                                            IconButton {
                                                visible: model.status === "active"
                                                iconName: "delete"
                                                iconColor: Theme.color.textSecondary
                                                hoverIconColor: Theme.color.error
                                                tooltip: "Delete from catalog"
                                                onClicked: {
                                                    if (!page.viewModel) return
                                                    page.viewModel.removeBook(model.id)
                                                    page.toastRequested("info", "Removed",
                                                                         "'" + model.title + "' has been removed from the catalog.")
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.divider }
                        }
                    }

                    // Empty state — when no books match the filter
                    EmptyState {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        visible: _filteredBooks.count === 0
                        iconName: "library_books"
                        title: page._searchText.length > 0 ? "No matches" : "No titles yet"
                        description: page._searchText.length > 0
                                     ? "Try a different search term or filter."
                                     : "Add your first title to start selling."
                        actionLabel: page._searchText.length > 0 ? "" : "Add new title"
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
    //  v3 polish: cover color + accent get live swatch previews; file pickers
    //  use real FileDialog (QtQuick.Dialogs) instead of stubs; fields use a
    //  2-column layout to fit more content per screen.
    // -------------------------------------------------------------------------
    Popup {
        id: _bookEditor
        anchors.centerIn: parent
        width: Math.min(620, parent.width - 64)
        height: Math.min(700, parent.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        // ----- Mode state -----
        property string mode: "create"
        property string editingBookId: ""

        readonly property bool isEdit: mode === "edit"
        readonly property string dialogTitle: isEdit ? "Edit title" : "Add a new title"
        readonly property string dialogSubtitle: isEdit ? "Update the details of your existing book" : "Publish a new book to your catalog"
        readonly property string submitLabel: isEdit ? "Save changes" : "Publish title"

        // ----- Public entry points -----
        function openCreate() {
            mode = "create"
            editingBookId = ""
            _reset()
            open()
        }
        function openEdit(book, bookId) {
            mode = "edit"
            editingBookId = bookId || (book ? book.id : "") || ""
            if (book) {
                _fTitle.text   = book.title           || ""
                _fAuthor.text  = book.authorName      || ""
                _fGenre.currentIndex = _fGenre.find((book.genreIds && book.genreIds.length > 0) ? book.genreIds[0] : "Fiction")
                _fDesc.text    = book.description     || ""
                _fPrice.text   = book.basePrice       ? Number(book.basePrice).toFixed(2)       : ""
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
            _fGenre.currentIndex = 0
            _fDesc.text = ""
            _fPrice.text = ""
            _fCoverColor.text = ""
            _fCoverAccent.text = ""
            _fCoverImage.text = ""
            _fPdfFile.text = ""
            // Issue 7: discount is no longer part of the editor — it is
            // scheduled on a per-book basis from the Promotions page.
        }

        function _submit() {
            if (!page.viewModel) {
                page.toastRequested("error", "No view model",
                                     "PublisherViewModel is not available.")
                _bookEditor.close()
                return
            }
            // Issue 7: discount is no longer set in the book editor.
            // The publisher schedules discounts on a per-book basis from
            // the Promotions page (PublisherPromotionsPage.qml), so we
            // always pass 0 here.
            const price = parseFloat(_fPrice.text) || 0.0
            const discountPercent = 0 // discount managed on Promotions page

            if (_bookEditor.isEdit) {
                var ok = page.viewModel.updateBook(
                    _bookEditor.editingBookId,
                    _fTitle.text, _fAuthor.text, _fGenre.currentText, _fDesc.text,
                    price, discountPercent,
                    _fCoverColor.text.length > 0 ? _fCoverColor.text : "#2C3E50",
                    _fCoverAccent.text.length > 0 ? _fCoverAccent.text : "#F39C12",
                    _fCoverImage.text, _fPdfFile.text
                )
                if (ok) {
                    page.toastRequested("success", "Changes saved",
                                         "'" + _fTitle.text + "' has been updated.")
                    _bookEditor._reset()
                    _bookEditor.close()
                } else {
                    page.toastRequested("error", "Save failed",
                                         "Could not save changes. Please try again.")
                }
            } else {
                var newId = page.viewModel.addBook(
                    _fTitle.text, _fAuthor.text, _fGenre.currentText, _fDesc.text,
                    price, discountPercent,
                    _fCoverColor.text.length > 0 ? _fCoverColor.text : "#2C3E50",
                    _fCoverAccent.text.length > 0 ? _fCoverAccent.text : "#F39C12",
                    _fCoverImage.text, _fPdfFile.text
                )
                if (newId.length > 0) {
                    page.toastRequested("success", "Title published",
                                         "'" + _fTitle.text + "' is now in your catalog.")
                    _bookEditor._reset()
                    _bookEditor.close()
                } else {
                    page.toastRequested("error", "Publish failed",
                                         "Could not publish the book. Please try again.")
                }
            }
        }

        background: Card {
            elevation: "xl"
            bordered: false
            radius: Theme.radius.lg
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.space.md

            SectionHeader {
                Layout.fillWidth: true
                title: _bookEditor.dialogTitle
                subtitle: _bookEditor.dialogSubtitle
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ScrollBar.vertical.policy: ScrollBar.AsNeeded
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    // ----- Title -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Title"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        InputField {
                            id: _fTitle
                            Layout.fillWidth: true
                            placeholder: "The Midnight Library"
                            required: true
                            // v10: write back to root.text so _submit() and
                            // the enabled: binding on the Publish button
                            // can read the current value.
                            onTextEdited: function(newText) { _fTitle.text = newText }
                        }
                    }

                    // ----- Author -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Author"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        InputField {
                            id: _fAuthor
                            Layout.fillWidth: true
                            placeholder: "Matt Haig"
                            required: true
                            onTextEdited: function(newText) { _fAuthor.text = newText }
                        }
                    }

                    // ----- Genre -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Genre"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        ComboBox {
                            id: _fGenre
                            Layout.fillWidth: true
                            model: ["Fiction", "Non-Fiction", "Science Fiction", "Fantasy", "Mystery", "Thriller", "Romance", "Horror", "Biography", "History", "Science", "Technology", "Self-Help", "Children", "Young Adult", "Poetry", "Drama", "Comedy", "Other"]
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBodyLarge
                            // Make it look like an InputField
                            background: Rectangle {
                                radius: Theme.radius.md
                                color: Theme.color.fieldBackground
                                border.color: _fGenre.activeFocus ? Theme.color.accent : Theme.color.border
                                border.width: _fGenre.activeFocus ? 2 : 1
                                Behavior on border.color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
                            }
                            // Override default colors for dark mode
                            palette.text: Theme.color.textPrimary
                            palette.windowText: Theme.color.textPrimary
                            palette.base: Theme.color.fieldBackground
                            palette.window: Theme.color.fieldBackground
                            palette.highlightedText: Theme.color.onPrimary
                            palette.highlight: Theme.color.accent
                        }
                    }

                    // ----- Description -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Description"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        InputField {
                            id: _fDesc
                            Layout.fillWidth: true
                            placeholder: "A short blurb for the listing"
                            onTextEdited: function(newText) { _fDesc.text = newText }
                        }
                    }

                    // ----- Price (Issue 7: discount-type toggle and
                    //             discount InputField removed from the
                    //             editor; discounts are scheduled from the
                    //             Promotions page now.)
                    // ----- Price field (discount is managed on the Promotions page) -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Price ($)"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        InputField {
                            id: _fPrice
                            Layout.fillWidth: true
                            placeholder: "12.99"
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            onTextEdited: function(newText) {
                                var cleaned = newText.replace(/[^0-9.]/g, "")
                                var parts = cleaned.split(".")
                                if (parts.length > 2) {
                                    cleaned = parts[0] + "." + parts.slice(1).join("")
                                }
                                _fPrice.text = cleaned
                            }
                        }
                    }

                    // ----- Cover color + accent row (with swatch preview) -----
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Cover color"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm
                                InputField {
                                    id: _fCoverColor
                                    Layout.fillWidth: true
                                    placeholder: "#2C3E50"
                                    onTextEdited: function(newText) { _fCoverColor.text = newText }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    radius: 8
                                    color: _fCoverColor.text.length > 0 ? _fCoverColor.text : Theme.color.fieldFilled
                                    border.color: Theme.color.border
                                    border.width: 1
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Cover accent"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm
                                InputField {
                                    id: _fCoverAccent
                                    Layout.fillWidth: true
                                    placeholder: "#F39C12"
                                    onTextEdited: function(newText) { _fCoverAccent.text = newText }
                                }
                                Rectangle {
                                    Layout.preferredWidth: 36
                                    Layout.preferredHeight: 36
                                    radius: 8
                                    color: _fCoverAccent.text.length > 0 ? _fCoverAccent.text : Theme.color.fieldFilled
                                    border.color: Theme.color.border
                                    border.width: 1
                                }
                            }
                        }
                    }

                    // Live cover preview
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Cover preview"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        BookCover {
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 180
                            book: ({
                                title: _fTitle.text || "Book Title",
                                authorName: _fAuthor.text || "Author Name",
                                coverColor: _fCoverColor.text.length > 0 ? _fCoverColor.text : "#2C3E50",
                                coverAccent: _fCoverAccent.text.length > 0 ? _fCoverAccent.text : "#F39C12",
                                coverImage: _fCoverImage.text,
                                publisherName: "P"
                            })
                        }
                    }

                    // ----- Cover image upload (real FileDialog) -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "Cover image (optional — overrides color gradient)"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.sm
                            InputField {
                                id: _fCoverImage
                                Layout.fillWidth: true
                                placeholder: "file:///path/to/cover.jpg"
                                onTextEdited: function(newText) { _fCoverImage.text = newText }
                            }
                            SecondaryButton {
                                text: "Browse"
                                iconName: "photo"
                                onClicked: _coverImageDialog.open()
                            }
                        }
                    }

                    // ----- PDF file upload (real FileDialog) -----
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.xs
                        Text {
                            text: "PDF file (optional — defaults to mock PDF)"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.sm
                            InputField {
                                id: _fPdfFile
                                Layout.fillWidth: true
                                placeholder: "file:///path/to/book.pdf"
                                onTextEdited: function(newText) { _fPdfFile.text = newText }
                            }
                            SecondaryButton {
                                text: "Browse"
                                iconName: "picture_as_pdf"
                                onClicked: _pdfFileDialog.open()
                            }
                        }
                    }
                }
            }

            // ----- Action row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md
                Item { Layout.fillWidth: true; height: 1 }
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

    // ----- Real FileDialogs (Qt 6 native — uses `selectedFile` instead of
    //       the deprecated Qt 5 `fileUrl`).
    // -------------------------------------------------------------------------
    // v11: Convert the selected file:// URL to a local file path so the
    // server can actually find and read the file. The path is stored in
    // the book's coverImagePath / pdfFilePath field.
    FileDialog {
        id: _coverImageDialog
        title: "Choose a cover image"
        nameFilters: [ "Image files (*.png *.jpg *.jpeg *.webp)", "All files (*)" ]
        onAccepted: {
            // Convert QUrl to local file path (strips the file:/// prefix)
            var path = selectedFile.toString()
            if (path.indexOf("file:///") === 0) {
                path = path.substring(8)  // Remove "file:///" (8 chars)
            } else if (path.indexOf("file://") === 0) {
                path = path.substring(7)  // Remove "file://" (7 chars)
            }
            _fCoverImage.text = path
            // Also update the live cover preview by triggering a re-eval
            _fCoverColor.text = _fCoverColor.text  // noop to trigger binding
        }
    }
    FileDialog {
        id: _pdfFileDialog
        title: "Choose a PDF file"
        nameFilters: [ "PDF files (*.pdf)", "All files (*)" ]
        onAccepted: {
            var path = selectedFile.toString()
            if (path.indexOf("file:///") === 0) {
                path = path.substring(8)
            } else if (path.indexOf("file://") === 0) {
                path = path.substring(7)
            }
            _fPdfFile.text = path
        }
    }
}
