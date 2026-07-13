// =============================================================================
//  GroupReadingCreateRoomDialog.qml
// =============================================================================
//  Modal popup for creating a new group-reading room. The user picks:
//    • Room name (required, 3-50 chars)
//    • Book (dropdown of the catalog — sourced from BookService)
//    • Privacy (public / private)
//    • Capacity (2-20, default 8)
//
//  On submit, emits `roomCreated({ name, bookId, bookTitle, privacy, capacity })`
//  which GroupReadingPage forwards to StudySessionViewModel.createRoom(...).
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

    property string _name: ""
    property string _bookId: ""
    property string _bookTitle: ""
    property string _privacy: "public"
    property int _capacity: 8

    // Book catalog — pulled from the BookService singleton. We bind to
    // `bestsellers` (always populated by the store) as the source of book
    // options. In a real app this would be a searchable picker.
    readonly property var _books: BookService.bestsellers || []

    function _reset() {
        _name = ""
        _bookId = _books.length > 0 ? _books[0].id : ""
        _bookTitle = _books.length > 0 ? _books[0].title : ""
        _privacy = "public"
        _capacity = 8
    }

    onAboutToShow: _reset()

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
                    onTextEdited: dialog._name = newText
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
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    Text { text: "20"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                }
            }
        }

        Row {
            width: parent.width
            spacing: Theme.space.md
            Item { width: 1; Layout.fillWidth: true; height: 1 }
            SecondaryButton { text: "Cancel"; onClicked: dialog.close() }
            PrimaryButton {
                text: "Create room"
                iconName: "check"
                enabled: dialog._name.trim().length >= 3 && dialog._bookId.length > 0
                onClicked: {
                    dialog.roomCreated({
                        name: dialog._name.trim(),
                        bookId: dialog._bookId,
                        bookTitle: dialog._bookTitle,
                        privacy: dialog._privacy,
                        capacity: dialog._capacity
                    })
                    dialog.close()
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
