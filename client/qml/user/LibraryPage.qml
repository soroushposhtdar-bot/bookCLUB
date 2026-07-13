// =============================================================================
//  LibraryPage.qml
// =============================================================================
//  Personal library with 3 tabs: My Books / Downloaded / My Shelves.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/book"
import "../components/data"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // LibraryViewModel

    signal bookDetailRequested(string bookId)
    signal openReaderRequested(string bookId)

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property int _gridColumns: root.width < 760 ? 2 : (root.width < 1100 ? 3 : 5)
    readonly property int _activeTab: root.viewModel ? root.viewModel.activeTab : 0

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: _column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentHeight > height

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
                    text: "Your library"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                // Stats
                Row {
                    spacing: Theme.space.lg
                    anchors.verticalCenter: parent.verticalCenter

                    Column {
                        spacing: 0
                        Text {
                            text: root.viewModel ? root.viewModel.myBooksCount : 0
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBodyLarge
                            font.weight: Theme.font.weightBold
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "Books"
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    Column {
                        spacing: 0
                        Text {
                            text: root.viewModel ? root.viewModel.savedCount : 0
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBodyLarge
                            font.weight: Theme.font.weightBold
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: "Saved"
                            color: Theme.color.textMuted
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }

            // Tabs
            Item {
                width: parent.width
                height: 44

                TabBar {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    height: parent.height
                    tabs: ["My Books", "Downloaded", "My Shelves"]
                    activeIndex: root._activeTab
                    onTabSelected: {
                        if (root.viewModel) root.viewModel.activeTab = index
                    }
                }
            }

            // ----- Tab content -----
            Item {
                width: parent.width
                height: _tabContent.implicitHeight

                Column {
                    id: _tabContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    spacing: 0

                    // ===== Tab 0: My Books =====
                    Item {
                        width: parent.width
                        height: root._activeTab === 0 ? _myBooksContent.implicitHeight : 0
                        visible: root._activeTab === 0

                        Column {
                            id: _myBooksContent
                            width: parent.width
                            spacing: Theme.space.lg

                            EmptyState {
                                width: parent.width
                                height: 320
                                visible: root.viewModel && root.viewModel.myBooksCount === 0
                                iconName: "library_books"
                                title: "No books in your library yet"
                                description: "Books you purchase will appear here, ready to read."
                            }

                            Grid {
                                width: parent.width
                                visible: root.viewModel && root.viewModel.myBooksCount > 0
                                columns: root._gridColumns
                                spacing: Theme.space.xl

                                Repeater {
                                    model: root.viewModel ? root.viewModel.myBooks : []
                                    delegate: Column {
                                        width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                        spacing: Theme.space.xs

                                        BookCard {
                                            width: parent.width
                                            book: modelData
                                            showAddButton: false
                                            onClicked: root.openReaderRequested(book.id)
                                        }

                                        // Offline-download toggle — persists via LibraryService.toggleDownloaded → MockDataStore
                                        TextButton {
                                            text: (root.viewModel && root.viewModel.isDownloaded(modelData.id))
                                                  ? "✓ Downloaded"
                                                  : "📥 Download"
                                            color: (root.viewModel && root.viewModel.isDownloaded(modelData.id))
                                                   ? Theme.color.success
                                                   : Theme.color.accent
                                            hoverColor: color
                                            onClicked: if (root.viewModel) root.viewModel.toggleDownloaded(modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ===== Tab 1: Downloaded =====
                    Item {
                        width: parent.width
                        height: root._activeTab === 1 ? _downloadedContent.implicitHeight : 0
                        visible: root._activeTab === 1

                        Column {
                            id: _downloadedContent
                            width: parent.width
                            spacing: Theme.space.lg

                            EmptyState {
                                width: parent.width
                                height: 320
                                visible: (root.viewModel ? root.viewModel.downloadedBooks.length : 0) === 0
                                iconName: "download"
                                title: "No downloaded books yet"
                                description: "Tap the download button on any purchased book to save it for offline reading."
                            }

                            Grid {
                                width: parent.width
                                visible: (root.viewModel ? root.viewModel.downloadedBooks.length : 0) > 0
                                columns: root._gridColumns
                                spacing: Theme.space.xl

                                Repeater {
                                    model: root.viewModel ? root.viewModel.downloadedBooks : []
                                    delegate: BookCard {
                                        width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                                        book: modelData
                                        showAddButton: false
                                        onClicked: root.openReaderRequested(book.id)
                                    }
                                }
                            }
                        }
                    }

                    // ===== Tab 2: My Shelves =====
                    Item {
                        width: parent.width
                        height: root._activeTab === 2 ? _shelvesContent.implicitHeight : 0
                        visible: root._activeTab === 2

                        Column {
                            id: _shelvesContent
                            width: parent.width
                            spacing: Theme.space.lg

                            // Create new shelf card
                            Card {
                                width: parent.width
                                elevation: "sm"
                                padding: Theme.space.xl

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.md

                                    Text {
                                        text: "Create a new shelf"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeTitle
                                        font.weight: Theme.font.weightSemibold
                                    }

                                    InputField {
                                        width: parent.width
                                        label: "Shelf name"
                                        placeholder: "e.g. Weekend Reads"
                                        text: root.viewModel ? root.viewModel.newShelfName : ""
                                        maximumLength: 50
                                        onTextEdited: {
                                            if (root.viewModel) root.viewModel.newShelfName = newText
                                        }
                                    }

                                    InputField {
                                        width: parent.width
                                        label: "Description (optional)"
                                        placeholder: "What's this shelf for?"
                                        text: root.viewModel ? root.viewModel.newShelfDescription : ""
                                        maximumLength: 200
                                        onTextEdited: {
                                            if (root.viewModel) root.viewModel.newShelfDescription = newText
                                        }
                                    }

                                    PrimaryButton {
                                        text: "Create shelf"
                                        iconName: "create_new_folder"
                                        iconPosition: "leading"
                                        enabled: root.viewModel && root.viewModel.canCreateShelf()
                                        onClicked: {
                                            if (root.viewModel) root.viewModel.createShelf()
                                        }
                                    }
                                }
                            }

                            // Shelves list
                            EmptyState {
                                width: parent.width
                                height: 240
                                visible: root.viewModel && root.viewModel.shelves.length === 0
                                iconName: "shelves"
                                title: "No shelves yet"
                                description: "Create your first shelf above to organize your library."
                            }

                            Repeater {
                                model: root.viewModel ? root.viewModel.shelves : []

                                delegate: Card {
                                    width: parent.width
                                    elevation: "none"
                                    bordered: true
                                    padding: Theme.space.lg

                                    Column {
                                        width: parent.width
                                        spacing: Theme.space.md

                                        // Header
                                        Row {
                                            width: parent.width
                                            spacing: Theme.space.md

                                            Rectangle {
                                                width: 44; height: 44; radius: 10
                                                color: Theme.color.fieldFilled
                                                anchors.verticalCenter: parent.verticalCenter

                                                AppIcon {
                                                    anchors.centerIn: parent
                                                    name: "folder"
                                                    size: 22
                                                    color: Theme.color.textSecondary
                                                }
                                            }

                                            Column {
                                                width: parent.width - 44 - Theme.space.md - _shelfActions.width - Theme.space.md
                                                spacing: 0
                                                anchors.verticalCenter: parent.verticalCenter

                                                Text {
                                                    text: modelData.name
                                                    color: Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBodyLarge
                                                    font.weight: Theme.font.weightSemibold
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }
                                                Text {
                                                    text: modelData.description.length > 0
                                                          ? modelData.description
                                                          : "%1 book(s)".arg(modelData.bookCount)
                                                    color: Theme.color.textSecondary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeCaption
                                                    elide: Text.ElideRight
                                                    width: parent.width
                                                }
                                            }

                                            Row {
                                                id: _shelfActions
                                                spacing: 0
                                                anchors.verticalCenter: parent.verticalCenter

                                                IconButton {
                                                    iconName: "edit"
                                                    iconColor: Theme.color.textMuted
                                                    hoverIconColor: Theme.color.textPrimary
                                                    onClicked: _renameDialog.openDialog(modelData.id, modelData.name)
                                                }
                                                IconButton {
                                                    iconName: "delete_outline"
                                                    iconColor: Theme.color.textMuted
                                                    hoverIconColor: Theme.color.error
                                                    onClicked: {
                                                        if (root.viewModel) root.viewModel.deleteShelf(modelData.id)
                                                    }
                                                }
                                            }
                                        }

                                        // Books in shelf
                                        Text {
                                            visible: modelData.bookCount > 0
                                            text: "%1 book(s) in this shelf".arg(modelData.bookCount)
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }

                                        // Book chips
                                        Row {
                                            visible: modelData.bookCount > 0
                                            width: parent.width
                                            spacing: Theme.space.sm

                                            Repeater {
                                                model: modelData.bookIds.slice(0, 6)
                                                delegate: Item {
                                                    width: 36
                                                    height: 52
                                                    Rectangle {
                                                        anchors.fill: parent
                                                        radius: 4
                                                        color: Theme.color.fieldFilled
                                                        border.color: Theme.color.border
                                                        border.width: 1
                                                    }
                                                    Text {
                                                        anchors.centerIn: parent
                                                        text: "📚"
                                                        font.pixelSize: Theme.font.sizeTitle
                                                    }
                                                }
                                            }
                                            Text {
                                                visible: modelData.bookCount > 6
                                                text: "+%1 more".arg(modelData.bookCount - 6)
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                anchors.verticalCenter: parent.verticalCenter
                                                leftPadding: Theme.space.sm
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // ----- Rename dialog (inline Popup with InputField) -----
    Popup {
        id: _renameDialog
        property string shelfId: ""
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: 420
        height: _renameCol.implicitHeight + 2 * Theme.space.xl
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        background: Card {
            radius: Theme.radius.xl
            elevation: "xl"
            bordered: false
            backgroundColor: Theme.color.cardBackground
            padding: 0
        }

        function openDialog(id, name) {
            shelfId = id
            _renameField.text = name
            open()
            _renameField.forceActiveFocus()
        }

        Column {
            id: _renameCol
            anchors.fill: parent
            anchors.margins: Theme.space.xxl
            spacing: Theme.space.lg

            Text {
                text: "Rename shelf"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightSemibold
            }

            InputField {
                id: _renameField
                width: parent.width
                label: "Shelf name"
                placeholder: "Enter a new name"
                maximumLength: 50
                onAccepted: {
                    if (root.viewModel && _renameField.text.length > 0) {
                        root.viewModel.renameShelf(_renameDialog.shelfId, _renameField.text)
                    }
                    _renameDialog.close()
                }
            }

            Row {
                width: parent.width
                spacing: Theme.space.md
                layoutDirection: Qt.RightToLeft

                PrimaryButton {
                    text: "Rename"
                    enabled: _renameField.text.length > 0
                    onClicked: {
                        if (root.viewModel && _renameField.text.length > 0) {
                            root.viewModel.renameShelf(_renameDialog.shelfId, _renameField.text)
                        }
                        _renameDialog.close()
                    }
                }

                SecondaryButton {
                    text: "Cancel"
                    onClicked: _renameDialog.close()
                }
            }
        }
    }
}
