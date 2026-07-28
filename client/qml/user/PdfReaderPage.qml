// =============================================================================
//  PdfReaderPage.qml  (v15 — fully synced with the real PDF)
// =============================================================================
//  Three-panel PDF reader:
//    • Left sidebar: book cover + progress + tabs (Pages / Bookmarks)
//    • Center: REAL PDF via QtQuick.Pdf's PdfMultiPageView
//    • Top toolbar: back + title + bookmark + open-external + theme
//    • Bottom toolbar: page nav + zoom + fit
//
//  v15 fixes:
//    • Sidebar "Contents" → "Pages" — lists actual PDF page numbers (1..N),
//      not hardcoded chapters. Clicking a page jumps the PDF view. The
//      current page is highlighted.
//    • Zoom buttons now WORK — PdfMultiPageView.renderScale is bound to
//      viewModel.zoom, so zoomIn()/zoomOut() actually change the PDF scale.
//    • Bookmark (save) button is synced — uses the real PDF current page
//      from PdfMultiPageView.currentPage (0-based → 1-based).
//    • Page count is pushed from PdfDocument → ViewModel via setPageCount()
//      so the sidebar list, progress bar, page indicator, and prev/next
//      enabled states all use the ACTUAL page count, not a hardcoded 100.
//    • Horizontal scrollbar sits flush at the bottom (no bottom margin on
//      PdfMultiPageView) so it doesn't float awkwardly above the toolbar.
//    • Page nav buttons drive PdfMultiPageView.goToPage() directly when
//      the PDF is loaded — no stale VM state.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Pdf
import "../theme"
import "../components"
import "../components/buttons"
import "../components/navigation"
import "../components/data"
import "../components/surfaces"
import "../components/feedback"
import "../components/book"
import "../components/inputs"
import "../components/effects"
import "../components/progress"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null
    readonly property bool _hasBook: root.viewModel && root.viewModel.hasBook
    // v15j: _sidebarOpen is now a regular property (not readonly) so
    // the user can toggle it with a button. Defaults to true when a
    // book is open.
    property bool _sidebarOpen: root._hasBook
    readonly property bool _cleanMode: false
    readonly property string _findQuery: ""

    // Convenience: true when the real PDF is loaded and visible.
    readonly property bool _pdfReady: pdfDocument.status === PdfDocument.Ready

    // The actual current page (1-based) — uses the PDF's own cursor when
    // available, falls back to the ViewModel's page for the synthetic view.
    readonly property int _currentPage1: root._pdfReady
                                         ? (_pdfView.currentPage + 1)
                                         : (root.viewModel ? root.viewModel.page : 0)
    // The actual total page count — uses the PDF's own count when available.
    readonly property int _totalPages: root._pdfReady
                                       ? pdfDocument.pageCount
                                       : (root.viewModel ? root.viewModel.pageCount : 0)

    signal closeRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // =========================================================================
    //  Main layout: sidebar + content column
    // =========================================================================
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ----- Left Sidebar -----
        Rectangle {
            id: _sidebar
            Layout.fillHeight: true
            Layout.preferredWidth: root._sidebarOpen ? 280 : 0
            visible: root._sidebarOpen
            color: Theme.color.sidebarBackground
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.color.divider
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // ===== v15d: Toolbar row (moved from the deleted top bar) =====
                // Contains: back, bookmark, open-external, theme.
                // Placed at the TOP of the sidebar so the buttons are
                // grouped with the book info and the PDF page surface gets
                // the full height of the right column.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
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
                        anchors.leftMargin: Theme.space.sm
                        anchors.rightMargin: Theme.space.sm
                        spacing: 0

                        // Close reader (back to library)
                        IconButton {
                            iconName: "arrow_back"
                            iconColor: Theme.color.textSecondary
                            hoverIconColor: Theme.color.textPrimary
                            tooltip: "Back to library"
                            onClicked: root.closeRequested()
                        }

                        Item { Layout.fillWidth: true; height: 1 }

                        // Bookmark toggle — uses the REAL current page.
                        IconButton {
                            iconName: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                     ? "bookmark"
                                     : "bookmark_border"
                            iconColor: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                       ? Theme.color.accent
                                       : Theme.color.textSecondary
                            hoverIconColor: Theme.color.accent
                            tooltip: "Toggle bookmark (Ctrl+D)"
                            enabled: root._hasBook
                            onClicked: {
                                if (root.viewModel) {
                                    root.viewModel.toggleBookmark(root._currentPage1)
                                }
                            }
                        }

                        // Open PDF externally (when real PDF exists)
                        IconButton {
                            iconName: "open_in_new"
                            iconColor: Theme.color.textSecondary
                            hoverIconColor: Theme.color.accent
                            visible: root._hasBook && root.viewModel && root.viewModel.pdfFilePath
                                     && root.viewModel.pdfFilePath.length > 0
                            tooltip: "Open PDF in system viewer"
                            onClicked: {
                                if (root.viewModel && root.viewModel.openExternally) {
                                    root.viewModel.openExternally()
                                }
                            }
                        }

                        // Theme toggle
                        IconButton {
                            iconName: "dark_mode"
                            iconColor: Theme.color.textSecondary
                            hoverIconColor: Theme.color.textPrimary
                            tooltip: "Toggle theme"
                            onClicked: Theme.mode = Theme.isDark ? "light" : "dark"
                        }
                    }
                }

                // Book info header
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.space.lg
                    Layout.rightMargin: Theme.space.lg
                    Layout.topMargin: Theme.space.lg
                    spacing: Theme.space.sm

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.md

                        BookCover {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 64
                            book: root.viewModel
                            cornerRadius: Theme.radius.sm
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                Layout.fillWidth: true
                                text: root.viewModel ? root.viewModel.bookTitle : ""
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                wrapMode: Text.WordWrap
                            }

                            Text {
                                text: root.viewModel ? root.viewModel.bookAuthor || "" : ""
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Progress — uses the REAL PDF page count + current page.
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.sm

                        Text {
                            text: root._currentPage1 + " / " + root._totalPages + " pages"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                        Item { Layout.fillWidth: true; height: 1 }
                        Text {
                            text: root._totalPages > 0
                                 ? Math.round(root._currentPage1 / root._totalPages * 100) + "%"
                                 : "0%"
                            color: Theme.color.accent
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightBold
                        }
                    }

                    // Progress bar — bound to real PDF values.
                    Rectangle {
                        Layout.fillWidth: true
                        height: 4
                        radius: 2
                        color: Theme.color.fieldFilled

                        Rectangle {
                            width: parent.width * (root._totalPages > 0
                                ? (root._currentPage1 / root._totalPages)
                                : 0)
                            height: parent.height
                            radius: parent.radius
                            color: Theme.color.accent

                            Behavior on width {
                                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                            }
                        }
                    }
                }

                // Tab bar — Pages / Bookmarks
                property int _activeTab: 0

                Row {
                    id: _tabRow
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.space.md
                    spacing: 2

                    Repeater {
                        model: ["Pages", "Bookmarks"]

                        delegate: Rectangle {
                            width: (_tabRow.width - 1 * _tabRow.spacing) / 2
                            height: 32
                            radius: Theme.radius.sm
                            color: _tabRow.parent._activeTab === index ? Theme.color.cardBackground : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: _tabRow.parent._activeTab === index ? Theme.color.textPrimary : Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: _tabRow.parent._activeTab === index ? Theme.font.weightMedium : Theme.font.weightRegular
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: _tabRow.parent._activeTab = index
                            }
                        }
                    }
                }

                // Tab content
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: _tabRow.parent._activeTab

                    // ===== Tab 0: Pages — list of actual PDF page numbers =====
                    ScrollView {
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ListView {
                            id: _pageList
                            width: parent.width
                            model: root._totalPages
                            spacing: 0
                            clip: true

                            delegate: Rectangle {
                                width: _pageList.width
                                height: 40
                                color: root._currentPage1 === (index + 1)
                                       ? Theme.color.accentSoft
                                       : "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.space.lg
                                    anchors.rightMargin: Theme.space.lg
                                    spacing: Theme.space.sm

                                    Text {
                                        text: (index + 1)
                                        color: root._currentPage1 === (index + 1)
                                               ? Theme.color.accent
                                               : Theme.color.textMuted
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Font.Medium
                                        Layout.preferredWidth: 32
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Page " + (index + 1)
                                        color: root._currentPage1 === (index + 1)
                                               ? Theme.color.textPrimary
                                               : Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        elide: Text.ElideRight
                                    }
                                    // Bookmark indicator on this page
                                    AppIcon {
                                        visible: root.viewModel
                                                 && root.viewModel.isBookmarked(index + 1)
                                        name: "bookmark"
                                        size: 14
                                        color: Theme.color.accent
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root._pdfReady) {
                                            _pdfView.goToPage(index)
                                        } else if (root.viewModel) {
                                            root.viewModel.goToPage(index + 1)
                                        }
                                    }
                                }
                            }
                        }

                        // Show empty state when no pages (PDF not loaded yet)
                        EmptyState {
                            anchors.fill: parent
                            visible: root._totalPages === 0
                            iconName: "menu_book"
                            title: "No pages"
                            description: "Open a PDF to see the page list."
                        }
                    }

                    // ===== Tab 1: Bookmarks =====
                    ScrollView {
                        clip: true
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ColumnLayout {
                            width: parent.width
                            spacing: 0

                            // Header row: count + "bookmark current page" button
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: Theme.space.lg
                                Layout.rightMargin: Theme.space.lg
                                Layout.topMargin: Theme.space.sm
                                Layout.bottomMargin: Theme.space.sm
                                spacing: Theme.space.sm

                                Text {
                                    Layout.fillWidth: true
                                    text: (root.viewModel && root.viewModel.bookmarks ?
                                           root.viewModel.bookmarks.length : 0) + " bookmark" +
                                          (((root.viewModel && root.viewModel.bookmarks ?
                                              root.viewModel.bookmarks.length : 0) === 1) ? "" : "s")
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }
                                // "Bookmark current page" mini-button
                                Rectangle {
                                    width: 28; height: 28; radius: 14
                                    color: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                           ? Theme.color.accentSoft : Theme.color.fieldFilled
                                    border.width: 1
                                    border.color: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                                  ? Theme.color.accent : Theme.color.border

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                              ? "bookmark" : "bookmark_border"
                                        size: 16
                                        color: root.viewModel && root.viewModel.isBookmarked(root._currentPage1)
                                               ? Theme.color.accent : Theme.color.textSecondary
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (root.viewModel) {
                                                root.viewModel.toggleBookmark(root._currentPage1)
                                            }
                                        }
                                    }
                                }
                            }

                            // Divider
                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: Theme.color.divider
                                visible: root.viewModel && root.viewModel.bookmarks && root.viewModel.bookmarks.length > 0
                            }

                            Repeater {
                                model: root.viewModel && root.viewModel.bookmarks ?
                                       root.viewModel.bookmarks : []

                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 44
                                    color: "transparent"

                                    // Row click — jump to the bookmarked
                                    // page. Declared FIRST so it's at the
                                    // bottom of the z-stack; the X button
                                    // (declared later in the RowLayout)
                                    // receives clicks first.
                                    MouseArea {
                                        anchors.fill: parent
                                        z: 0
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (root._pdfReady) {
                                                _pdfView.goToPage(modelData - 1)
                                            } else if (root.viewModel) {
                                                root.viewModel.goToPage(modelData)
                                            }
                                        }
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: Theme.space.lg
                                        anchors.rightMargin: Theme.space.lg
                                        spacing: Theme.space.sm
                                        z: 1

                                        AppIcon {
                                            name: "bookmark"
                                            size: 16
                                            color: Theme.color.accent
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: "Page " + modelData
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                        // v15c: the remove (X) button.
                                        // z: 2 so it's above the row's
                                        // MouseArea (z: 0). The IconButton's
                                        // internal MouseArea handles the
                                        // click and stops propagation, so
                                        // the row click (jump to page)
                                        // doesn't also fire.
                                        IconButton {
                                            iconName: "close"
                                            iconSize: 16
                                            z: 2
                                            onClicked: {
                                                if (root.viewModel) {
                                                    root.viewModel.removeBookmark(modelData)
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            EmptyState {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                visible: !root.viewModel || !root.viewModel.bookmarks || root.viewModel.bookmarks.length === 0
                                iconName: "bookmark_border"
                                title: "No bookmarks"
                                description: "Bookmark pages to jump back quickly."
                            }
                        }
                    }
                }
            }
        }

        // ----- Right column (page surface + bottom bar) -----
        // v15d: the top toolbar was REMOVED. Its buttons (back, bookmark,
        // open-external, theme) moved to the TOP of the left sidebar so
        // they're grouped with the book info. The right column now only
        // contains the PDF page surface + the bottom page-nav toolbar.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // v15j: sidebar toggle bar — a thin strip at the top of the
            // right column with a single button to show/hide the sidebar.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                color: Theme.color.topbarBackground

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.color.divider
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.sm
                    anchors.rightMargin: Theme.space.sm
                    spacing: Theme.space.xs

                    IconButton {
                        iconName: root._sidebarOpen ? "chevron_left" : "menu"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        tooltip: root._sidebarOpen ? "Hide sidebar" : "Show sidebar"
                        onClicked: root._sidebarOpen = !root._sidebarOpen
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.viewModel ? root.viewModel.bookTitle : ""
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        elide: Text.ElideRight
                        visible: !root._sidebarOpen
                    }
                }
            }

            // ===== Page surface =====
            // Contains the PdfDocument (shared), PdfMultiPageView (real PDF),
            // loading / error overlays, and the synthetic-text fallback.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // ---- Shared PdfDocument (loaded on demand) ----
                PdfDocument {
                    id: pdfDocument
                    source: root.viewModel && root.viewModel.pdfUrl
                            ? root.viewModel.pdfUrl
                            : ""

                    // v15: Push the real page count to the ViewModel as soon
                    // as the PDF finishes loading, so the sidebar page list,
                    // progress bar, and prev/next button states all use the
                    // actual count instead of the old hardcoded "100".
                    onStatusChanged: {
                        if (status === PdfDocument.Ready && root.viewModel) {
                            root.viewModel.setPageCount(pageCount)
                        }
                    }
                    onPageCountChanged: {
                        if (root.viewModel && pageCount > 0) {
                            root.viewModel.setPageCount(pageCount)
                        }
                    }
                }

                // ---- Real PDF view ----
                // Wrapped in a PinchArea so trackpad pinch gestures zoom
                // the PDF. A WheelHandler catches Ctrl+scroll for mice.
                // The renderScale is driven by viewModel.zoom via a
                // Connections block (more robust than a direct binding
                // on some Qt 6.x builds where PdfMultiPageView.renderScale
                // doesn't re-render on property-binding updates).
                //
                // NOTE: pinch.target is NOT set, so the PinchArea does
                // NOT drag/scale any item — it only reports pinch.scale
                // in onPinchUpdated, which we feed into viewModel.zoom.
                // The PdfMultiPageView keeps full control of its own
                // panning / scrolling.
                PinchArea {
                    id: _pdfPinch
                    anchors.fill: parent
                    enabled: root._pdfReady
                    pinch.target: null
                    pinch.minimumScale: 0.5
                    pinch.maximumScale: 3.0

                    onPinchUpdated: function(pinch) {
                        if (!root.viewModel) return
                        var newZoom = root.viewModel.zoom * pinch.scale
                        newZoom = Math.max(0.5, Math.min(3.0, newZoom))
                        if (Math.abs(newZoom - root.viewModel.zoom) > 0.01) {
                            // Disable fitWidth so the manual zoom sticks.
                            if (root.viewModel.fitWidth) {
                                root.viewModel.toggleFitWidth()
                            }
                            // Set zoom directly by calling zoomIn/zoomOut
                            // in a loop — the VM doesn't expose a setZoom().
                            // We compute the delta and call the right method.
                            var diff = newZoom - root.viewModel.zoom
                            while (Math.abs(root.viewModel.zoom - newZoom) > 0.11) {
                                if (root.viewModel.zoom < newZoom) {
                                    root.viewModel.zoomIn()
                                } else {
                                    root.viewModel.zoomOut()
                                }
                            }
                        }
                    }

                    PdfMultiPageView {
                        id: _pdfView
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.lg
                        anchors.rightMargin: Theme.space.lg
                        anchors.topMargin: Theme.space.lg
                        // bottomMargin = 0 so the horizontal scrollbar sits
                        // flush at the bottom, right above the toolbar.
                        anchors.bottomMargin: 0
                        visible: root._pdfReady
                        document: pdfDocument
                        // NOTE: PdfMultiPageView is NOT a Flickable (it
                        // inherits from PdfView → Item), so it doesn't
                        // expose maximumFlickVelocity / flickDeceleration.
                        // The scroll speed is controlled internally by
                        // Qt's PDF view. The Theme-level flick speed
                        // increase still applies to all other Flickables
                        // (dashboard, sidebar, cart page, etc.).

                        // Keep the ViewModel's page in sync with the PDF's
                        // own page cursor.
                        onCurrentPageChanged: {
                            if (root.viewModel && currentPage >= 0) {
                                root.viewModel.goToPage(currentPage + 1)
                            }
                        }

                        // Ctrl+scroll wheel zoom for mice / non-pinch trackpads.
                        WheelHandler {
                            id: _wheelZoom
                            acceptedModifiers: Qt.ControlModifier
                            rotationScale: 0.01
                            grabPermissions: PointerHandler.CanTakeOverFromHandlersOfDifferentType
                            onWheel: function(event) {
                                if (!root.viewModel) return
                                var delta = event.angleDelta.y > 0 ? 0.1 : -0.1
                                var newZoom = Math.max(0.5, Math.min(3.0, root.viewModel.zoom + delta))
                                if (Math.abs(newZoom - root.viewModel.zoom) < 0.01) return
                                if (root.viewModel.fitWidth) root.viewModel.toggleFitWidth()
                                // Approximate: step zoom until close.
                                while (Math.abs(root.viewModel.zoom - newZoom) > 0.11) {
                                    if (root.viewModel.zoom < newZoom) root.viewModel.zoomIn()
                                    else root.viewModel.zoomOut()
                                }
                            }
                        }
                    }
                }

                // v15: Explicitly push viewModel.zoom → _pdfView.renderScale.
                // A direct binding (`renderScale: viewModel.zoom`) sometimes
                // doesn't trigger a re-render on Qt 6.11, so we use a
                // Connections block to set it imperatively.
                Connections {
                    target: root.viewModel
                    ignoreUnknownSignals: true
                    function onZoomChanged() {
                        if (root.viewModel && _pdfView.visible) {
                            _pdfView.renderScale = root.viewModel.zoom
                        }
                    }
                    function onBookChanged() {
                        // Reset render scale when a new book opens.
                        if (root.viewModel) {
                            _pdfView.renderScale = root.viewModel.zoom
                        }
                    }
                }

                // ---- Loading overlay (PDF still loading) ----
                Rectangle {
                    anchors.fill: parent
                    color: Theme.color.pageBackground
                    visible: pdfDocument.status === PdfDocument.Loading

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.space.md

                        Spinner {
                            size: 32
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: "Loading PDF…"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                        }
                    }
                }

                // ---- Error overlay (PDF failed to load) ----
                Rectangle {
                    anchors.fill: parent
                    color: Theme.color.pageBackground
                    visible: pdfDocument.status === PdfDocument.Error

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.space.sm

                        AppIcon {
                            name: "error_outline"
                            size: 48
                            color: Theme.color.error
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "Couldn't open the PDF file."
                            color: Theme.color.error
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.viewModel ? root.viewModel.pdfFilePath : ""
                            color: Theme.color.textMuted
                            font.family: Theme.font.familyMono
                            font.pixelSize: Theme.font.sizeCaption
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // ---- Synthetic-text fallback (no PDF / pre-load) ----
                // Only shown when there's no real PDF (pdfUrl is empty or
                // the document is in Null/Unloading state). When a PDF is
                // loading, the loading overlay above is shown instead.
                Flickable {
                    id: _pageFlick
                    anchors.fill: parent
                    visible: pdfDocument.status !== PdfDocument.Ready
                             && pdfDocument.status !== PdfDocument.Loading
                             && pdfDocument.status !== PdfDocument.Error
                    contentWidth: _pageCard.width
                    contentHeight: _pageCard.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    leftMargin: Theme.space.xxl
                    rightMargin: Theme.space.xxl
                    topMargin: Theme.space.xl
                    bottomMargin: Theme.space.xl

                    Rectangle {
                        id: _pageCard
                        width: root.viewModel && root.viewModel.fitWidth ?
                               _pageFlick.width - Theme.space.xxl * 2 :
                               Math.min(820, _pageFlick.width - Theme.space.xxl * 2) * (root.viewModel ? root.viewModel.zoom : 1.0)
                        height: Math.max(_pageFlick.height - Theme.space.xl * 2,
                                         _pageContent.implicitHeight + Theme.space.xxxl * 2)
                        radius: Theme.radius.sm
                        color: Theme.color.cardBackground
                        anchors.horizontalCenter: parent.horizontalCenter

                        layer.enabled: true
                        layer.effect: DropShadowBase { colorSpec: Theme.shadow.xl }

                        ColumnLayout {
                            id: _pageContent
                            anchors.fill: parent
                            anchors.margins: Theme.space.xxxl
                            spacing: Theme.space.lg

                            Text {
                                Layout.fillWidth: true
                                text: root.viewModel ? root.viewModel.bookTitle : ""
                                color: Theme.color.accent
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBodyLarge
                                font.weight: Theme.font.weightSemibold
                                wrapMode: Text.WordWrap
                            }

                            Divider {
                                Layout.fillWidth: true
                                orientation: "horizontal"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: root.viewModel ? root.viewModel.pageText : ""
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBodyLarge
                                font.weight: Theme.font.weightRegular
                                wrapMode: Text.WordWrap
                                lineHeight: 1.7
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.topMargin: Theme.space.md
                                text: root._currentPage1
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                            }
                        }
                    }
                }
            }

            // ===== Bottom toolbar =====
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: Theme.color.topbarBackground

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: 1
                    color: Theme.color.divider
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.lg
                    anchors.rightMargin: Theme.space.lg
                    spacing: Theme.space.sm

                    // First page
                    IconButton {
                        iconName: "first_page"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root._currentPage1 > 1
                        onClicked: {
                            if (root._pdfReady) {
                                _pdfView.goToPage(0)
                            } else if (root.viewModel) {
                                root.viewModel.firstPage()
                            }
                        }
                    }

                    // Previous page
                    IconButton {
                        iconName: "chevron_left"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root._currentPage1 > 1
                        onClicked: {
                            if (root._pdfReady) {
                                _pdfView.goToPage(Math.max(0, _pdfView.currentPage - 1))
                            } else if (root.viewModel) {
                                root.viewModel.prevPage()
                            }
                        }
                    }

                    // Page indicator — uses real PDF values when available.
                    Text {
                        text: root._currentPage1 + " / " + root._totalPages
                        color: Theme.color.textPrimary
                        font.family: Theme.font.familyMono
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightMedium
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Next page
                    IconButton {
                        iconName: "chevron_right"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root._totalPages > 0 && root._currentPage1 < root._totalPages
                        onClicked: {
                            if (root._pdfReady) {
                                _pdfView.goToPage(Math.min(pdfDocument.pageCount - 1,
                                                           _pdfView.currentPage + 1))
                            } else if (root.viewModel) {
                                root.viewModel.nextPage()
                            }
                        }
                    }

                    // Last page
                    IconButton {
                        iconName: "last_page"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root._totalPages > 0 && root._currentPage1 < root._totalPages
                        onClicked: {
                            if (root._pdfReady) {
                                _pdfView.goToPage(pdfDocument.pageCount - 1)
                            } else if (root.viewModel) {
                                root.viewModel.lastPage()
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; height: 1 }

                    // Zoom out — now WORKS because PdfMultiPageView.renderScale
                    // is bound to viewModel.zoom.
                    IconButton {
                        iconName: "zoom_out"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root.viewModel && root.viewModel.zoom > 0.5
                        onClicked: if (root.viewModel) root.viewModel.zoomOut()
                    }

                    // Zoom level
                    Text {
                        text: root.viewModel ? Math.round(root.viewModel.zoom * 100) + "%" : "100%"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.familyMono
                        font.pixelSize: Theme.font.sizeCaption
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Zoom in
                    IconButton {
                        iconName: "zoom_in"
                        iconColor: Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        enabled: root.viewModel && root.viewModel.zoom < 3.0
                        onClicked: if (root.viewModel) root.viewModel.zoomIn()
                    }

                    // Fit width — resets zoom to 1.0 (default fit).
                    IconButton {
                        iconName: "fit_screen"
                        iconColor: root.viewModel && root.viewModel.fitWidth ? Theme.color.accent : Theme.color.textSecondary
                        hoverIconColor: Theme.color.textPrimary
                        tooltip: "Fit width (reset zoom)"
                        onClicked: if (root.viewModel) root.viewModel.resetZoom()
                    }
                }
            }
        }
    }

    // ===== Empty state (no book open) =====
    EmptyState {
        anchors.fill: parent
        visible: !root._hasBook
        iconName: "menu_book"
        title: "No book open"
        description: "Open a book from your library to start reading."
    }

    // ===== Keyboard shortcuts =====
    Shortcut {
        sequence: "Escape"
        enabled: root._hasBook
        onActivated: root.closeRequested()
    }
    Shortcut {
        sequence: "Right"
        enabled: root._hasBook
        onActivated: {
            if (root._pdfReady) {
                _pdfView.goToPage(Math.min(pdfDocument.pageCount - 1, _pdfView.currentPage + 1))
            } else if (root.viewModel) {
                root.viewModel.nextPage()
            }
        }
    }
    Shortcut {
        sequence: "Left"
        enabled: root._hasBook
        onActivated: {
            if (root._pdfReady) {
                _pdfView.goToPage(Math.max(0, _pdfView.currentPage - 1))
            } else if (root.viewModel) {
                root.viewModel.prevPage()
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+D"
        enabled: root._hasBook
        onActivated: {
            if (root.viewModel) {
                root.viewModel.toggleBookmark(root._currentPage1)
            }
        }
    }
    Shortcut {
        sequence: "Ctrl++"
        enabled: root._hasBook
        onActivated: if (root.viewModel) root.viewModel.zoomIn()
    }
    Shortcut {
        sequence: "Ctrl+-"
        enabled: root._hasBook
        onActivated: if (root.viewModel) root.viewModel.zoomOut()
    }
}
