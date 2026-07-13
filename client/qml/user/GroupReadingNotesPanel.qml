// =============================================================================
//  GroupReadingNotesPanel.qml
// =============================================================================
//  Shared-notes panel for a group-reading room. Renders the room's existing
//  notes (each tagged with page + author) and a compose row at the bottom
//  for adding a new note.
//
//  Emits `noteAdded(string text, int page)` when the user submits a new note.
//  GroupReadingPage forwards this to StudySessionViewModel.addNote(...).
//
//  The notes list itself is bound to viewModel.notes (injected via the
//  `notes` property from the parent page).
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

Item {
    id: panel

    property string roomName: ""
    property string currentUser: "You"
    property var notes: []   // bound from parent: viewModel.notes

    signal noteAdded(string text, int page)

    property string _draftText: ""
    property string _draftPage: "1"

    function _submit() {
        const text = panel._draftText.trim()
        const page = parseInt(panel._draftPage) || 1
        if (text.length === 0) return
        panel.noteAdded(text, page)
        panel._draftText = ""
        panel._draftPage = "1"
    }

    Column {
        anchors.fill: parent
        spacing: Theme.space.md

        SectionHeader {
            width: parent.width
            title: "Shared notes"
            subtitle: panel.roomName.length > 0 ? panel.roomName : "Room notes"
        }

        // Notes list
        ScrollView {
            width: parent.width
            height: parent.height - 200
            clip: true
            contentWidth: availableWidth

            ListView {
                width: parent.width
                anchors.fill: parent
                model: panel.notes
                spacing: Theme.space.sm

                delegate: Rectangle {
                    width: parent.width
                    radius: Theme.radius.md
                    color: Theme.color.fieldFilled
                    border.color: Theme.color.divider
                    implicitHeight: _noteCol.implicitHeight + 2 * Theme.space.md

                    Column {
                        id: _noteCol
                        anchors.fill: parent
                        anchors.margins: Theme.space.md
                        spacing: Theme.space.xs

                        Row {
                            width: parent.width
                            spacing: Theme.space.sm

                            // Page badge
                            Rectangle {
                                width: _pageLbl.implicitWidth + 12
                                height: 20
                                radius: 10
                                color: Theme.color.accentSoft
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    id: _pageLbl
                                    anchors.centerIn: parent
                                    text: "p. %1".arg(modelData.page || 1)
                                    color: Theme.color.accent
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeMicro2
                                    font.weight: Theme.font.weightBold
                                }
                            }
                            // Author
                            Text {
                                text: modelData.author || panel.currentUser
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
                            // Timestamp
                            Text {
                                text: modelData.time || "Just now"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text {
                            width: parent.width
                            text: modelData.text || ""
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        EmptyState {
            width: parent.width
            height: 160
            visible: panel.notes.length === 0
            iconName: "sticky_note_2"
            title: "No notes yet"
            description: "Add the first note for this room — it'll be visible to everyone."
        }

        // Compose row
        Rectangle {
            width: parent.width
            height: _composeRow.height + 2 * Theme.space.md
            radius: Theme.radius.md
            color: Theme.color.cardBackground
            border.color: Theme.color.divider

            Column {
                id: _composeRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.space.md
                anchors.rightMargin: Theme.space.md
                spacing: Theme.space.sm

                Row {
                    width: parent.width
                    spacing: Theme.space.sm

                    // Page number input (narrow)
                    InputField {
                        id: _pageField
                        width: 80
                        placeholder: "p."
                        text: panel._draftPage
                        onTextEdited: panel._draftPage = newText
                        inputMethodHints: Qt.ImhDigitsOnly
                        maximumLength: 4
                    }

                    // Note text input
                    InputField {
                        id: _textField
                        width: parent.width - 80 - Theme.space.sm - 100 - Theme.space.sm
                        placeholder: "Add a note for the room…"
                        text: panel._draftText
                        onTextEdited: panel._draftText = newText
                        onAccepted: panel._submit()
                    }

                    PrimaryButton {
                        width: 100
                        text: "Post"
                        iconName: "send"
                        enabled: panel._draftText.trim().length > 0
                        onClicked: panel._submit()
                    }
                }
            }
        }
    }
}
