// =============================================================================
//  GroupReadingPage.qml
// =============================================================================
//  Group reading UI for the regular User role. Lets users:
//      • Browse active reading rooms
//      • Create a new room (book + invitees + privacy)
//      • Open a room: see who's online, where everyone is in the book,
//        live chat, and synchronized "Turn the page" pulses.
//
//  All data is mocked locally — the real backend would broadcast over the
//  StudySession socket protocol (see common/Network/Protocol.h).
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
    anchors.fill: parent

    signal toastRequested(string variant, string title, string description)
    signal openReaderRequested(string bookId)

    property var viewModel: null   // StudySessionViewModel

    // ----- Data now comes from the viewModel -----
    readonly property var _rooms: page.viewModel ? page.viewModel.rooms : []
    readonly property var _chat: page.viewModel ? page.viewModel.chatMessages : []

    property int _selectedRoomIndex: 0
    property var _selectedRoom: _rooms.length > 0 ? _rooms[_selectedRoomIndex] : null
    property string _chatInput: ""

    function _joinRoom(idx) {
        _selectedRoomIndex = idx
        if (page.viewModel && _rooms.length > idx) {
            page.viewModel.joinRoom(_rooms[idx].id || "")
        }
        page.toastRequested("success", "Joined room", "You're now reading with " + (_rooms.length > idx ? _rooms[idx].participants : 0) + " others.")
    }

    function _sendMessage() {
        if (_chatInput.trim().length === 0) return
        if (page.viewModel) page.viewModel.sendMessage(_chatInput.trim())
        _chatInput = ""
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header row -----
            Card {
                width: parent.width
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                Row {
                    width: parent.width
                    spacing: Theme.space.md

                    Column {
                        spacing: 0
                        anchors.verticalCenter: parent.verticalCenter
                        Text { text: "Group reading"; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold }
                        Text { text: "Read together, in sync."; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    PrimaryButton {
                        text: "Create room"
                        iconName: "add"
                        onClicked: _createRoomDialog.open()
                    }
                }
            }

            // ----- Active rooms grid -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Active reading rooms"
                        subtitle: "%1 rooms — sorted by activity".arg(_rooms.length)
                    }

                    // 2-column grid of room cards
                    GridView {
                        id: _grid
                        width: parent.width
                        height: Math.ceil(_rooms.length / 2) * 200 + Theme.space.md
                        clip: true
                        interactive: false
                        cellWidth: parent.width / 2 - Theme.space.md / 2
                        cellHeight: 200
                        model: _rooms
                        spacing: Theme.space.md

                        delegate: Card {
                            width: _grid.cellWidth
                            height: _grid.cellHeight - Theme.space.md
                            elevation: "sm"
                            bordered: false
                            padding: Theme.space.lg

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: page._joinRoom(index)
                            }

                            Column {
                                anchors.fill: parent
                                spacing: Theme.space.sm

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Rectangle {
                                        width: 4; height: 32; radius: 2
                                        color: model.color
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: model.name
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightBold
                                        elide: Text.ElideRight
                                        width: parent.width - 4 - Theme.space.sm - 80
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Rectangle {
                                        width: 70; height: 22; radius: 11
                                        color: model.live ? Theme.color.successSoft : Theme.color.fieldFilled
                                        anchors.verticalCenter: parent.verticalCenter
                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 4
                                            Rectangle {
                                                width: 6; height: 6; radius: 3
                                                color: model.live ? Theme.color.success : Theme.color.textMuted
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            Text {
                                                text: model.live ? "Live" : "Idle"
                                                color: model.live ? Theme.color.success : Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                font.weight: Theme.font.weightBold
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                        }
                                    }
                                }

                                Text {
                                    text: model.bookTitle
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    elide: Text.ElideRight
                                    width: parent.width
                                }

                                Item { width: 1; height: 4 }

                                // Reading progress bar
                                Rectangle {
                                    width: parent.width
                                    height: 6
                                    radius: 3
                                    color: Theme.color.fieldFilled
                                    Rectangle {
                                        width: parent.width * (model.pageCount > 0 ? model.page / model.pageCount : 0)
                                        height: parent.height
                                        radius: parent.radius
                                        color: model.color
                                        Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } }
                                    }
                                }

                                Row {
                                    width: parent.width
                                    Text {
                                        text: "Page %1 of %2".arg(model.page).arg(model.pageCount)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: "%1 / %2 readers".arg(model.participants).arg(model.capacity)
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightMedium
                                    }
                                }

                                Item { width: 1; Layout.fillWidth: true; height: 1 }

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Text {
                                        text: "Hosted by %1".arg(model.host)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Rectangle {
                                        width: 60; height: 22; radius: 11
                                        color: model.privacy === "public" ? Theme.color.infoSoft : Theme.color.warningSoft
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.privacy === "public" ? "Public" : "Private"
                                            color: model.privacy === "public" ? Theme.color.info : Theme.color.warning
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeMicro
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                }
                            }
                        }
                    }

                    EmptyState {
                        width: parent.width
                        height: 200
                        visible: _rooms.length === 0
                        iconName: "groups"
                        title: "No reading rooms yet"
                        description: "Create the first room to start reading with friends."
                        actionLabel: "Create room"
                        onActionTriggered: _createRoomDialog.open()
                    }
                }
            }

            // ----- Selected room detail: synced progress + chat -----
            Card {
                width: parent.width
                padding: Theme.space.xl
                visible: _rooms.length > 0

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: page._selectedRoom ? page._selectedRoom.name : ""
                        subtitle: page._selectedRoom ? "Reading: " + page._selectedRoom.bookTitle : ""
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        // Left: synchronized progress
                        Column {
                            width: parent.width * 0.55 - Theme.space.lg / 2
                            spacing: Theme.space.md

                            Text {
                                text: "Where everyone is"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }

                            // Reader positions — bound to viewModel.participants
                            Repeater {
                                model: page.viewModel ? page.viewModel.participants : []
                                delegate: Row {
                                    width: parent.width
                                    height: 40
                                    spacing: Theme.space.md

                                    Rectangle {
                                        width: 28; height: 28; radius: 14
                                        color: modelData.color || Theme.color.accent
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: (modelData.name || "?").charAt(0)
                                            color: Theme.color.textOnAccent
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                        }
                                    }

                                    Text {
                                        text: modelData.name || "Anonymous"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: (modelData.name === "You" || modelData.isYou === true) ? Theme.font.weightBold : Theme.font.weightRegular
                                        width: 100
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    // Mini progress
                                    Item {
                                        width: parent.width - 28 - 100 - 60 - Theme.space.md * 3
                                        height: 6
                                        anchors.verticalCenter: parent.verticalCenter
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 3
                                            color: Theme.color.fieldFilled
                                        }
                                        Rectangle {
                                            width: parent.width * (page._selectedRoom && page._selectedRoom.pageCount > 0 ? (modelData.page || 0) / page._selectedRoom.pageCount : 0)
                                            height: parent.height
                                            radius: 3
                                            color: modelData.color || Theme.color.accent
                                            Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } }
                                        }
                                    }

                                    Text {
                                        text: "p. %1".arg(modelData.page || 0)
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        width: 60
                                        horizontalAlignment: Text.AlignRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            Item { width: 1; height: Theme.space.sm }

                            Row {
                                width: parent.width
                                spacing: Theme.space.md

                                PrimaryButton {
                                    text: "Open in reader"
                                    iconName: "menu_book"
                                    onClicked: {
                                        if (page._selectedRoom) {
                                            page.openReaderRequested(page._selectedRoom.bookId)
                                        }
                                    }
                                }
                                SecondaryButton {
                                    text: "Invite friends"
                                    iconName: "person_add"
                                    onClicked: _inviteDialog.open()
                                }
                                SecondaryButton {
                                    text: "Shared notes"
                                    iconName: "sticky_note_2"
                                    onClicked: _notesPopup.open()
                                }
                            }
                        }

                        // Right: chat
                        Column {
                            width: parent.width * 0.45 - Theme.space.lg / 2
                            spacing: Theme.space.md

                            Text {
                                text: "Room chat"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }

                            // Chat history
                            Rectangle {
                                width: parent.width
                                height: 280
                                radius: Theme.radius.lg
                                color: Theme.color.fieldFilled
                                border.color: Theme.color.border
                                border.width: 1

                                ListView {
                                    id: _chatList
                                    anchors.fill: parent
                                    anchors.margins: Theme.space.md
                                    clip: true
                                    model: _chat
                                    spacing: Theme.space.sm

                                    delegate: Row {
                                        width: parent.width
                                        spacing: Theme.space.sm
                                        layoutDirection: model.self ? Qt.RightToLeft : Qt.LeftToRight

                                        Rectangle {
                                            width: 28; height: 28; radius: 14
                                            color: model.color
                                            anchors.verticalCenter: parent.verticalCenter
                                            Text {
                                                anchors.centerIn: parent
                                                text: model.initials
                                                color: Theme.color.textOnAccent
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                font.weight: Theme.font.weightBold
                                            }
                                        }

                                        Column {
                                            width: parent.width - 28 - Theme.space.sm
                                            spacing: 2

                                            Row {
                                                spacing: 6
                                                layoutDirection: model.self ? Qt.RightToLeft : Qt.LeftToRight
                                                Text {
                                                    text: model.user
                                                    color: Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeCaption
                                                    font.weight: Theme.font.weightBold
                                                }
                                                Text {
                                                    text: model.time
                                                    color: Theme.color.textMuted
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeMicro
                                                    anchors.verticalCenter: parent.verticalCenter
                                                }
                                            }

                                            Rectangle {
                                                width: Math.min(_msgText.implicitWidth + 2 * Theme.space.md, parent.width)
                                                height: _msgText.implicitHeight + 2 * Theme.space.sm
                                                radius: Theme.radius.md
                                                color: model.self ? Theme.color.accent : Theme.color.cardBackground
                                                border.color: model.self ? "transparent" : Theme.color.border
                                                border.width: 1

                                                Text {
                                                    id: _msgText
                                                    anchors.fill: parent
                                                    anchors.margins: Theme.space.sm
                                                    text: model.text
                                                    color: model.self ? "#FFFFFF" : Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBody
                                                    wrapMode: Text.WordWrap
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Compose row
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                InputField {
                                    id: _chatInput
                                    width: parent.width - 44 - Theme.space.sm
                                    placeholder: "Type a message…"
                                    text: page._chatInput
                                    onTextEdited: page._chatInput = newText
                                    onAccepted: page._sendMessage()
                                }
                                IconButton {
                                    iconName: "send"
                                    width: 44
                                    height: 44
                                    onClicked: page._sendMessage()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ----- Room-creation dialog -----
    GroupReadingCreateRoomDialog {
        id: _createRoomDialog
        onRoomCreated: function(room) {
            if (page.viewModel) {
                page.viewModel.createRoom(room.name, room.bookId, room.bookTitle,
                                          room.privacy, room.capacity)
            }
            page.toastRequested("success", "Room created",
                                 "'" + room.name + "' is now live. Invite friends to start reading together.")
        }
    }

    // ----- Invitations dialog -----
    GroupReadingInviteDialog {
        id: _inviteDialog
        roomName: page._selectedRoom ? page._selectedRoom.name : ""
        onInvitationsSent: function(names) {
            if (names.length === 0) return
            // Forward the invitation list to the VM so it can send them
            // over the StudySession socket protocol (mock: just logs it).
            if (page.viewModel && typeof page.viewModel.inviteUsers === "function") {
                page.viewModel.inviteUsers(names)
            }
            page.toastRequested("success", "Invitations sent",
                                 names.length + " invitation" + (names.length > 1 ? "s" : "") + " sent to " + names.join(", "))
        }
    }

    // ----- Shared notes popup -----
    Popup {
        id: _notesPopup
        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: 540
        height: 600
        modal: true
        focus: true
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Card {
            radius: Theme.radius.xl
            elevation: "xl"
            bordered: false
            backgroundColor: Theme.color.cardBackground
            padding: 0
        }

        GroupReadingNotesPanel {
            anchors.fill: parent
            anchors.margins: Theme.space.lg
            roomName: page._selectedRoom ? page._selectedRoom.name : ""
            currentUser: "You"
            notes: page.viewModel ? page.viewModel.notes : []
            onNoteAdded: function(text, pageNum) {
                if (page.viewModel) page.viewModel.addNote(text, pageNum)
                page.toastRequested("success", "Note added",
                                     "Your note on page " + pageNum + " is now visible to the room.")
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
}
