// =============================================================================
//  AdminBooksPage.qml
// =============================================================================
//  Book & content management for the admin role (spec §4-3).
//
//  Capabilities (mirroring the spec):
//    • View all books in the system (every status, every publisher)
//    • Inspect a specific book's info + reviews in a side drawer
//    • Delete inappropriate books (soft-delete via MockDataStore.setBookStatus)
//    • Modify book info (title / author / genre / price / description)
//    • Monitor reviews posted by users (approve / delete)
//
//  Data source: page.viewModel (AdminViewModel). The VM exposes
//  `allBooks` (QVariantList) + `totalBooks` (int) + `allReviews` +
//  `flaggedReviewsCount` + `deleteBook(bookId, reason)` +
//  `updateBookInfo(bookId, ...)` + `bookDetails(bookId)` +
//  `reviewsForBook(bookId)` + `deleteReview(reviewId)` +
//  `approveReview(reviewId)`.
//
//  We mirror the VM's book list into a local `_allBooks` ListModel so we can
//  apply search / status / publisher filtering without round-tripping through
//  the VM on every keystroke. The reviews monitor (bottom card) binds
//  directly to the VM's `allReviews` property — every review across every
//  book in one place.
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

Item {
    id: page

    // ----- AdminViewModel (injected by AdminShell) -----
    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal openBookDetail(string bookId)   // emitted when a row is clicked

    // ----- Filters / pagination state -----
    property string _search: ""
    property string _statusFilter: "all"   // all | published | draft | pending | removed
    property string _publisherFilter: "all"
    property int _currentPage: 1
    readonly property int _pageSize: 8
    readonly property int _totalPages: Math.max(1, Math.ceil(_filteredBooks.count / _pageSize))

    // ----- Local mirrors of the VM's books -----
    ListModel { id: _allBooks }
    ListModel { id: _filteredBooks }
    ListModel { id: _booksPage }

    // ----- Distinct publisher names (for the publisher filter dropdown) -----
    property var _publishers: []

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

    // -------------------------------------------------------------------------
    //  VM → local ListModel sync
    // -------------------------------------------------------------------------
    function _refreshFromVM() {
        if (!page.viewModel) return
        _allBooks.clear()
        const books = page.viewModel.allBooks || []
        const pubSet = {}
        for (let i = 0; i < books.length; ++i) {
            const b = books[i]
            _allBooks.append({
                id:             b.id             || "",
                title:          b.title          || "",
                authorName:     b.authorName     || "",
                publisherName:  b.publisherName  || "",
                priceText:      b.priceText      || "",
                price:          b.price          || 0,
                averageRating:  b.averageRating  || 0,
                ratingCount:    b.ratingCount    || 0,
                totalSales:     b.totalSales     || 0,
                status:         b.status         || "published",
                active:         b.active         !== false,
                createdAtText:  b.createdAtText  || "",
                coverColor:     b.coverColor     || Theme.color.primary,
                coverAccent:    b.coverAccent    || Theme.color.accent,
                description:    b.description    || "",
                genreIds:       b.genreIds       || []
            })
            if (b.publisherName && !pubSet[b.publisherName]) pubSet[b.publisherName] = true
        }
        // Rebuild the publisher filter list ("all" + sorted unique names).
        const pubs = Object.keys(pubSet).sort()
        page._publishers = ["all"].concat(pubs)
        page._applyFilter()
    }

    function _applyFilter() {
        _filteredBooks.clear()
        const q = page._search.trim().toLowerCase()
        const status = page._statusFilter
        const pub = page._publisherFilter
        for (let i = 0; i < _allBooks.count; ++i) {
            const row = _allBooks.get(i)
            if (status !== "all" && row.status !== status) continue
            if (pub !== "all" && row.publisherName !== pub) continue
            if (q.length > 0) {
                const hay = (row.title + " " + row.authorName + " " + row.publisherName).toLowerCase()
                if (hay.indexOf(q) < 0) continue
            }
            _filteredBooks.append(row)
        }
        // Clamp page
        const totalPages = Math.max(1, Math.ceil(_filteredBooks.count / page._pageSize))
        if (page._currentPage > totalPages) page._currentPage = totalPages
        page._applyPage()
    }

    function _applyPage() {
        _booksPage.clear()
        const start = (page._currentPage - 1) * page._pageSize
        const end = Math.min(start + page._pageSize, _filteredBooks.count)
        for (let i = start; i < end; ++i) _booksPage.append(_filteredBooks.get(i))
    }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onBooksChanged: page._refreshFromVM()
    }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshFromVM()
            if (typeof page.viewModel.refresh === "function") {
                page.viewModel.refresh()
            }
        }
    }

    // ----- Column widths -----
    readonly property real _colTitle:     280
    readonly property real _colAuthor:    180
    readonly property real _colPublisher: 180
    readonly property real _colPrice:     100
    readonly property real _colSales:     100
    readonly property real _colRating:    130
    readonly property real _colStatus:    130

    // ----- Confirmation dialog for destructive actions -----
    ConfirmDialog { id: _confirmDialog }

    // ----- Book-edit popup (admin edits book metadata) -----
    Popup {
        id: _editPopup
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 64)
        height: Math.min(560, parent.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        property string editingBookId: ""

        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        function openEdit(bookId) {
            editingBookId = bookId
            // Find the book in _allBooks to pre-fill the fields.
            for (let i = 0; i < _allBooks.count; ++i) {
                const b = _allBooks.get(i)
                if (b.id === bookId) {
                    _fTitle.text  = b.title
                    _fAuthor.text = b.authorName
                    _fGenre.text  = b.genreIds && b.genreIds.length > 0 ? b.genreIds[0] : ""
                    _fPrice.text  = b.price ? Number(b.price).toFixed(2) : ""
                    _fDesc.text   = b.description
                    break
                }
            }
            open()
        }

        Column {
            anchors.fill: parent
            spacing: Theme.space.md

            SectionHeader {
                width: parent.width
                title: "Modify book metadata"
                subtitle: "Admin edit — changes propagate to every module"
            }

            ScrollView {
                width: parent.width
                height: parent.height - 130
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text { text: "Title"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                    InputField { id: _fTitle; width: parent.width; placeholder: "Book title" }

                    Text { text: "Author"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _fAuthor; width: parent.width; placeholder: "Author name" }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md
                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Genre"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fGenre; width: parent.width; placeholder: "Fiction" }
                        }
                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Price ($)"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _fPrice; width: parent.width; placeholder: "12.99" }
                        }
                    }

                    Text { text: "Description"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _fDesc; width: parent.width; placeholder: "Short blurb" }
                }
            }

            Row {
                width: parent.width
                spacing: Theme.space.md
                Item { width: 1; Layout.fillWidth: true; height: 1 }
                SecondaryButton { text: "Cancel"; onClicked: _editPopup.close() }
                PrimaryButton {
                    text: "Save changes"
                    iconName: "check"
                    enabled: _fTitle.text.length > 0
                    onClicked: {
                        if (!page.viewModel) {
                            page.toastRequested("error", "No view model", "AdminViewModel is not available.")
                            _editPopup.close()
                            return
                        }
                        page.viewModel.updateBookInfo(
                            _editPopup.editingBookId,
                            _fTitle.text,
                            _fAuthor.text,
                            _fGenre.text,
                            parseFloat(_fPrice.text) || 0.0,
                            _fDesc.text
                        )
                        page.toastRequested("success", "Book updated",
                                            "Metadata for the book has been saved.")
                        _editPopup.close()
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "library_books"
                    value: (page.viewModel ? page.viewModel.totalBooks : 0).toString()
                    label: "Total books"
                    delta: "Across all publishers"
                    deltaUp: true
                    accent: Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "star"
                    value: (page.viewModel ? page.viewModel.totalReviews : 0).toString()
                    label: "Total reviews"
                    delta: "Monitored"
                    deltaUp: true
                    accent: Theme.color.info
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "flag"
                    value: (page.viewModel ? page.viewModel.flaggedReviewsCount : 0).toString()
                    label: "Flagged reviews"
                    delta: "Awaiting moderation"
                    deltaUp: false
                    accent: Theme.color.warning
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "delete"
                    value: {
                        if (!page.viewModel) return "0"
                        const books = page.viewModel.allBooks || []
                        let n = 0
                        for (let i = 0; i < books.length; ++i) {
                            if (books[i].status === "removed") ++n
                        }
                        return n.toString()
                    }
                    label: "Removed books"
                    delta: "Soft-deleted"
                    deltaUp: false
                    accent: Theme.color.error
                }
            }

            // ----- Search / status filter / publisher filter row -----
            Card {
                width: parent.width
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                Row {
                    width: parent.width
                    spacing: Theme.space.md

                    SearchField {
                        width: Math.min(420, parent.width * 0.45)
                        placeholder: "Search books by title, author, or publisher…"
                        text: page._search
                        onTextEdited: {
                            page._search = newText
                            page._currentPage = 1
                            page._applyFilter()
                        }
                    }

                    SortDropdown {
                        width: 200
                        options: page._publishers.map(function(p) {
                            return { label: p === "all" ? "All publishers" : p, value: p }
                        })
                        onChanged: {
                            page._publisherFilter = value
                            page._currentPage = 1
                            page._applyFilter()
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
                        FilterChip {
                            label: modelData.label
                            iconName: page._statusFilter === modelData.key ? "check" : ""
                            onClicked: {
                                page._statusFilter = modelData.key
                                page._currentPage = 1
                                page._applyFilter()
                            }
                        }
                    }
                }
            }

            // ----- Books table -----
            Card {
                width: parent.width
                padding: 0

                Column {
                    anchors.fill: parent
                    spacing: 0

                    // Header row
                    Rectangle {
                        width: parent.width
                        height: 44
                        color: Theme.color.fieldFilled

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Text { width: page._colTitle;     text: "Title";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colAuthor;    text: "Author";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colPublisher; text: "Publisher";  color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colPrice;     text: "Price";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colSales;     text: "Sales";      color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colRating;    text: "Rating";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colStatus;    text: "Status";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            Text { width: 176; text: "Actions"; horizontalAlignment: Text.AlignRight; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.color.divider
                        }
                    }

                    // Body
                    ListView {
                        width: parent.width
                        height: Math.max(0, _booksPage.count) * 72
                        clip: true
                        interactive: false
                        model: _booksPage
                        spacing: 0

                        delegate: Rectangle {
                            width: parent.width
                            height: 72
                            color: _rowHover.hovered ? Theme.color.fieldFilled
                                 : (index % 2 === 0 ? "transparent" : Theme.color.fieldFilled)

                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                            HoverHandler {
                                id: _rowHover
                                cursorShape: Qt.PointingHandCursor
                            }

                            // Row click → open the book-detail drawer
                            MouseArea {
                                anchors.fill: parent
                                onClicked: page.openBookDetail(model.id)
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: Theme.color.divider
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.space.xl
                                anchors.rightMargin: Theme.space.xl
                                spacing: 0

                                // Title + cover
                                Row {
                                    width: page._colTitle
                                    spacing: Theme.space.md
                                    anchors.verticalCenter: parent.verticalCenter

                                    BookCover {
                                        width: 36; height: 50
                                        book: model
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
                                            width: page._colTitle - 36 - Theme.space.md - 12
                                        }
                                        Text {
                                            text: (model.genreIds && model.genreIds.length > 0) ? model.genreIds[0] : "—"
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }
                                }

                                Text { width: page._colAuthor;    text: model.authorName;    color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colPublisher; text: model.publisherName; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colPrice;     text: model.priceText;     color: Theme.color.textPrimary;   font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                                Text { width: page._colSales;     text: (model.totalSales || 0).toLocaleString(Qt.locale(), "f", 0); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

                                // Rating
                                Item {
                                    width: page._colRating
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 4
                                        RatingStars { size: 12; rating: model.averageRating }
                                        Text {
                                            text: "%1 (%2)".arg(model.averageRating.toFixed(1)).arg((model.ratingCount || 0).toLocaleString(Qt.locale(), "f", 0))
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                // Status
                                Item {
                                    width: page._colStatus
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: Theme.space.xs
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: page._statusColor(model.status)
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: page._statusLabel(model.status)
                                            color: page._statusColor(model.status)
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                // Row actions: view reviews, edit, delete
                                Row {
                                    width: 176
                                    spacing: Theme.space.xs
                                    layoutDirection: Qt.RightToLeft
                                    anchors.verticalCenter: parent.verticalCenter

                                    IconButton {
                                        iconName: "delete"
                                        iconColor: Theme.color.error
                                        hoverIconColor: Theme.color.error
                                        onClicked: {
                                            _confirmDialog.openDialog({
                                                title: "Remove book?",
                                                message: "Soft-delete '" + model.title + "' (" + model.id + ").",
                                                detail: "The book will be hidden from the storefront but can be re-published later.",
                                                iconName: "delete_forever",
                                                confirmLabel: "Remove",
                                                confirmStyle: "danger",
                                                onConfirmed: function() {
                                                    if (page.viewModel && typeof page.viewModel.deleteBook === "function") {
                                                        page.viewModel.deleteBook(model.id, "Admin policy violation")
                                                        page.toastRequested("warning", "Book removed",
                                                                            "'" + model.title + "' has been removed from the storefront.")
                                                    }
                                                }
                                            })
                                        }
                                    }
                                    IconButton {
                                        iconName: "edit"
                                        onClicked: _editPopup.openEdit(model.id)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Pagination + count -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                Text {
                    text: {
                        const total = _filteredBooks.count
                        if (total === 0) return "No books"
                        const start = (page._currentPage - 1) * page._pageSize + 1
                        const end = Math.min(page._currentPage * page._pageSize, total)
                        return "Showing " + start + "–" + end + " of " + total + " books"
                    }
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item { width: 1; Layout.fillWidth: true; height: 1 }
                Pagination {
                    currentPage: page._currentPage
                    totalPages: page._totalPages
                    onPageRequested: function(pageNum) {
                        page._currentPage = pageNum
                        page._applyPage()
                    }
                }
            }

            // ----- Review monitor (spec §4-3: monitor reviews posted by users) -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Review monitor"
                        subtitle: "All reviews across every book — approve or remove in one click"
                    }

                    // Reviews list — bound directly to viewModel.allReviews.
                    // Previously interactive:false + a capped height meant
                    // reviews beyond the 420px viewport were silently clipped
                    // and unreachable. Fixed by enabling interactive scrolling
                    // so all reviews are accessible.
                    ListView {
                        width: parent.width
                        height: Math.min(420, Math.max(0, (page.viewModel && page.viewModel.allReviews ? page.viewModel.allReviews.length : 0)) * 80)
                        clip: true
                        interactive: true
                        model: page.viewModel ? (page.viewModel.allReviews || []) : []
                        spacing: Theme.space.sm

                        delegate: Rectangle {
                            width: parent.width
                            height: _revCol.implicitHeight + 2 * Theme.space.md
                            radius: Theme.radius.md
                            color: Theme.color.fieldFilled
                            border.color: modelData.flagged ? Theme.color.warning : Theme.color.divider
                            border.width: modelData.flagged ? 2 : 1

                            Column {
                                id: _revCol
                                anchors.fill: parent
                                anchors.margins: Theme.space.md
                                spacing: Theme.space.sm

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    RatingStars { size: 12; rating: modelData.rating }

                                    Text {
                                        text: "on <b>" + modelData.bookTitle + "</b>"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        textFormat: Text.RichText
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: "by @" + modelData.username + " · " + (modelData.createdAtText || "")
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    // Flagged badge
                                    Rectangle {
                                        visible: modelData.flagged
                                        width: _flagLbl.implicitWidth + 16
                                        height: 22
                                        radius: 11
                                        color: Theme.color.warningSoft
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            id: _flagLbl
                                            anchors.centerIn: parent
                                            text: "FLAGGED"
                                            color: Theme.color.warning
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeMicro2
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                }

                                Text {
                                    width: parent.width
                                    text: "\"" + (modelData.comment || "") + "\""
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Text {
                                        text: "▲ " + (modelData.helpfulCount || 0) + " helpful"
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    SecondaryButton {
                                        text: "Approve"
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.approveReview === "function") {
                                                page.viewModel.approveReview(modelData.id)
                                                page.toastRequested("success", "Review approved",
                                                                    "Review by @" + modelData.username + " cleared.")
                                            }
                                        }
                                    }
                                    PrimaryButton {
                                        text: "Remove"
                                        iconName: "delete"
                                        onClicked: {
                                            if (page.viewModel && typeof page.viewModel.deleteReview === "function") {
                                                page.viewModel.deleteReview(modelData.id)
                                                page.toastRequested("warning", "Review removed",
                                                                    "Review by @" + modelData.username + " was deleted.")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    EmptyState {
                        width: parent.width
                        height: 180
                        visible: page.viewModel && page.viewModel.allReviews && page.viewModel.allReviews.length === 0
                        iconName: "rate_review"
                        title: "No reviews yet"
                        description: "Reviews posted by users will appear here for monitoring."
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
