// =============================================================================
//  SearchPage.qml
// =============================================================================
//  Advanced search + filters page — full rebuild.
//
//  Layout (top → bottom):
//      1. Sticky search bar — SearchField + filter-toggle IconButton +
//         SortDropdown. Always visible while scrolling.
//      2. Suggestions dropdown — appears when query.length >= 2 and the
//         viewModel has suggestions. Clicking a suggestion calls
//         selectSuggestion().
//      3. Recent + Popular searches — horizontal chip rows shown only when
//         the query is empty.
//      4. Active filter chips — Flow of FilterChip components + "Clear all"
//         TextButton. Visible when activeFilterCount > 0.
//      5. Collapsible filter panel — animated height 0 ↔ auto:
//           • Search-in field selector (4 GenreChips)
//           • Genres grid (multi-select with "Clear" link)
//           • Price range — two Sliders (min/max) with labels
//           • Min rating — 5 star selector buttons (0-5)
//           • Toggles row — onlyFree, onlyPaid, onlyDiscounted,
//             onlyDownloaded, onlyFavorite (GenreChip toggle each)
//           • Availability — 3 GenreChips (All / In stock / Out of stock)
//           • Reset all button
//      6. Results header — "N results" + view toggle (grid/list).
//      7. Results grid — 5-column responsive grid of BookCards.
//
//  States:
//      • Loading  → 8 skeleton BookCards in grid layout
//      • Empty    → EmptyIllustration "No books found" + "Browse all"
//      • Error    → ErrorState with retry
//      • Results  → grid of BookCards
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/book"
import "../components/data"
import "../components/selection"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // SearchViewModel

    signal bookDetailRequested(string bookId)

    // ----- Layout constants -----
    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property int _gridColumns: root.width < 760 ? 2
                                       : root.width < 1100 ? 3
                                       : root.width < 1400 ? 4 : 5

    // ----- Internal state -----
    property bool _filtersExpanded: false
    property string _viewMode: "grid"   // "grid" | "list"

    // -------------------------------------------------------------------------
    //  Page background
    // -------------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // -------------------------------------------------------------------------
    //  Scrollable content
    // -------------------------------------------------------------------------
    Flickable {
        id: _flickable
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

            // -----------------------------------------------------------------
            //  1. Sticky search bar
            // -----------------------------------------------------------------
            Item {
                id: _searchBar
                width: parent.width
                height: 56
                z: Theme.z.sticky

                // Background that becomes opaque + bordered when scrolled
                Rectangle {
                    anchors.fill: parent
                    color: Theme.color.pageBackground
                    border.color: _flickable.contentY > 4 ? Theme.color.divider : "transparent"
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
                }

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.space.md

                    SearchField {
                        id: _search
                        width: parent.width - _filterToggle.width - _sort.width - 2 * Theme.space.md
                        height: 48
                        placeholder: "Search by title, author, or publisher…"
                        text: root.viewModel ? root.viewModel.query : ""
                        onTextEdited: {
                            if (root.viewModel) {
                                root.viewModel.setQuery(newText)
                                _suggestionsDropdown.visible = newText.length >= 2
                            }
                        }
                        onAccepted: {
                            if (root.viewModel) {
                                _suggestionsDropdown.visible = false
                                root.viewModel.search()
                            }
                        }
                    }

                    IconButton {
                        id: _filterToggle
                        iconName: "tune"
                        iconColor: root._filtersExpanded ? Theme.color.accent : Theme.color.textSecondary
                        hoverIconColor: Theme.color.accent
                        width: 48
                        height: 48
                        onClicked: root._filtersExpanded = !root._filtersExpanded

                        // Active-filter indicator dot.
                        Rectangle {
                            visible: root.viewModel && root.viewModel.activeFilterCount > 0 && !root._filtersExpanded
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 8
                            anchors.rightMargin: 8
                            width: 8
                            height: 8
                            radius: 4
                            color: Theme.color.accent
                            border.color: Theme.color.pageBackground
                            border.width: 1.5
                        }
                    }

                    SortDropdown {
                        id: _sort
                        width: 220
                        height: 36
                        anchors.verticalCenter: parent.verticalCenter
                        options: [
                            { label: "Relevance", value: "relevance" },
                            { label: "Price ↑",    value: "price_asc" },
                            { label: "Price ↓",    value: "price_desc" },
                            { label: "Top rated",  value: "rating" },
                            { label: "Newest",     value: "newest" }
                        ]
                        currentValue: root.viewModel ? root.viewModel.sort : "relevance"
                        onChanged: function(value) {
                            if (root.viewModel) {
                                root.viewModel.sort = value
                                root.viewModel.search()
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  2. Suggestions dropdown
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: _suggestionsDropdown.visible ? _suggestionsDropdown.height : 0
                visible: _suggestionsDropdown.visible
                z: Theme.z.sticky + 1

                Card {
                    id: _suggestionsDropdown
                    visible: false
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    elevation: "md"
                    padding: Theme.space.sm

                    height: _suggestionsColumn.implicitHeight + 2 * Theme.space.sm

                    Column {
                        id: _suggestionsColumn
                        width: parent.width
                        spacing: 0

                        Repeater {
                            model: root.viewModel && _suggestionsDropdown.visible ? root.viewModel.suggestions : []
                            delegate: Item {
                                width: parent.width
                                height: 40

                                Rectangle {
                                    anchors.fill: parent
                                    color: _sgMa.containsMouse ? Theme.color.fieldFilled : "transparent"
                                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.space.md
                                    anchors.rightMargin: Theme.space.md
                                    spacing: Theme.space.sm

                                    AppIcon {
                                        name: "search"
                                        size: 16
                                        color: Theme.color.textMuted
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        text: modelData.label || modelData
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                        elide: Text.ElideRight
                                        width: parent.width - 16 - Theme.space.sm
                                    }
                                }

                                MouseArea {
                                    id: _sgMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.selectSuggestion(modelData.value || modelData)
                                            _suggestionsDropdown.visible = false
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  3. Recent + Popular searches (only when query is empty)
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: _discoveryColumn.implicitHeight
                visible: root.viewModel && root.viewModel.query.length === 0

                Column {
                    id: _discoveryColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.lg

                    // Recent
                    Column {
                        width: parent.width
                        spacing: Theme.space.sm

                        Row {
                            width: parent.width
                            spacing: Theme.space.sm

                            Text {
                                text: "Recent"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightSemibold
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Item { width: 1; height: 1; Layout.fillWidth: true }

                            TextButton {
                                text: "Clear"
                                iconName: "delete_outline"
                                visible: root.viewModel && root.viewModel.recentSearches.length > 0
                                onClicked: {
                                    if (root.viewModel) root.viewModel.clearRecentSearches()
                                }
                            }
                        }

                        Flickable {
                            width: parent.width
                            height: 38
                            contentWidth: _recentRow.implicitWidth
                            contentHeight: height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            Row {
                                id: _recentRow
                                spacing: Theme.space.sm

                                Repeater {
                                    model: root.viewModel ? root.viewModel.recentSearches : []
                                    delegate: GenreChip {
                                        label: modelData
                                        iconName: "history"
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.setQuery(modelData)
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Popular
                    Column {
                        width: parent.width
                        spacing: Theme.space.sm

                        Text {
                            text: "Popular"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightSemibold
                        }

                        Flickable {
                            width: parent.width
                            height: 38
                            contentWidth: _popularRow.implicitWidth
                            contentHeight: height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            flickableDirection: Flickable.HorizontalFlick

                            Row {
                                id: _popularRow
                                spacing: Theme.space.sm

                                Repeater {
                                    model: root.viewModel ? root.viewModel.popularSearches : []
                                    delegate: GenreChip {
                                        label: modelData
                                        iconName: "trending_up"
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.setQuery(modelData)
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  4. Active filter chips
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: _activeFiltersColumn.implicitHeight
                visible: root.viewModel && root.viewModel.activeFilterCount > 0

                Column {
                    id: _activeFiltersColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: Theme.space.sm

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Text {
                            text: "%1 filter%2 applied".arg(root.viewModel ? root.viewModel.activeFilterCount : 0)
                                                       .arg((root.viewModel && root.viewModel.activeFilterCount === 1) ? "" : "s")
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightMedium
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Item { width: 1; height: 1; Layout.fillWidth: true }

                        TextButton {
                            text: "Clear all"
                            iconName: "filter_list_off"
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.clearFilters()
                                    root.viewModel.search()
                                }
                            }
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: Theme.space.sm

                        Repeater {
                            model: root.viewModel ? root.viewModel.activeFilters : []
                            delegate: FilterChip {
                                label: modelData.label
                                iconName: modelData.iconName || "filter_alt"
                                onRemoveClicked: {
                                    if (root.viewModel) {
                                        root.viewModel.clearFilter(modelData.key)
                                        root.viewModel.search()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  5. Collapsible filter panel
            // -----------------------------------------------------------------
            Item {
                id: _filterPanelWrapper
                width: parent.width
                height: root._filtersExpanded ? _filterPanel.height + Theme.space.lg : 0
                visible: height > 0
                clip: true

                Behavior on height {
                    NumberAnimation {
                        duration: Theme.motion.durationSlow
                        easing.type: Easing.OutCubic
                    }
                }

                Card {
                    id: _filterPanel
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    elevation: "sm"
                    padding: Theme.space.xl
                    height: _filterPanelContent.implicitHeight + 2 * Theme.space.xl

                    Column {
                        id: _filterPanelContent
                        width: parent.width
                        spacing: Theme.space.lg

                        // Header
                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                            AppIcon {
                                name: "tune"
                                size: 22
                                color: Theme.color.textPrimary
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: "Refine results"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Item { width: 1; height: 1; Layout.fillWidth: true }

                            IconButton {
                                iconName: "close"
                                iconColor: Theme.color.textSecondary
                                hoverIconColor: Theme.color.textPrimary
                                onClicked: root._filtersExpanded = false
                            }
                        }

                        // ----- Search-in field selector -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Text {
                                text: "Search in"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }

                            Row {
                                spacing: Theme.space.sm
                                Repeater {
                                    model: [
                                        { key: "all",       label: "Everything" },
                                        { key: "title",     label: "Title" },
                                        { key: "author",    label: "Author" },
                                        { key: "publisher", label: "Publisher" }
                                    ]
                                    delegate: GenreChip {
                                        label: modelData.label
                                        selected: root.viewModel && root.viewModel.field === modelData.key
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.setField(modelData.key)
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Genres grid -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: "Genres"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Item { width: 1; height: 1; Layout.fillWidth: true }

                                TextButton {
                                    text: "Clear"
                                    visible: root.viewModel && root.viewModel.selectedGenres.length > 0
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.clearGenres()
                                            root.viewModel.search()
                                        }
                                    }
                                }
                            }

                            Grid {
                                width: parent.width
                                columns: root.width < 760 ? 2 : 4
                                spacing: Theme.space.sm

                                Repeater {
                                    model: root.viewModel ? root.viewModel.availableGenres : []
                                    delegate: GenreChip {
                                        label: modelData
                                        selected: root.viewModel && root.viewModel.isGenreSelected(modelData)
                                        width: (parent.width - (parent.columns - 1) * parent.spacing) / parent.columns
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.toggleGenre(modelData)
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Price range (min/max) -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: "Price range"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Item { width: 1; height: 1; Layout.fillWidth: true }

                                Text {
                                    text: "$%1 – $%2".arg(root.viewModel ? root.viewModel.minPrice.toFixed(0) : "0")
                                                   .arg(root.viewModel ? root.viewModel.maxPrice.toFixed(0) : "100")
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightSemibold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            Row {
                                width: parent.width
                                spacing: Theme.space.lg

                                // Min slider
                                Column {
                                    width: (parent.width - Theme.space.lg) / 2
                                    spacing: 4

                                    Slider {
                                        width: parent.width
                                        from: 0
                                        to: root.viewModel ? root.viewModel.maxPrice : 100
                                        value: root.viewModel ? root.viewModel.minPrice : 0
                                        onMoved: {
                                            if (root.viewModel) {
                                                root.viewModel.minPrice = value
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }

                                // Max slider
                                Column {
                                    width: (parent.width - Theme.space.lg) / 2
                                    spacing: 4

                                    Slider {
                                        width: parent.width
                                        from: root.viewModel ? root.viewModel.minPrice : 0
                                        to: 100
                                        value: root.viewModel ? root.viewModel.maxPrice : 100
                                        onMoved: {
                                            if (root.viewModel) {
                                                root.viewModel.maxPrice = value
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Min rating -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                Text {
                                    text: "Minimum rating"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Item { width: 1; height: 1; Layout.fillWidth: true }

                                TextButton {
                                    text: "Any"
                                    visible: root.viewModel && root.viewModel.minRating > 0
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.minRating = 0
                                            root.viewModel.search()
                                        }
                                    }
                                }
                            }

                            Row {
                                spacing: Theme.space.xs
                                Repeater {
                                    model: [1, 2, 3, 4, 5]
                                    delegate: Item {
                                        width: 40
                                        height: 40

                                        Rectangle {
                                            anchors.fill: parent
                                            radius: Theme.radius.md
                                            color: root.viewModel && root.viewModel.minRating >= modelData
                                                   ? Theme.color.warning
                                                   : _ratingMa.containsMouse ? Theme.color.fieldFilled
                                                   : "transparent"
                                            border.color: root.viewModel && root.viewModel.minRating >= modelData
                                                          ? Theme.color.warning
                                                          : Theme.color.border
                                            border.width: 1
                                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                                            Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }
                                        }

                                        AppIcon {
                                            anchors.centerIn: parent
                                            name: "star"
                                            size: 18
                                            color: root.viewModel && root.viewModel.minRating >= modelData
                                                   ? Theme.color.textInverse
                                                   : Theme.color.textMuted
                                        }

                                        MouseArea {
                                            id: _ratingMa
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (root.viewModel) {
                                                    // Click the same star again → clear
                                                    var v = root.viewModel.minRating === modelData ? modelData - 1 : modelData
                                                    root.viewModel.minRating = v
                                                    root.viewModel.search()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Toggles row -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Text {
                                text: "Show only"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }

                            Row {
                                spacing: Theme.space.sm

                                GenreChip {
                                    label: "Free"
                                    iconName: "local_offer"
                                    selected: root.viewModel && root.viewModel.onlyFree
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.onlyFree = !root.viewModel.onlyFree
                                            root.viewModel.search()
                                        }
                                    }
                                }

                                GenreChip {
                                    label: "Paid"
                                    iconName: "payments"
                                    selected: root.viewModel && root.viewModel.onlyPaid
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.onlyPaid = !root.viewModel.onlyPaid
                                            root.viewModel.search()
                                        }
                                    }
                                }

                                GenreChip {
                                    label: "Discounted"
                                    iconName: "percent"
                                    selected: root.viewModel && root.viewModel.onlyDiscounted
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.onlyDiscounted = !root.viewModel.onlyDiscounted
                                            root.viewModel.search()
                                        }
                                    }
                                }

                                GenreChip {
                                    label: "Downloaded"
                                    iconName: "download"
                                    selected: root.viewModel && root.viewModel.onlyDownloaded
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.onlyDownloaded = !root.viewModel.onlyDownloaded
                                            root.viewModel.search()
                                        }
                                    }
                                }

                                GenreChip {
                                    label: "Favorites"
                                    iconName: "favorite_border"
                                    selected: root.viewModel && root.viewModel.onlyFavorite
                                    onClicked: {
                                        if (root.viewModel) {
                                            root.viewModel.onlyFavorite = !root.viewModel.onlyFavorite
                                            root.viewModel.search()
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Availability -----
                        Column {
                            width: parent.width
                            spacing: Theme.space.sm

                            Text {
                                text: "Availability"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }

                            Row {
                                spacing: Theme.space.sm
                                Repeater {
                                    model: [
                                        { key: "all",     label: "All" },
                                        { key: "inStock", label: "In stock" },
                                        { key: "outOfStock", label: "Out of stock" }
                                    ]
                                    delegate: GenreChip {
                                        label: modelData.label
                                        selected: root.viewModel && root.viewModel.availability === modelData.key
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.availability = modelData.key
                                                root.viewModel.search()
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // ----- Reset all -----
                        Row {
                            width: parent.width
                            layoutDirection: Qt.RightToLeft
                            spacing: Theme.space.md

                            SecondaryButton {
                                text: "Reset all"
                                iconName: "refresh"
                                onClicked: {
                                    if (root.viewModel) {
                                        root.viewModel.clearFilters()
                                        root.viewModel.search()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  6. Results header
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: 40
                visible: root.viewModel && (root.viewModel.hasResults || root.viewModel.isSearching)

                Row {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.space.md

                    Text {
                        text: root.viewModel
                              ? "%1 result%2".arg(root.viewModel.resultCount)
                                              .arg(root.viewModel.resultCount === 1 ? "" : "s")
                              : "0 results"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightMedium
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Item { width: 1; height: 1; Layout.fillWidth: true }

                    ViewToggle {
                        mode: root._viewMode
                        onModeChanged: root._viewMode = mode
                    }
                }
            }

            // -----------------------------------------------------------------
            //  7. Results area
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: _resultsColumn.implicitHeight

                Column {
                    id: _resultsColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: 0

                    // ----- Loading state — skeleton grid -----
                    Grid {
                        id: _skeletonGrid
                        width: parent.width
                        visible: root.viewModel && root.viewModel.isSearching
                        columns: root._gridColumns
                        spacing: Theme.space.xl

                        Repeater {
                            model: 8
                            delegate: Item {
                                width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                height: Theme.size.bookCardHeight + Theme.space.base

                                Column {
                                    anchors.fill: parent
                                    spacing: Theme.space.sm

                                    SkeletonLoader {
                                        width: parent.width
                                        height: parent.width * Theme.size.bookCoverRatio
                                        radius: Theme.radius.md
                                    }

                                    SkeletonLoader {
                                        width: parent.width * 0.85
                                        height: 14
                                        radius: Theme.radius.xs
                                    }

                                    SkeletonLoader {
                                        width: parent.width * 0.6
                                        height: 12
                                        radius: Theme.radius.xs
                                    }

                                    SkeletonLoader {
                                        width: parent.width * 0.5
                                        height: 16
                                        radius: Theme.radius.xs
                                    }
                                }
                            }
                        }
                    }

                    // ----- Error state -----
                    ErrorState {
                        width: parent.width
                        height: 480
                        visible: root.viewModel && root.viewModel.hasError
                        title: "Search failed"
                        description: root.viewModel ? root.viewModel.error : ""
                        onRetry: {
                            if (root.viewModel) root.viewModel.search()
                        }
                    }

                    // ----- Empty state -----
                    EmptyIllustration {
                        width: parent.width
                        height: 480
                        visible: root.viewModel && !root.viewModel.isSearching && !root.viewModel.hasError && !root.viewModel.hasResults
                        iconName: "search_off"
                        title: root.viewModel && root.viewModel.query.length > 0
                               ? "No books found"
                               : "Start your search"
                        description: root.viewModel && root.viewModel.query.length > 0
                                     ? "Try a different search term or adjust your filters."
                                     : "Search by title, author, or publisher to find your next great read."
                        primaryActionLabel: root.viewModel && root.viewModel.query.length > 0 ? "Browse all" : ""
                        onPrimaryActionTriggered: {
                            if (root.viewModel) {
                                root.viewModel.setQuery("")
                                root.viewModel.clearFilters()
                                root.viewModel.search()
                            }
                        }
                    }

                    // ----- Results grid -----
                    Grid {
                        id: _resultsGrid
                        width: parent.width
                        visible: root.viewModel && !root.viewModel.isSearching && !root.viewModel.hasError && root.viewModel.hasResults && root._viewMode === "grid"
                        columns: root._gridColumns
                        spacing: Theme.space.xl

                        Repeater {
                            model: root.viewModel ? root.viewModel.results : []
                            delegate: BookCard {
                                width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                book: modelData
                                onClicked: root.bookDetailRequested(book.id)
                                onAddToCartClicked: {
                                    if (root.viewModel) root.viewModel.addToCart(book.id)
                                }
                                onWishlistClicked: {
                                    if (root.viewModel) root.viewModel.toggleWishlist(book.id)
                                }
                            }
                        }
                    }

                    // ----- Results list -----
                    Column {
                        width: parent.width
                        visible: root.viewModel && !root.viewModel.isSearching && !root.viewModel.hasError && root.viewModel.hasResults && root._viewMode === "list"
                        spacing: Theme.space.sm

                        Repeater {
                            model: root.viewModel ? root.viewModel.results : []
                            delegate: BookRow {
                                width: parent.width
                                book: modelData
                                onClicked: root.bookDetailRequested(book.id)
                                onAddToCartClicked: {
                                    if (root.viewModel) root.viewModel.addToCart(book.id)
                                }
                                onWishlistClicked: {
                                    if (root.viewModel) root.viewModel.toggleWishlist(book.id)
                                }
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // -------------------------------------------------------------------------
    //  Initial load
    // -------------------------------------------------------------------------
    Component.onCompleted: {
        if (root.viewModel && !root.viewModel.hasResults && !root.viewModel.isSearching) {
            root.viewModel.search()
        }
    }

    // Hide suggestions when the page is scrolled — prevents the dropdown
    // from drifting out of alignment with the search field.
    Connections {
        target: _flickable
        ignoreUnknownSignals: true
        onMovementStarted: _suggestionsDropdown.visible = false
    }
}
