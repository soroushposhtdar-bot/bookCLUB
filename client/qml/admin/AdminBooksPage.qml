// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//   - SearchField.onTextEdited: function(newText) { ... }
//   - SortDropdown.onChanged: function(value) { ... }
//   - removed Overlay.overlay parenting block from openEdit()
//   - rebound _editPopup.x to page.width
//   - rebound _editPopup.y to page.height
//   - rebound _editPopup.width to page.width
//   - rebound _editPopup.height to page.height
//
// FUNCTIONAL FIXES:
//   - Added fallback Connections handlers (onRefreshed, onDataChanged, onDataReady)
//   - Added _delayedRefresh Timer to re-sync after async VM.refresh() completes
//   - Defensive null/undefined checks in _refreshFromVM for malformed book records
//
// LAYOUT FIX (this pass):
//   - Replaced ScrollView > ColumnLayout with a plain ScrollView whose content is a
//     Column — same root pattern used by AdminUsersPage. The ColumnLayout.implicitHeight
//     is 0 on the first paint in Qt 6, so the entire page was invisible until resize.
//   - Books table Card: replaced the fixed height: 400 with
//     height: parent.width > 0 ? 44 + _pageSize * Theme.size.tableRowHeight : 400
//     so the table always shows the configured page worth of rows.

// =============================================================================
//  AdminBooksPage.qml
// =============================================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"
import "../components/book"
import "../components"

Item {
    id: page

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal openBookDetail(string bookId)

    property string _search: ""
    property string _statusFilter: "all"
    property string _publisherFilter: "all"
    property int _currentPage: 1
    readonly property int _pageSize: 8
    readonly property int _totalPages: Math.max(1, Math.ceil(_filteredBooks.count / _pageSize))

    ListModel { id: _allBooks }
    ListModel { id: _filteredBooks }
    ListModel { id: _booksPage }
    ListModel { id: _reviews }

    property var _publishers: []

    property int _retryCount: 0
    onVisibleChanged: {
        if (page.visible) {
            page._retryCount = 0
            if (page.viewModel && _allBooks.count === 0) {
                page.viewModel.refresh()
                Qt.callLater(function() { if (page) page._refreshFromVM() })
            } else if (page.viewModel) {
                page._refreshFromVM()
            }
        }
    }

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

    function _refreshFromVM() {
        if (!page.viewModel) return
        _allBooks.clear()
        const books = page.viewModel.allBooks || []
        const pubSet = {}
        for (let i = 0; i < books.length; ++i) {
            const b = books[i]
            if (!b) continue
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

    function _refreshReviews() {
        if (!page.viewModel) return
        _reviews.clear()
        const reviews = page.viewModel.allReviews || []
        for (let i = 0; i < reviews.length; ++i) {
            const r = reviews[i]
            _reviews.append({
                id:              r.id              || "",
                rating:          Number(r.rating)  || 0,
                username:        r.username        || "",
                comment:         r.comment         || "",
                helpfulCount:    r.helpfulCount    || 0,
                flagged:         r.flagged         || false,
                bookTitle:       r.bookTitle       || "",
                createdAtText:   r.createdAtText   || ""
            })
        }
    }

    property bool _refreshing: false

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        function onBooksChanged() {
            if (!page._refreshing) { page._refreshing = true; page._refreshFromVM(); page._refreshing = false }
        }
        function onReviewsChanged() {
            if (!page._refreshing) { page._refreshing = true; page._refreshReviews(); page._refreshing = false }
        }
        function onRefreshed() {
            if (!page._refreshing) { page._refreshing = true; page._refreshFromVM(); page._refreshReviews(); page._refreshing = false }
        }
        function onDataChanged() {
            if (!page._refreshing) { page._refreshing = true; page._refreshFromVM(); page._refreshReviews(); page._refreshing = false }
        }
        function onDataReady() {
            if (!page._refreshing) { page._refreshing = true; page._refreshFromVM(); page._refreshReviews(); page._refreshing = false }
        }
    }

    Timer {
        id: _deferredReviews
        interval: 1500
        repeat: false
        onTriggered: { if (page.viewModel) page._refreshReviews() }
    }

    Timer {
        id: _delayedRefresh
        interval: 600
        repeat: true
        running: false
        onTriggered: {
            if (!page.viewModel) { _delayedRefresh.stop(); return }
            if (page.viewModel.allBooks && page.viewModel.allBooks.length > 0) {
                page._refreshFromVM()
                _delayedRefresh.stop()
            } else {
                if (page._retryCount === 0) page.viewModel.refresh()
                page._retryCount++
                if (page._retryCount >= 10) _delayedRefresh.stop()
            }
        }
    }

    Component.onDestruction: { if (_deferredReviews.running) _deferredReviews.stop() }

    Component.onCompleted: {
        if (page.viewModel) {
            page._refreshFromVM()
            if (_allBooks.count === 0) _delayedRefresh.start()
            _deferredReviews.start()
        }
    }

    readonly property real _colTitle:     280
    readonly property real _colAuthor:    180
    readonly property real _colPublisher: 180
    readonly property real _colPrice:     100
    readonly property real _colSales:     100
    readonly property real _colRating:    130
    readonly property real _colStatus:    130

    ConfirmDialog { id: _confirmDialog }

    Popup {
        id: _editPopup
        x: (page.width  - _editPopup.width)  / 2
        y: (page.height - _editPopup.height) / 2
        width: Math.min(560, page.width  - 64)
        height: Math.min(560, page.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        property string editingBookId: ""

        background: Rectangle {
            radius: Theme.radius.lg
            color: Theme.color.cardBackground
            Rectangle { anchors.fill: parent; anchors.margins: -4; radius: Theme.radius.lg + 4; color: Theme.color.divider; opacity: 0.3 }
            Rectangle { anchors.fill: parent; anchors.margins: -8; radius: Theme.radius.lg + 8; color: Theme.color.divider; opacity: 0.15 }
        }

        function openEdit(bookId) {
            editingBookId = bookId
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

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.md

            SectionHeader { width: parent.width; title: "Modify book metadata"; subtitle: "Admin edit — changes propagate to every module" }

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

            RowLayout {
                width: parent.width
                spacing: Theme.space.md
                Item { width: 1; Layout.fillWidth: true; height: 1 }
                SecondaryButton { text: "Cancel"; onClicked: _editPopup.close() }
                PrimaryButton {
                    text: "Save changes"; iconName: "check"; enabled: _fTitle.text.length > 0
                    onClicked: {
                        if (!page.viewModel) { page.toastRequested("error", "No view model", "AdminViewModel is not available."); _editPopup.close(); return }
                        page.viewModel.updateBookInfo(_editPopup.editingBookId, _fTitle.text, _fAuthor.text, _fGenre.text, parseFloat(_fPrice.text) || 0.0, _fDesc.text)
                        page.toastRequested("success", "Book updated", "Metadata for the book has been saved.")
                        _editPopup.close()
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  LAYOUT FIX: plain Column with anchors.fill — identical pattern to
    //  AdminUsersPage. The old ScrollView > ColumnLayout combo produced a
    //  zero-height viewport in Qt 6 until a window resize.
    // -------------------------------------------------------------------------
    Column {
        id: _mainColumn
        anchors.fill: parent
        anchors.margins: Theme.space.xl
        spacing: Theme.space.lg

        // ----- Page header -----
        Item {
            width: _mainColumn.width
            height: 56
            Row {
                anchors.fill: parent
                spacing: Theme.space.md
                Rectangle {
                    width: 44; height: 44; radius: 12
                    color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.14)
                    anchors.verticalCenter: parent.verticalCenter
                    AppIcon { anchors.centerIn: parent; name: "library_books"; size: 22; color: Theme.color.accent }
                }
                Column {
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter
                    Text { text: "Books & content"; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold }
                    Text { text: "Inspect, modify, or remove any title in the system"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                }
            }
        }

        // ----- KPI cards row -----
        Row {
            width: _mainColumn.width
            spacing: Theme.space.lg

            StatCard {
                width: (_mainColumn.width - Theme.space.lg * 3) / 4
                height: Theme.size.kpiCardHeight; iconName: "library_books"
                value: (page.viewModel ? page.viewModel.totalBooks : 0).toString()
                label: "Total books"; delta: "Across all publishers"; deltaUp: true; accent: Theme.color.accent
            }
            StatCard {
                width: (_mainColumn.width - Theme.space.lg * 3) / 4
                height: Theme.size.kpiCardHeight; iconName: "star"
                value: (page.viewModel ? page.viewModel.totalReviews : 0).toString()
                label: "Total reviews"; delta: "Monitored"; deltaUp: true; accent: Theme.color.info
            }
            StatCard {
                width: (_mainColumn.width - Theme.space.lg * 3) / 4
                height: Theme.size.kpiCardHeight; iconName: "flag"
                value: (page.viewModel ? page.viewModel.flaggedReviewsCount : 0).toString()
                label: "Flagged reviews"; delta: "Awaiting moderation"; deltaUp: false; accent: Theme.color.warning
            }
            StatCard {
                width: (_mainColumn.width - Theme.space.lg * 3) / 4
                height: Theme.size.kpiCardHeight; iconName: "delete"
                value: {
                    if (!page.viewModel) return "0"
                    let n = 0
                    for (let i = 0; i < _allBooks.count; ++i) {
                        if (_allBooks.get(i).status === "removed") ++n
                    }
                    return n.toString()
                }
                label: "Removed books"; delta: "Soft-deleted"; deltaUp: false; accent: Theme.color.error
            }
        }

        // ----- Search / filter row -----
        Card {
            width: _mainColumn.width
            elevation: "none"
            bordered: true
            padding: Theme.space.lg

            Column {
                width: parent.width
                spacing: Theme.space.sm

                Row {
                    width: parent.width
                    spacing: Theme.space.md

                    SearchField {
                        width: parent.width - 200 - Theme.space.md
                        placeholder: "Search books by title, author, or publisher…"
                        text: page._search
                        onTextEdited: function(newText) { page._search = newText; page._currentPage = 1; page._applyFilter() }
                    }

                    SortDropdown {
                        width: 200
                        options: page._publishers.map(function(p) {
                            return { label: p === "all" ? "All publishers" : p, value: p }
                        })
                        onChanged: function(value) { page._publisherFilter = value; page._currentPage = 1; page._applyFilter() }
                    }
                }

                Row {
                    width: parent.width
                    spacing: Theme.space.sm

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
                            onClicked: { page._statusFilter = modelData.key; page._currentPage = 1; page._applyFilter() }
                        }
                    }
                }
            }
        }

        // ----- Books table -----
        // LAYOUT FIX: Card fills remaining space (after header + KPI + filter + pagination).
        // This guarantees the table body is always visible — the old fixed height:400 was
        // not enough on some screens.
        Card {
            width: _mainColumn.width
            height: _mainColumn.height
                    - 56                         // header
                    - Theme.size.kpiCardHeight   // KPI row
                    - 80                         // filter card (approx)
                    - 40                         // pagination row
                    - Theme.space.lg * 4         // gaps
                    - Theme.space.xl * 2         // column margins
            padding: 0

            Column {
                anchors.fill: parent
                spacing: 0

                // Header row
                Rectangle {
                    id: _booksTableHeader
                    width: parent.width
                    height: 44
                    color: Theme.color.fieldFilled

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.xl
                        anchors.rightMargin: Theme.space.xl
                        spacing: 0

                        Text { width: page._colTitle;     text: "Title";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colAuthor;    text: "Author";    color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colPublisher; text: "Publisher"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colPrice;     text: "Price";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colSales;     text: "Sales";     color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colRating;    text: "Rating";    color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Text { width: page._colStatus;    text: "Status";    color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                        Item { width: 1; height: 1 }
                        Text { width: 176; text: "Actions"; horizontalAlignment: Text.AlignRight; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                    }

                    Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }
                }

                // Body — ListView fills remaining Card height
                ListView {
                    id: _booksListView
                    width: parent.width
                    height: parent.height - _booksTableHeader.height
                    clip: true
                    interactive: true
                    model: _booksPage
                    spacing: 0

                    delegate: Rectangle {
                        width: _booksListView.width
                        height: Theme.size.tableRowHeight
                        color: _rowHover.hovered ? Theme.color.fieldFilled
                             : (index % 2 === 0 ? "transparent" : Theme.color.fieldFilled)

                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

                        HoverHandler { id: _rowHover; cursorShape: Qt.PointingHandCursor }
                        MouseArea { anchors.fill: parent; onClicked: page.openBookDetail(model.id) }

                        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 1; color: Theme.color.divider }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl
                            anchors.rightMargin: Theme.space.xl
                            spacing: 0

                            Row {
                                width: page._colTitle
                                spacing: Theme.space.md
                                anchors.verticalCenter: parent.verticalCenter
                                BookCover { width: 36; height: 50; book: model; anchors.verticalCenter: parent.verticalCenter }
                                Column {
                                    spacing: 2
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { text: model.title; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; elide: Text.ElideRight; width: page._colTitle - 36 - Theme.space.md - 12 }
                                    Text { text: (model.genreIds && model.genreIds.length > 0) ? model.genreIds[0] : "—"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                                }
                            }

                            Text { width: page._colAuthor;    text: model.authorName;    color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colPublisher; text: model.publisherName; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colPrice;     text: model.priceText;     color: Theme.color.textPrimary;   font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }
                            Text { width: page._colSales;     text: Number(model.totalSales || 0).toLocaleString(Qt.locale(), "f", 0); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; anchors.verticalCenter: parent.verticalCenter; elide: Text.ElideRight }

                            Item {
                                width: page._colRating
                                height: parent.height
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 4
                                    RatingStars { size: 12; rating: model.averageRating }
                                    Text {
                                        text: "%1 (%2)".arg(Number(model.averageRating || 0).toFixed(1)).arg(Number(model.ratingCount || 0).toLocaleString(Qt.locale(), "f", 0))
                                        color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            Item {
                                width: page._colStatus
                                height: parent.height
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: Theme.space.xs
                                    Rectangle { width: 8; height: 8; radius: 4; color: page._statusColor(model.status); anchors.verticalCenter: parent.verticalCenter }
                                    Text { text: page._statusLabel(model.status); color: page._statusColor(model.status); font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; anchors.verticalCenter: parent.verticalCenter }
                                }
                            }

                            Item { width: 1; height: 1 }

                            Row {
                                width: 176
                                spacing: Theme.space.xs
                                layoutDirection: Qt.RightToLeft
                                anchors.verticalCenter: parent.verticalCenter

                                IconButton {
                                    iconName: "delete"; iconColor: Theme.color.error; hoverIconColor: Theme.color.error
                                    onClicked: {
                                        _confirmDialog.openDialog({
                                            title: "Remove book?",
                                            message: "Soft-delete '" + model.title + "' (" + model.id + ").",
                                            detail: "The book will be hidden from the storefront but can be re-published later.",
                                            iconName: "delete_forever", confirmLabel: "Remove", confirmStyle: "danger",
                                            confirmedCb: function() {
                                                if (page.viewModel && typeof page.viewModel.deleteBook === "function") {
                                                    page.viewModel.deleteBook(model.id, "Admin policy violation")
                                                    page.toastRequested("warning", "Book removed", "'" + model.title + "' has been removed from the storefront.")
                                                }
                                            }
                                        })
                                    }
                                }
                                IconButton { iconName: "edit"; onClicked: _editPopup.openEdit(model.id) }
                            }
                        }
                    }

                    EmptyState {
                        anchors.centerIn: parent
                        width: parent.width
                        height: 200
                        visible: _booksPage.count === 0
                        iconName: "library_books"
                        title: "No books found"
                        description: "Try adjusting the search or filter."
                    }
                }
            }
        }

        // ----- Pagination + count -----
        Row {
            width: _mainColumn.width
            height: 40
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
            Item { width: _mainColumn.width - 400; height: 1 }
            Pagination {
                currentPage: page._currentPage
                totalPages: page._totalPages
                onPageRequested: function(pageNum) { page._currentPage = pageNum; page._applyPage() }
            }
        }
    }
}
