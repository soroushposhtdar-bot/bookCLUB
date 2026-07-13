// =============================================================================
//  PdfReaderPage.qml
// =============================================================================
//  Premium PDF reader with a collapsible left sidebar (TOC / Thumbnails /
//  Bookmarks), top toolbar, page surface, and bottom progress bar.
//
//  Layout:
//      ┌──────────────────────────────────────────────────────────────┐
//      │ ┌────────────┬─────────────────────────────────────────────┐ │
//      │ │            │  Toolbar: back, title, prev/next, page,     │ │
//      │ │ Sidebar    │           zoom, fit, search, brightness     │ │
//      │ │ (TOC /     ├─────────────────────────────────────────────┤ │
//      │ │  Thumbs /  │                                             │ │
//      │ │  Marks)    │           Page surface (Flickable)          │ │
//      │ │            │                                             │ │
//      │ │            ├─────────────────────────────────────────────┤ │
//      │ │            │  Progress bar (current / total)             │ │
//      │ └────────────┴─────────────────────────────────────────────┘ │
//      └──────────────────────────────────────────────────────────────┘
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/progress"
import "../components/feedback"
import "../components/navigation"
import "../components/effects"
import "../components/data"
import "../components/book"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // ReaderViewModel

    // Find-in-book state — the current search query. The page-text renderer
    // uses this to highlight matches via a simple case-insensitive split.
    property string _findQuery: ""

    signal closeRequested()

    readonly property bool _hasBook: root.viewModel && root.viewModel.hasBook
    readonly property bool _cleanMode: root.viewModel && root.viewModel.cleanMode
    readonly property bool _sidebarOpen: _sidebar.visible

    // Find-in-book: walk pages starting from the current one until we find
    // one whose pageText contains the query (case-insensitive). Jumps to it.
    function _findNext(query) {
        if (!root.viewModel || query.length === 0) return
        const q = query.toLowerCase()
        const currentPage = root.viewModel.page
        const pageCount = root.viewModel.pageCount
        // Search forward from current+1, wrapping around to page 1.
        for (let offset = 1; offset <= pageCount; ++offset) {
            const p = ((currentPage - 1 + offset) % pageCount) + 1
            const text = (root.viewModel.pageTextFor ? root.viewModel.pageTextFor(p) : "") + ""
            if (text.toLowerCase().indexOf(q) >= 0) {
                root.viewModel.goToPage(p)
                return
            }
        }
        // No match found — stay on the current page.
    }

    // Full-screen dark surface
    Rectangle {
        anchors.fill: parent
        color: Theme.isDark ? "#000000" : "#1A1A1C"
    }

    // ----- Sidebar (TOC / Thumbnails / Bookmarks) -----
    Rectangle {
        id: _sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 260
        color: Theme.isDark ? "#0E0F11" : "#F4F5F7"
        visible: !root._cleanMode && _hasBook
        z: 2

        Behavior on width {
            NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }

        Column {
            anchors.fill: parent
            spacing: 0

            // Sidebar header
            Item {
                width: parent.width
                height: 56
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: Theme.color.divider
                    border.width: 0
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Theme.color.divider
                    }
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: "Contents"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightSemibold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Close-sidebar button
                IconButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 8
                    iconName: "menu_open"
                    iconColor: Theme.color.textSecondary
                    hoverIconColor: Theme.color.textPrimary
                    onClicked: _sidebar.visible = false
                }
            }

            // Sidebar tabs (TOC / Thumbnails / Bookmarks)
            TabBar {
                id: _sidebarTabs
                width: parent.width
                height: 40
                tabs: ["TOC", "Pages", "Marks"]
                activeIndex: 0
                onTabSelected: function(index) {
                    _sidebarTabs.activeIndex = index
                }
            }

            // Sidebar content — switches between TOC / Pages / Marks via a StackLayout
            StackLayout {
                id: _sidebarStack
                width: parent.width
                height: parent.height - 56 - 40
                currentIndex: _sidebarTabs.activeIndex
                clip: true

                // ----- Tab 0: TOC -----
                Flickable {
                    contentWidth: width
                    contentHeight: _tocCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: _tocCol
                        width: parent.width
                        spacing: 2

                        // TOC entries (driven by viewModel.tableOfContents)
                        Repeater {
                            model: root._hasBook && root.viewModel.tableOfContents
                                   ? root.viewModel.tableOfContents : []
                            delegate: Item {
                                width: parent.width
                                height: 56

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: Theme.radius.md
                                    color: root.viewModel && root.viewModel.page === (index + 1)
                                           ? Theme.color.accentSoft
                                           : (_tocMa.containsMouse ? Theme.color.fieldFilled : "transparent")
                                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                                }

                                Row {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 12

                                    Text {
                                        text: "Ch %1".arg(index + 1)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Column {
                                        spacing: 0
                                        Text {
                                            text: modelData
                                            color: root.viewModel && root.viewModel.page === (index + 1)
                                                   ? Theme.color.accent
                                                   : Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: root.viewModel && root.viewModel.page === (index + 1) ? Theme.font.weightSemibold : Theme.font.weightRegular
                                            elide: Text.ElideRight
                                            width: 180
                                        }
                                        Text {
                                            text: "Page " + (index + 1)
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeMicro2
                                        }
                                    }
                                }

                                MouseArea {
                                    id: _tocMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (root.viewModel) root.viewModel.goToPage(index + 1)
                                }
                            }
                        }
                    }
                }

                // ----- Tab 1: Pages (thumbnails grid) -----
                // Renders a grid of page-number tiles. Clicking a tile jumps
                // to that page. Real PDF thumbnails would require a PDF
                // rendering library; the mock uses numbered tiles.
                Flickable {
                    contentWidth: width
                    contentHeight: _pagesGrid.height
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Grid {
                        id: _pagesGrid
                        width: parent.width
                        columns: 3
                        spacing: 8

                        Repeater {
                            model: root._hasBook && root.viewModel ? root.viewModel.pageCount : 0
                            delegate: Rectangle {
                                width: (parent.width - 2 * 8) / 3
                                height: 80
                                radius: Theme.radius.sm
                                color: root.viewModel && root.viewModel.page === (index + 1)
                                       ? Theme.color.accentSoft
                                       : Theme.color.fieldFilled
                                border.color: root.viewModel && root.viewModel.page === (index + 1)
                                       ? Theme.color.accent
                                       : Theme.color.divider
                                border.width: root.viewModel && root.viewModel.page === (index + 1) ? 2 : 1

                                Text {
                                    anchors.centerIn: parent
                                    text: (index + 1).toString()
                                    color: root.viewModel && root.viewModel.page === (index + 1)
                                           ? Theme.color.accent
                                           : Theme.color.textSecondary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (root.viewModel) root.viewModel.goToPage(index + 1)
                                }
                            }
                        }
                    }
                }

                // ----- Tab 2: Marks (bookmarks list) -----
                // Bound to viewModel.bookmarks. Each entry shows the page
                // number + a "remove" button. Empty state when no bookmarks.
                Flickable {
                    contentWidth: width
                    contentHeight: _marksCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: _marksCol
                        width: parent.width
                        spacing: 2

                        // Empty state
                        Item {
                            width: parent.width
                            height: 120
                            visible: !root.viewModel || !root.viewModel.bookmarks || root.viewModel.bookmarks.length === 0
                            Column {
                                anchors.centerIn: parent
                                spacing: Theme.space.sm
                                AppIcon {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    name: "bookmark_border"
                                    size: 32
                                    color: Theme.color.textMuted
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "No bookmarks yet"
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "Press the bookmark icon in the toolbar to save a page."
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeMicro2
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 200
                                }
                            }
                        }

                        // Bookmark entries
                        Repeater {
                            model: root.viewModel ? root.viewModel.bookmarks : []
                            delegate: Item {
                                width: parent.width
                                height: 48

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: Theme.radius.md
                                    color: root.viewModel && root.viewModel.page === modelData
                                           ? Theme.color.accentSoft
                                           : (_marksMa.containsMouse ? Theme.color.fieldFilled : "transparent")
                                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                                }

                                Row {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8

                                    AppIcon {
                                        name: "bookmark"
                                        size: 16
                                        color: Theme.color.accent
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "Page " + modelData
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                IconButton {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    iconName: "close"
                                    iconColor: Theme.color.textMuted
                                    onClicked: if (root.viewModel) root.viewModel.removeBookmark(modelData)
                                }

                                MouseArea {
                                    id: _marksMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: if (root.viewModel) root.viewModel.goToPage(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }

        // ----- Top toolbar -----
        Item {
            id: _toolbar
            anchors.left: _sidebar.visible ? _sidebar.right : parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 56
            visible: !root._cleanMode
            z: 1

            Rectangle {
                anchors.fill: parent
                color: "rgba(0, 0, 0, 0.30)"
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: Theme.space.lg
                anchors.rightMargin: Theme.space.lg
                spacing: Theme.space.md

                // Sidebar toggle (when sidebar is hidden)
                IconButton {
                    iconName: "menu"
                    iconColor: "#FFFFFF"
                    hoverIconColor: "#FFFFFF"
                    width: 40; height: 40
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !_sidebar.visible
                    onClicked: _sidebar.visible = true
                }

                IconButton {
                    iconName: "close"
                    iconColor: "#FFFFFF"
                    hoverIconColor: "#FFFFFF"
                    width: 40; height: 40
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.closeRequested()
                }

                Text {
                    text: root.viewModel ? root.viewModel.bookTitle : ""
                    color: "#FFFFFF"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightSemibold
                    elide: Text.ElideRight
                    width: parent.width - 40 - Theme.space.md - _toolbarRight.width - Theme.space.md - 200
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Search box
                Item {
                    width: 200
                    height: 36
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._hasBook

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radius.md
                        color: "rgba(255, 255, 255, 0.10)"
                        border.color: "rgba(255, 255, 255, 0.18)"
                        border.width: 1
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        AppIcon {
                            name: "search"
                            size: 18
                            color: "rgba(255,255,255,0.7)"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        TextField {
                            id: _findField
                            width: parent.width - 26
                            height: parent.height
                            placeholderText: "Find in book..."
                            placeholderTextColor: "rgba(255,255,255,0.5)"
                            color: "#FFFFFF"
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            verticalAlignment: TextInput.AlignVCenter
                            background: Item {}
                            selectByMouse: true
                            onTextEdited: {
                                root._findQuery = text
                            }
                            onAccepted: {
                                // Jump to the first page that contains the query.
                                if (root.viewModel && text.length > 0) {
                                    root._findNext(text)
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true; width: 1; height: 1 }

                Row {
                    id: _toolbarRight
                    spacing: 4
                    anchors.verticalCenter: parent.verticalCenter

                    IconButton {
                        iconName: "chevron_left"
                        iconColor: "#FFFFFF"
                        hoverIconColor: "#FFFFFF"
                        width: 40; height: 40
                        enabled: root.viewModel && root.viewModel.page > 1
                        onClicked: if (root.viewModel) root.viewModel.prevPage()
                    }
                    Rectangle {
                        width: _pageInd.implicitWidth + 20; height: 32; radius: 16
                        color: "rgba(255, 255, 255, 0.10)"
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: _pageInd
                            anchors.centerIn: parent
                            text: (root.viewModel ? root.viewModel.page : 0) + " / " + (root.viewModel ? root.viewModel.pageCount : 0)
                            color: "#FFFFFF"
                            font.family: Theme.font.familyMono
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }
                    }
                    IconButton {
                        iconName: "chevron_right"
                        iconColor: "#FFFFFF"
                        hoverIconColor: "#FFFFFF"
                        width: 40; height: 40
                        enabled: root.viewModel && root.viewModel.page < root.viewModel.pageCount
                        onClicked: if (root.viewModel) root.viewModel.nextPage()
                    }

                    Rectangle { width: 1; height: 24; color: "rgba(255, 255, 255, 0.18)"; anchors.verticalCenter: parent.verticalCenter }

                    IconButton {
                        iconName: "zoom_out"
                        iconColor: "#FFFFFF"
                        hoverIconColor: "#FFFFFF"
                        width: 40; height: 40
                        onClicked: if (root.viewModel) root.viewModel.zoomOut()
                    }
                    IconButton {
                        iconName: "zoom_in"
                        iconColor: "#FFFFFF"
                        hoverIconColor: "#FFFFFF"
                        width: 40; height: 40
                        onClicked: if (root.viewModel) root.viewModel.zoomIn()
                    }
                    IconButton {
                        iconName: "fit_screen"
                        iconColor: root.viewModel && root.viewModel.fitWidth ? Theme.color.accent : "#FFFFFF"
                        hoverIconColor: Theme.color.accent
                        width: 40; height: 40
                        onClicked: if (root.viewModel) root.viewModel.toggleFitWidth()
                    }

                    Rectangle { width: 1; height: 24; color: "rgba(255, 255, 255, 0.18)"; anchors.verticalCenter: parent.verticalCenter }

                    IconButton {
                        iconName: root.viewModel && root.viewModel.pageBookmarked ? "bookmark" : "bookmark_border"
                        iconColor: root.viewModel && root.viewModel.pageBookmarked ? Theme.color.accent : "#FFFFFF"
                        hoverIconColor: Theme.color.accent
                        width: 40; height: 40
                        onClicked: if (root.viewModel) root.viewModel.toggleBookmark()
                    }
                    IconButton {
                        iconName: "contrast"
                        iconColor: "#FFFFFF"
                        hoverIconColor: "#FFFFFF"
                        width: 40; height: 40
                        onClicked: if (root.viewModel) root.viewModel.toggleCleanMode()
                    }
                }
            }
        }

        // ----- Page surface -----
        Flickable {
            id: _pageFlick
            anchors.left: _sidebar.visible ? _sidebar.right : parent.left
            anchors.right: parent.right
            anchors.top: root._cleanMode ? parent.top : _toolbar.bottom
            anchors.bottom: _progress.top
            anchors.margins: Theme.space.xxl
            contentWidth: _pageCard.width
            contentHeight: _pageCard.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Rectangle {
                id: _pageCard
                width: root.viewModel && root.viewModel.fitWidth
                       ? _pageFlick.width
                       : Math.min(820, _pageFlick.width) * (root.viewModel ? root.viewModel.zoom : 1.0)
                height: Math.max(_pageFlick.height, _pageText.implicitHeight + 2 * Theme.space.xxxl)
                radius: Theme.radius.sm
                color: Theme.isDark ? Theme.color.cardBackground
                     : (root.viewModel && root.viewModel.cleanMode ? "#FBF8F1" : "#FFFFFF")
                anchors.horizontalCenter: parent.horizontalCenter

                layer.enabled: true
                layer.effect: DropShadowBase { colorSpec: Theme.shadow.xl }

                Column {
                    id: _pageText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Theme.space.xxxl
                    spacing: Theme.space.lg

                    Row {
                        width: parent.width
                        spacing: Theme.space.md
                        Text {
                            text: root.viewModel ? root.viewModel.bookTitle : ""
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightBold
                        }
                        Item { Layout.fillWidth: true; width: 1; height: 1 }
                        Text {
                            text: "Page " + (root.viewModel ? root.viewModel.page : 0)
                            color: Theme.color.textMuted
                            font.family: Theme.font.familyMono
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }

                    Divider { width: parent.width; orientation: "horizontal" }

                    Text {
                        width: parent.width
                        text: root.viewModel ? root.viewModel.pageText : ""
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBodyLarge
                        font.weight: Theme.font.weightRegular
                        wrapMode: Text.WordWrap
                        lineHeight: 1.7
                    }
                }
            }
        }

        // ----- Loading overlay (visible while a book is opening) -----
        Item {
            anchors.fill: parent
            visible: root.viewModel && root.viewModel.loading
            z: 10

            Rectangle {
                anchors.fill: parent
                color: Theme.isDark ? "#000000" : "#1A1A1C"
                opacity: 0.92
            }

            Column {
                anchors.centerIn: parent
                spacing: Theme.space.md

                Spinner {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: 36
                    color: "#FFFFFF"
                    progress: -1
                }
                Text {
                    text: "Opening book…"
                    color: "#FFFFFF"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightMedium
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // ----- Error state (visible when opening failed) -----
        Item {
            anchors.fill: parent
            visible: root.viewModel && root.viewModel.hasError && !root.viewModel.hasBook
            z: 10

            Rectangle { anchors.fill: parent; color: Theme.isDark ? "#000000" : "#1A1A1C" }

            Column {
                anchors.centerIn: parent
                spacing: Theme.space.lg

                Rectangle {
                    width: 64; height: 64; radius: 32
                    color: Theme.color.errorSoft
                    anchors.horizontalCenter: parent.horizontalCenter
                    AppIcon {
                        anchors.centerIn: parent
                        name: "error_outline"
                        size: 32
                        color: Theme.color.error
                    }
                }
                Text {
                    text: "Couldn't open this book"
                    color: "#FFFFFF"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeTitle
                    font.weight: Theme.font.weightBold
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: root.viewModel ? root.viewModel.error : ""
                    color: "rgba(255, 255, 255, 0.7)"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                PrimaryButton {
                    text: "Close"
                    iconName: "close"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: root.closeRequested()
                }
            }
        }

        // ----- Empty state (no book loaded yet — shouldn't normally happen) -----
        Item {
            anchors.fill: parent
            visible: root.viewModel && !root.viewModel.hasBook && !root.viewModel.loading && !root.viewModel.hasError
            z: 10

            Rectangle { anchors.fill: parent; color: Theme.isDark ? "#000000" : "#1A1A1C" }

            Column {
                anchors.centerIn: parent
                spacing: Theme.space.lg

                Rectangle {
                    width: 64; height: 64; radius: 32
                    color: "rgba(255, 255, 255, 0.10)"
                    anchors.horizontalCenter: parent.horizontalCenter
                    AppIcon {
                        anchors.centerIn: parent
                        name: "menu_book"
                        size: 32
                        color: "#FFFFFF"
                    }
                }
                Text {
                    text: "No book open"
                    color: "#FFFFFF"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeTitle
                    font.weight: Theme.font.weightBold
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "Choose a book from your library to start reading."
                    color: "rgba(255, 255, 255, 0.7)"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                PrimaryButton {
                    text: "Close reader"
                    iconName: "close"
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: root.closeRequested()
                }
            }
        }

        // ----- Bottom progress bar -----
        Item {
            id: _progress
            anchors.left: _sidebar.visible ? _sidebar.right : parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 4
            visible: !root._cleanMode

            Rectangle {
                anchors.fill: parent
                color: "rgba(255, 255, 255, 0.10)"
            }
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * (root.viewModel && root.viewModel.pageCount > 0
                                       ? root.viewModel.page / root.viewModel.pageCount : 0)
                color: Theme.color.accent
                Behavior on width { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }
            }
        }

        // ----- Clean-mode exit -----
        Item {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.space.xl
            width: 48; height: 48
            visible: root._cleanMode

            Rectangle {
                anchors.fill: parent
                radius: 24
                color: "rgba(255, 255, 255, 0.14)"
            }
            IconButton {
                anchors.fill: parent
                iconName: "fullscreen_exit"
                iconColor: "#FFFFFF"
                hoverIconColor: "#FFFFFF"
                onClicked: if (root.viewModel) root.viewModel.toggleCleanMode()
            }
        }

        // ----- Keyboard shortcuts -----
        Shortcut { sequence: "Left";  enabled: root._hasBook; onActivated: if (root.viewModel) root.viewModel.prevPage() }
        Shortcut { sequence: "Right"; enabled: root._hasBook; onActivated: if (root.viewModel) root.viewModel.nextPage() }
        Shortcut { sequence: "Escape"; enabled: root._hasBook; onActivated: root.closeRequested() }
        Shortcut { sequence: "Ctrl+F"; enabled: root._hasBook; onActivated: if (root.viewModel) root.viewModel.toggleFitWidth() }
        Shortcut { sequence: "Ctrl+L"; enabled: root._hasBook; onActivated: if (root.viewModel) root.viewModel.toggleCleanMode() }
        Shortcut { sequence: "Ctrl+T"; enabled: root._hasBook; onActivated: _sidebar.visible = !_sidebar.visible }
    }
}
