// =============================================================================
//  ShelvesPage.qml
// =============================================================================
//  Full shelves management page.
//
//  Layout (top → bottom):
//      1. Header — "My Shelves" title + count badge + ViewToggle (grid/list)
//         + SortDropdown (Manual / Name / Recent / Book count) + SearchField.
//      2. "Create new shelf" Card — InputField for name, InputField for
//         description, color picker (8 swatches from Theme.accentPalette),
//         private toggle (AppCheckbox), "Create shelf" PrimaryButton
//         (disabled when !canCreate).
//      3. Shelves list — grid (3-column responsive) or list view. Each shelf
//         card shows: colored folder icon (shelf.color), name, description or
//         "N book(s)", favorite star (filled if favorite), private lock icon,
//         book count chip. Hover lifts + adds shadow. Click → selectShelf(id).
//         Right-click → context menu (Rename / Duplicate / Set color submenu /
//         Toggle favorite / Toggle private / Move up / Move down / Delete).
//      4. Selected shelf detail panel — BookRow list of the shelf's books,
//         "Add book" button, per-book remove button. Animates in.
//      5. Empty state — EmptyIllustration "No shelves yet" + description +
//         "Create your first shelf" button (scrolls to create form).
//      6. Delete confirmation — ConfirmDialog (danger style).
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
import "../components/effects"
import BookClub.Services 1.0

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // ShelfViewModel

    signal bookDetailRequested(string bookId)
    signal openReaderRequested(string bookId)
    signal addBookRequested(string shelfId)

    // ----- Layout constants -----
    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property int _gridColumns: root.width < 760 ? 1
                                       : root.width < 1100 ? 2 : 3

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
            //  1. Header
            // -----------------------------------------------------------------
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                spacing: Theme.space.md

                Text {
                    text: "My Shelves"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Count badge
                Rectangle {
                    width: _countText.implicitWidth + 16
                    height: 26
                    radius: 13
                    color: Theme.color.fieldFilled
                    border.color: Theme.color.border
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        id: _countText
                        anchors.centerIn: parent
                        text: root.viewModel ? root.viewModel.count : 0
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightSemibold
                    }
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                // Sort dropdown
                SortDropdown {
                    id: _sort
                    width: 220
                    height: 36
                    anchors.verticalCenter: parent.verticalCenter
                    options: [
                        { label: "Manual order",  value: "manual" },
                        { label: "Name (A→Z)",    value: "name" },
                        { label: "Recent",        value: "recent" },
                        { label: "Book count",    value: "count" }
                    ]
                    currentValue: root.viewModel ? root.viewModel.sortMode : "manual"
                    onChanged: function(value) {
                        if (root.viewModel) root.viewModel.sortMode = value
                    }
                }

                // View toggle
                ViewToggle {
                    mode: root.viewModel ? root.viewModel.viewMode : "grid"
                    onModeChanged: {
                        if (root.viewModel) root.viewModel.viewMode = mode
                    }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // -----------------------------------------------------------------
            //  2. Search bar (separate row to give it room)
            // -----------------------------------------------------------------
            Item {
                width: parent.width
                height: 56

                SearchField {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: root._horizontalPadding
                    anchors.rightMargin: root._horizontalPadding
                    anchors.verticalCenter: parent.verticalCenter
                    height: 48
                    placeholder: "Search shelves by name…"
                    text: root.viewModel ? root.viewModel.searchQuery : ""
                    onTextEdited: {
                        if (root.viewModel) root.viewModel.searchQuery = newText
                    }
                }
            }

            // -----------------------------------------------------------------
            //  3. "Create new shelf" Card
            // -----------------------------------------------------------------
            Card {
                id: _createCard
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "sm"
                padding: Theme.space.xl
                height: _createCardContent.implicitHeight + 2 * Theme.space.xl

                Column {
                    id: _createCardContent
                    width: parent.width
                    spacing: Theme.space.lg

                    // Section title
                    Row {
                        spacing: Theme.space.md

                        Rectangle {
                            width: 36
                            height: 36
                            radius: Theme.radius.md
                            color: Theme.color.accentSoft
                            anchors.verticalCenter: parent.verticalCenter

                            AppIcon {
                                anchors.centerIn: parent
                                name: "create_new_folder"
                                size: 20
                                color: Theme.color.accent
                            }
                        }

                        Text {
                            text: "Create a new shelf"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightSemibold
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // Name + Description row
                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        InputField {
                            width: (parent.width - Theme.space.lg) / 2
                            label: "Shelf name"
                            placeholder: "e.g. Weekend Reads"
                            text: root.viewModel ? root.viewModel.newName : ""
                            maximumLength: 50
                            leadingIcon: "folder"
                            onTextEdited: {
                                if (root.viewModel) root.viewModel.newName = newText
                            }
                        }

                        InputField {
                            width: (parent.width - Theme.space.lg) / 2
                            label: "Description (optional)"
                            placeholder: "What's this shelf for?"
                            text: root.viewModel ? root.viewModel.newDescription : ""
                            maximumLength: 200
                            onTextEdited: {
                                if (root.viewModel) root.viewModel.newDescription = newText
                            }
                        }
                    }

                    // Color picker
                    Column {
                        width: parent.width
                        spacing: Theme.space.sm

                        Text {
                            text: "Color"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                            font.weight: Theme.font.weightMedium
                        }

                        Row {
                            spacing: Theme.space.sm

                            Repeater {
                                model: Theme.accentPalette
                                delegate: Item {
                                    width: 32
                                    height: 32

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: width / 2
                                        color: modelData.color
                                        border.color: (root.viewModel && root.viewModel.newColor === modelData.color)
                                                       ? Theme.color.textPrimary
                                                       : "transparent"
                                        border.width: 2
                                        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }

                                        scale: _swatchMa.containsMouse ? 1.1 : 1.0
                                        Behavior on scale {
                                            NumberAnimation {
                                                duration: Theme.motion.durationFast
                                                easing.type: Easing.OutBack
                                            }
                                        }

                                        // Checkmark when selected
                                        AppIcon {
                                            anchors.centerIn: parent
                                            name: "check"
                                            size: 16
                                            color: Theme.color.textOnPrimary
                                            visible: root.viewModel && root.viewModel.newColor === modelData.color
                                        }
                                    }

                                    MouseArea {
                                        id: _swatchMa
                                        anchors.fill: parent
                                        anchors.margins: -4
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (root.viewModel) root.viewModel.newColor = modelData.color
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Private toggle + Create button
                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        AppCheckbox {
                            checked: root.viewModel ? root.viewModel.newIsPrivate : false
                            label: "Make private"
                            helperText: "Only you can see this shelf"
                            onToggled: {
                                if (root.viewModel) root.viewModel.newIsPrivate = checked
                            }
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Item { width: 1; height: 1; Layout.fillWidth: true }

                        PrimaryButton {
                            text: "Create shelf"
                            iconName: "create_new_folder"
                            iconPosition: "leading"
                            enabled: root.viewModel && root.viewModel.canCreate
                            onClicked: {
                                if (root.viewModel) root.viewModel.createShelf()
                            }
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  4. Empty state
            // -----------------------------------------------------------------
            EmptyIllustration {
                width: parent.width
                height: 360
                visible: root.viewModel && root.viewModel.isEmpty
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                iconName: "shelves"
                title: "No shelves yet"
                description: "Create your first shelf above to organize your library by mood, genre, or reading goal."
                primaryActionLabel: "Create your first shelf"
                onPrimaryActionTriggered: root.scrollToCreateForm()
            }

            // -----------------------------------------------------------------
            //  5. Shelves grid (grid view)
            // -----------------------------------------------------------------
            Grid {
                id: _shelvesGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                visible: root.viewModel && !root.viewModel.isEmpty
                         && (root.viewModel.viewMode === "grid")
                columns: root._gridColumns
                spacing: Theme.space.lg

                Repeater {
                    model: root.viewModel ? root.viewModel.shelves : []
                    delegate: ShelfCard {
                        width: (parent.width - (root._gridColumns - 1) * parent.spacing) / root._gridColumns
                        shelf: modelData
                        selected: root.viewModel && root.viewModel.selectedShelfId === modelData.id
                        onClicked: {
                            if (root.viewModel) root.viewModel.selectShelf(modelData.id)
                        }
                        onRenameRequested: _renameDialog.openDialog(modelData.id, modelData.name)
                        onDuplicateRequested: {
                            if (root.viewModel) root.viewModel.duplicateShelf(modelData.id)
                        }
                        onSetColorRequested: {
                            _currentColorShelfId = modelData.id
                            _colorPickerPopup.open()
                        }
                        onToggleFavoriteRequested: {
                            if (root.viewModel) root.viewModel.toggleFavorite(modelData.id)
                        }
                        onTogglePrivateRequested: {
                            if (root.viewModel) root.viewModel.togglePrivate(modelData.id)
                        }
                        onMoveUpRequested: {
                            if (root.viewModel) root.viewModel.moveUp(modelData.id)
                        }
                        onMoveDownRequested: {
                            if (root.viewModel) root.viewModel.moveDown(modelData.id)
                        }
                        onDeleteRequested: {
                            _deleteDialog.shelfId = modelData.id
                            _deleteDialog.shelfName = modelData.name
                            _deleteDialog.openDialog({
                                title: "Delete shelf?",
                                message: "This will permanently remove the shelf.",
                                detail: "Books will remain in your library.",
                                iconName: "delete_outline",
                                confirmLabel: "Delete",
                                cancelLabel: "Cancel",
                                confirmStyle: "danger",
                                onConfirmed: function() {
                                    if (root.viewModel) root.viewModel.deleteShelf(_deleteDialog.shelfId)
                                }
                            })
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  5b. Shelves list (list view)
            // -----------------------------------------------------------------
            Column {
                id: _shelvesList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                visible: root.viewModel && !root.viewModel.isEmpty
                         && (root.viewModel.viewMode === "list")
                spacing: Theme.space.sm

                Repeater {
                    model: root.viewModel ? root.viewModel.shelves : []
                    delegate: ShelfRow {
                        width: parent.width
                        shelf: modelData
                        selected: root.viewModel && root.viewModel.selectedShelfId === modelData.id
                        onClicked: {
                            if (root.viewModel) root.viewModel.selectShelf(modelData.id)
                        }
                        onRenameRequested: _renameDialog.openDialog(modelData.id, modelData.name)
                        onDuplicateRequested: {
                            if (root.viewModel) root.viewModel.duplicateShelf(modelData.id)
                        }
                        onSetColorRequested: {
                            _currentColorShelfId = modelData.id
                            _colorPickerPopup.open()
                        }
                        onToggleFavoriteRequested: {
                            if (root.viewModel) root.viewModel.toggleFavorite(modelData.id)
                        }
                        onTogglePrivateRequested: {
                            if (root.viewModel) root.viewModel.togglePrivate(modelData.id)
                        }
                        onMoveUpRequested: {
                            if (root.viewModel) root.viewModel.moveUp(modelData.id)
                        }
                        onMoveDownRequested: {
                            if (root.viewModel) root.viewModel.moveDown(modelData.id)
                        }
                        onDeleteRequested: {
                            _deleteDialog.shelfId = modelData.id
                            _deleteDialog.shelfName = modelData.name
                            _deleteDialog.openDialog({
                                title: "Delete shelf?",
                                message: "This will permanently remove the shelf.",
                                detail: "Books will remain in your library.",
                                iconName: "delete_outline",
                                confirmLabel: "Delete",
                                cancelLabel: "Cancel",
                                confirmStyle: "danger",
                                onConfirmed: function() {
                                    if (root.viewModel) root.viewModel.deleteShelf(_deleteDialog.shelfId)
                                }
                            })
                        }
                    }
                }
            }

            // -----------------------------------------------------------------
            //  6. Selected shelf detail panel
            // -----------------------------------------------------------------
            Card {
                id: _detailCard
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: root._horizontalPadding
                anchors.rightMargin: root._horizontalPadding
                elevation: "md"
                padding: Theme.space.xl
                visible: root.viewModel && root.viewModel.selectedShelfId.length > 0
                height: visible ? _detailContent.implicitHeight + 2 * Theme.space.xl : 0

                // Slide-up entrance
                opacity: visible ? 1.0 : 0.0
                Behavior on opacity {
                    NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
                }

                Column {
                    id: _detailContent
                    width: parent.width
                    spacing: Theme.space.lg

                    // Header
                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Rectangle {
                            width: 44
                            height: 44
                            radius: Theme.radius.md
                            color: Qt.rgba(_detailShelfColor.r, _detailShelfColor.g, _detailShelfColor.b, 0.16)
                            anchors.verticalCenter: parent.verticalCenter

                            AppIcon {
                                anchors.centerIn: parent
                                name: "folder_open"
                                size: 22
                                color: _detailShelfColor
                            }
                        }

                        Column {
                            width: parent.width - 44 - Theme.space.md - _closeDetailBtn.width - Theme.space.md
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: root.viewModel && root.viewModel.selectedShelf
                                      ? root.viewModel.selectedShelf.name
                                      : ""
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightSemibold
                                elide: Text.ElideRight
                                width: parent.width
                            }
                            Text {
                                text: root.viewModel && root.viewModel.selectedShelf
                                      ? (root.viewModel.selectedShelf.description.length > 0
                                          ? root.viewModel.selectedShelf.description
                                          : "%1 book%2".arg(root.viewModel.selectedShelfBooks.length)
                                                       .arg(root.viewModel.selectedShelfBooks.length === 1 ? "" : "s"))
                                      : ""
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                elide: Text.ElideRight
                                width: parent.width
                            }
                        }

                        // Add book + close
                        Row {
                            id: _closeDetailBtn
                            spacing: 0
                            anchors.verticalCenter: parent.verticalCenter

                            SecondaryButton {
                                text: "Add book"
                                iconName: "add"
                                iconPosition: "leading"
                                onClicked: {
                                    if (root.viewModel && root.viewModel.selectedShelf) {
                                        _bookPicker.openForShelf(root.viewModel.selectedShelf.id,
                                                                  root.viewModel.selectedShelf.name)
                                    }
                                }
                            }

                            IconButton {
                                iconName: "close"
                                iconColor: Theme.color.textSecondary
                                hoverIconColor: Theme.color.textPrimary
                                onClicked: {
                                    if (root.viewModel) root.viewModel.selectShelf("")
                                }
                            }
                        }
                    }

                    Divider { orientation: "horizontal" }

                    // Books list
                    Column {
                        width: parent.width
                        spacing: Theme.space.sm
                        visible: root.viewModel && root.viewModel.selectedShelfBooks.length > 0

                        Repeater {
                            model: root.viewModel ? root.viewModel.selectedShelfBooks : []
                            delegate: Item {
                                width: parent.width
                                height: _bookRow.height

                                BookRow {
                                    id: _bookRow
                                    anchors.left: parent.left
                                    anchors.right: _removeBookBtn.left
                                    anchors.rightMargin: Theme.space.md
                                    book: modelData
                                    showActions: false
                                    onClicked: {
                                        // Shelves can contain any book (owned or not).
                                        // Defer to the parent's routing logic.
                                        root.bookDetailRequested(modelData.id)
                                    }
                                }

                                IconButton {
                                    id: _removeBookBtn
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    iconName: "delete_outline"
                                    iconColor: Theme.color.textMuted
                                    hoverIconColor: Theme.color.error
                                    onClicked: {
                                        if (root.viewModel && root.viewModel.selectedShelf) {
                                            root.viewModel.removeBookFromShelf(
                                                root.viewModel.selectedShelf.id, modelData.id)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Empty shelf — no books yet
                    EmptyIllustration {
                        width: parent.width
                        height: 200
                        visible: root.viewModel && root.viewModel.selectedShelf
                                 && root.viewModel.selectedShelfBooks.length === 0
                        iconName: "menu_book"
                        title: "This shelf is empty"
                        description: "Add books from any book detail page using the shelf action."
                        primaryActionLabel: "Browse books"
                        onPrimaryActionTriggered: {
                            if (root.viewModel) root.viewModel.selectShelf("")
                        }
                    }
                }
            }

            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // -------------------------------------------------------------------------
    //  Internal: resolve the selected shelf's color for the detail header
    // -------------------------------------------------------------------------
    readonly property color _detailShelfColor: root.viewModel
                                               && root.viewModel.selectedShelf
                                               && root.viewModel.selectedShelf.color
                                                  ? root.viewModel.selectedShelf.color
                                                  : Theme.color.accent

    property string _currentColorShelfId: ""

    // -------------------------------------------------------------------------
    //  Rename dialog (inline Popup)
    // -------------------------------------------------------------------------
    Popup {
        id: _renameDialog
        property string shelfId: ""
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: 420
        height: _renameCol.implicitHeight + 2 * Theme.space.xxl
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

            Row {
                width: parent.width
                spacing: Theme.space.md

                Rectangle {
                    width: 44
                    height: 44
                    radius: Theme.radius.md
                    color: Theme.color.accentSoft
                    anchors.verticalCenter: parent.verticalCenter

                    AppIcon {
                        anchors.centerIn: parent
                        name: "edit"
                        size: 22
                        color: Theme.color.accent
                    }
                }

                Text {
                    text: "Rename shelf"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHeadline
                    font.weight: Theme.font.weightSemibold
                    anchors.verticalCenter: parent.verticalCenter
                }
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

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.durationFast; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: Theme.motion.durationBase; easing.type: Easing.OutBack }
            }
        }
        exit: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.motion.durationFast; easing.type: Easing.InQuad }
                NumberAnimation { property: "scale"; from: 1; to: 0.95; duration: Theme.motion.durationFast; easing.type: Easing.InQuad }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Color picker popup (used by "Set color" context menu action)
    // -------------------------------------------------------------------------
    Popup {
        id: _colorPickerPopup
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        width: 360
        height: _colorCol.implicitHeight + 2 * Theme.space.xxl
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

        Column {
            id: _colorCol
            anchors.fill: parent
            anchors.margins: Theme.space.xxl
            spacing: Theme.space.lg

            Text {
                text: "Set shelf color"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightSemibold
            }

            // Grid of swatches
            Grid {
                width: parent.width
                columns: 4
                spacing: Theme.space.md

                Repeater {
                    model: Theme.accentPalette
                    delegate: Item {
                        width: (parent.width - 3 * Theme.space.md) / 4
                        height: width

                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radius.md
                            color: modelData.color
                            border.color: _swatchMa2.containsMouse ? Theme.color.textPrimary : "transparent"
                            border.width: 2
                            scale: _swatchMa2.containsMouse ? 1.06 : 1.0
                            Behavior on scale {
                                NumberAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutBack }
                            }
                        }

                        MouseArea {
                            id: _swatchMa2
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.viewModel && root._currentColorShelfId.length > 0) {
                                    root.viewModel.setShelfColor(root._currentColorShelfId, modelData.color)
                                }
                                _colorPickerPopup.close()
                            }
                        }
                    }
                }
            }

            Row {
                width: parent.width
                layoutDirection: Qt.RightToLeft
                SecondaryButton {
                    text: "Cancel"
                    onClicked: _colorPickerPopup.close()
                }
            }
        }

        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.durationFast; easing.type: Easing.OutCubic }
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: Theme.motion.durationBase; easing.type: Easing.OutBack }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Delete confirmation dialog (danger style)
    // -------------------------------------------------------------------------
    ConfirmDialog {
        id: _deleteDialog
        property string shelfId: ""
        property string shelfName: ""
    }

    // -------------------------------------------------------------------------
    //  Public function: scroll to the create form (used by empty state)
    // -------------------------------------------------------------------------
    function scrollToCreateForm() {
        _flickable.contentY = _createCard.y - Theme.space.xl
    }

    // -------------------------------------------------------------------------
    //  Book-picker popup — lets the user search the catalog and add books to
    //  the selected shelf. Books already on the shelf are shown with a check
    //  and can be removed by clicking again.
    // -------------------------------------------------------------------------
    Popup {
        id: _bookPicker
        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: Math.min(560, parent.width - 64)
        height: Math.min(620, parent.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        property string _shelfId: ""
        property string _shelfName: ""
        property string _search: ""

        function openForShelf(shelfId, shelfName) {
            _bookPicker._shelfId = shelfId
            _bookPicker._shelfName = shelfName
            _bookPicker._search = ""
            open()
        }

        background: Card {
            radius: Theme.radius.lg
            elevation: "xl"
            bordered: false
        }

        Column {
            anchors.fill: parent
            spacing: Theme.space.md

            SectionHeader {
                width: parent.width
                title: "Add books to shelf"
                subtitle: _bookPicker._shelfName.length > 0 ? _bookPicker._shelfName : "Select books to add"
            }

            // Search field
            SearchField {
                width: parent.width
                placeholder: "Search by title or author…"
                text: _bookPicker._search
                onTextEdited: _bookPicker._search = newText
            }

            // Book list
            ScrollView {
                width: parent.width
                height: parent.height - 130
                clip: true
                contentWidth: availableWidth

                ListView {
                    width: parent.width
                    anchors.fill: parent
                    model: {
                        // Pull the catalog from BookService and filter by search query.
                        // We use bestsellers() as the source since it returns a broad
                        // QObject* list suitable for BookCover.
                        const all = BookService.bestsellers || []
                        const q = _bookPicker._search.trim().toLowerCase()
                        if (q.length === 0) return all
                        const filtered = []
                        for (let i = 0; i < all.length; ++i) {
                            const b = all[i]
                            const hay = ((b.title || "") + " " + (b.authorName || "")).toLowerCase()
                            if (hay.indexOf(q) >= 0) filtered.push(b)
                        }
                        return filtered
                    }
                    spacing: Theme.space.xs

                    delegate: Rectangle {
                        width: parent.width
                        height: 56
                        radius: Theme.radius.md
                        color: _pickHover.hovered ? Theme.color.fieldFilled : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
                        HoverHandler { id: _pickHover; cursorShape: Qt.PointingHandCursor }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.md
                            anchors.rightMargin: Theme.space.md
                            spacing: Theme.space.md

                            BookCover {
                                width: 32; height: 44
                                book: modelData
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Column {
                                width: parent.width - 32 - Theme.space.md - 32 - Theme.space.md
                                spacing: 1
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    width: parent.width
                                    text: modelData.title || ""
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.authorName || ""
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                }
                            }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }

                            // Add/remove toggle
                            IconButton {
                                iconName: root.viewModel && root.viewModel.selectedShelf && root.viewModel.selectedShelf.bookIds && root.viewModel.selectedShelf.bookIds.indexOf(modelData.id) >= 0
                                           ? "check_circle" : "add_circle"
                                iconColor: root.viewModel && root.viewModel.selectedShelf && root.viewModel.selectedShelf.bookIds && root.viewModel.selectedShelf.bookIds.indexOf(modelData.id) >= 0
                                            ? Theme.color.success : Theme.color.accent
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: {
                                    if (root.viewModel && _bookPicker._shelfId.length > 0) {
                                        const onShelf = root.viewModel.selectedShelf && root.viewModel.selectedShelf.bookIds && root.viewModel.selectedShelf.bookIds.indexOf(modelData.id) >= 0
                                        if (onShelf) {
                                            root.viewModel.removeBookFromShelf(_bookPicker._shelfId, modelData.id)
                                        } else {
                                            root.viewModel.addBookToShelf(_bookPicker._shelfId, modelData.id)
                                        }
                                    }
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
                SecondaryButton { text: "Done"; onClicked: _bookPicker.close() }
            }
        }
    }
}
