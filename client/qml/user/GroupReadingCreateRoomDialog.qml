// =============================================================================
//  GroupReadingCreateRoomDialog.qml
// =============================================================================
//  Modal popup for creating a new group-reading room. The user picks:
//    • Room name (required, 3-50 chars)
//    • Book (dropdown of the catalog — sourced from BookService)
//    • Privacy (public / private)
//    • Capacity (2-20, default 8)
//
//  v11 flow: on submit the dialog calls `viewModel.createRoom(...)` and
//  listens to the viewModel's `roomCreated(roomId)` / `roomCreateFailed(reason)`
//  signals (Bug 6). Previously the dialog detected success by checking
//  whether `viewModel.rooms` grew after the synchronous call — fragile
//  because createRoom auto-joins the new room, which can grow the rooms
//  list from a stale cached fetch. The explicit signals are deterministic.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/data"
import "../components/feedback"
import BookClub.Services 1.0

Popup {
    id: dialog
    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: Math.min(520, parent.width - 64)
    height: Math.min(620, parent.height - 64)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.space.xl

    signal roomCreated(var room)
    signal createFailed(string reason)

    property var viewModel: null   // StudySessionViewModel — used to call createRoom(...) directly

    property string _name: ""
    property string _bookId: ""
    property string _bookTitle: ""
    property string _privacy: "public"
    property int _capacity: 8

    // Bug 6: capture the inputs we just submitted so the
    // roomCreated/roomCreateFailed handlers know which room to report.
    property string _pendingName: ""
    property string _pendingBookId: ""
    property string _pendingBookTitle: ""
    property string _pendingPrivacy: "public"
    property int _pendingCapacity: 8

    // Book catalog — pulled from the BookService singleton. We invoke
    // `bestsellers()` (a Q_INVOKABLE method) as the source of book options.
    // In a real app this would be a searchable picker.
    // BUG FIX: `bestsellers` without `()` evaluates to `undefined` in QML
    // because it's a Q_INVOKABLE method, not a Q_PROPERTY. Added `()` to
    // actually invoke the method so the book picker shows real books.
    readonly property var _books: BookService.bestsellers() || []

    function _reset() {
        // v10: clear `_nameField.text` directly because the publisher-style
        // `onTextEdited` handler destroys the `text: dialog._name` binding
        // after the first keystroke — so writing to `dialog._name` no longer
        // propagates back to the field.
        _name = ""
        _nameField.text = ""
        _bookId = _books.length > 0 ? _books[0].id : ""
        _bookTitle = _books.length > 0 ? _books[0].title : ""
        _privacy = "public"
        _capacity = 8
    }

    onAboutToShow: _reset()

    // Bug 6: listen to the viewModel's deterministic success/failure
    // signals. We use `Connections` with the explicit function-on-foo
    // syntax so the handler names survive Qt6's implicit-signal lookup.
    Connections {
        target: dialog.viewModel
        ignoreUnknownSignals: true

        function onRoomCreated(roomId) {
            // Only react if this dialog is open and we have a pending submit.
            if (!dialog.visible || dialog._pendingName.length === 0) return
            dialog.roomCreated({
                id: roomId,
                name: dialog._pendingName,
                bookId: dialog._pendingBookId,
                bookTitle: dialog._pendingBookTitle,
                privacy: dialog._pendingPrivacy,
                capacity: dialog._pendingCapacity
            })
            dialog._pendingName = ""
            dialog.close()
        }

        function onRoomCreateFailed(reason) {
            if (!dialog.visible || dialog._pendingName.length === 0) return
            dialog.createFailed(reason && reason.length > 0
                                ? reason
                                : "Could not create the room. Please try again.")
            // Keep the dialog open so the user can retry. Clear the pending
            // state so a *later* roomCreated (e.g. from another tab) doesn't
            // fire this handler again.
            dialog._pendingName = ""
        }
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
            title: "Create a reading room"
            subtitle: "Read a book together, in sync"
        }

        ScrollView {
            width: parent.width
            height: parent.height - 130
            clip: true
            contentWidth: availableWidth

            Column {
                width: parent.width
                spacing: Theme.space.md

                // Room name
                Text { text: "Room name"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                InputField {
                    id: _nameField
                    width: parent.width
                    placeholder: "Friday night book club"
                    text: dialog._name
                    // v10 fix (same bug as publisher pages): write typed text
                    // back to the InputField's `text` property so the dialog's
                    // enabled-checks and submit handler read non-empty values.
                    // We also keep `dialog._name` in sync so external readers
                    // (and _reset()) stay correct.
                    onTextEdited: function(newText) {
                        _nameField.text = newText
                        dialog._name = newText
                    }
                    maximumLength: 50
                }

                // Book picker
                Text { text: "Book"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                ComboBox {
                    id: _bookCombo
                    width: parent.width
                    height: Theme.size.fieldHeight
                    model: dialog._books
                    textRole: "title"
                    valueRole: "id"
                    currentIndex: 0
                    onActivated: {
                        if (currentIndex >= 0 && currentIndex < dialog._books.length) {
                            dialog._bookId = dialog._books[currentIndex].id
                            dialog._bookTitle = dialog._books[currentIndex].title
                        }
                    }
                    delegate: ItemDelegate {
                        width: _bookCombo.width
                        text: modelData.title
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        highlighted: _bookCombo.highlightedIndex === index
                    }
                    background: Rectangle {
                        radius: Theme.radius.md
                        color: Theme.color.fieldBackground
                        border.color: _bookCombo.activeFocus ? Theme.color.accent : Theme.color.border
                        border.width: _bookCombo.activeFocus ? 2 : 1
                    }
                }

                // Privacy
                Text { text: "Privacy"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                Row {
                    width: parent.width
                    spacing: Theme.space.sm
                    Repeater {
                        model: [
                            { key: "public",  label: "Public",  icon: "public",    desc: "Anyone with the link can join" },
                            { key: "private", label: "Private", icon: "lock",      desc: "Invite-only" }
                        ]
                        Rectangle {
                            width: (parent.width - Theme.space.sm) / 2
                            height: 64
                            radius: Theme.radius.md
                            color: dialog._privacy === modelData.key ? Theme.color.accentSoft : Theme.color.fieldBackground
                            border.color: dialog._privacy === modelData.key ? Theme.color.accent : Theme.color.border
                            border.width: dialog._privacy === modelData.key ? 2 : 1

                            MouseArea {
                                anchors.fill: parent
                                onClicked: dialog._privacy = modelData.key
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.label
                                    color: dialog._privacy === modelData.key ? Theme.color.accent : Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightBold
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.desc
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeMicro2
                                }
                            }
                        }
                    }
                }

                // Capacity
                Text { text: "Capacity: %1 readers".arg(dialog._capacity); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                Slider {
                    id: _capSlider
                    width: parent.width
                    from: 2
                    to: 20
                    stepSize: 1
                    value: dialog._capacity
                    onValueChanged: dialog._capacity = Math.round(value)
                }
                Row {
                    width: parent.width
                    Text { text: "2"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                    Item { width: 1; height: 1 }
                    Text { text: "20"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                }
            }
        }

        RowLayout {
            width: parent.width
            spacing: Theme.space.md
            Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
            SecondaryButton { text: "Cancel"; Layout.alignment: Qt.AlignVCenter; onClicked: dialog.close() }
            PrimaryButton {
                text: "Create room"
                iconName: "check"
                Layout.alignment: Qt.AlignVCenter
                enabled: _nameField.text.trim().length >= 3 && dialog._bookId.length > 0
                onClicked: {
                    // Bug 6: capture the pending submit, call the VM, and let
                    // the Connections handler above react to the VM's
                    // roomCreated / roomCreateFailed signals. No more
                    // before/after list-length sniffing.
                    if (!dialog.viewModel) {
                        dialog.createFailed("No view model")
                        return
                    }
                    var name = _nameField.text.trim()
                    dialog._pendingName      = name
                    dialog._pendingBookId    = dialog._bookId
                    dialog._pendingBookTitle = dialog._bookTitle
                    dialog._pendingPrivacy   = dialog._privacy
                    dialog._pendingCapacity  = dialog._capacity
                    dialog.viewModel.createRoom(name,
                                                dialog._bookId,
                                                dialog._bookTitle,
                                                dialog._privacy,
                                                dialog._capacity)
                }
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
