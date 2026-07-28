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
//
//  v2 polish: switched all manual width arithmetic to Layouts, gave every
//  room card an explicit Join/Enter button, made the chat history auto-scroll
//  to the newest message, and fixed the InputField text-binding bug in the
//  compose row (every keystroke was being lost).
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
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

    // Bug 6 (QML polish): busy overlay. Shown whenever the viewModel is
    // fetching data (e.g. refreshing the room list after a create). The
    // BusyIndicator is part of QtQuick.Controls, so no new QML module
    // import is required.
    BusyIndicator {
        id: _busyOverlay
        anchors.centerIn: parent
        running: page.viewModel ? page.viewModel.isBusy : false
        visible: running
        z: 1000
    }

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

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header row (title + prominent Create room CTA) -----
            Card {
                Layout.fillWidth: true
                elevation: "none"
                bordered: true
                padding: Theme.space.lg

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    ColumnLayout {
                        spacing: 0
                        Layout.alignment: Qt.AlignVCenter
                        Text {
                            text: "Group reading"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "Read together, in sync."
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }

                    Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                    // POLISH: prominent Create room CTA — right-aligned with
                    // the full Theme.size.buttonHeight so it stands out as
                    // the primary action on this page.
                    PrimaryButton {
                        text: "Create room"
                        iconName: "add"
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredHeight: Theme.size.buttonHeight
                        onClicked: _createRoomDialog.open()
                    }
                }
            }

            // ----- Active rooms grid -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: "Active reading rooms"
                        subtitle: "%1 rooms — sorted by activity".arg(_rooms.length)
                    }

                    // 2-column grid of room cards — GridLayout handles the
                    // flow so we don't need any cellWidth / cellHeight math.
                    GridLayout {
                        Layout.fillWidth: true
                        visible: _rooms.length > 0
                        columns: 2
                        columnSpacing: Theme.space.md
                        rowSpacing: Theme.space.md

                        Repeater {
                            model: _rooms
                            delegate: Card {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 220
                                elevation: "sm"
                                bordered: false
                                padding: Theme.space.lg

                                ColumnLayout {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    // Top row: color stripe + room name + live badge
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space.sm

                                        Rectangle {
                                            width: 4
                                            height: 32
                                            radius: 2
                                            color: model.color
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                        Text {
                                            text: model.name
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                        // Live / Idle badge
                                        Rectangle {
                                            width: 70
                                            height: 22
                                            radius: 11
                                            color: model.live ? Theme.color.successSoft : Theme.color.fieldFilled
                                            Layout.alignment: Qt.AlignVCenter
                                            Row {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Rectangle {
                                                    width: 6
                                                    height: 6
                                                    radius: 3
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

                                    // Book title
                                    Text {
                                        text: model.bookTitle
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    // Reading progress bar
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 6
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

                                    // Stats row: page count + reader count (member count)
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space.sm
                                        Text {
                                            text: "Page %1 of %2".arg(model.page).arg(model.pageCount)
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                        Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
                                        Text {
                                            text: "%1 / %2 readers".arg(model.participants).arg(model.capacity)
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightMedium
                                        }
                                    }

                                    Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xs }

                                    // Bottom row: host + privacy + Join/Enter button
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space.sm

                                        Text {
                                            text: "Hosted by %1".arg(model.host)
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        Rectangle {
                                            width: 60
                                            height: 22
                                            radius: 11
                                            color: model.privacy === "public" ? Theme.color.infoSoft : Theme.color.warningSoft
                                            Layout.alignment: Qt.AlignVCenter
                                            Text {
                                                anchors.centerIn: parent
                                                text: model.privacy === "public" ? "Public" : "Private"
                                                color: model.privacy === "public" ? Theme.color.info : Theme.color.warning
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeMicro
                                                font.weight: Theme.font.weightBold
                                            }
                                        }

                                        Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }

                                        // POLISH: explicit Join / Enter button on every
                                        // card — replaces the previous "click anywhere on
                                        // the card" MouseArea so the action is discoverable.
                                        PrimaryButton {
                                            text: page._selectedRoomIndex === index ? "Enter" : "Join"
                                            iconName: page._selectedRoomIndex === index ? "login" : "group_add"
                                            Layout.preferredHeight: Theme.size.buttonHeightSm
                                            Layout.alignment: Qt.AlignVCenter
                                            onClicked: page._joinRoom(index)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    EmptyState {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
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
                Layout.fillWidth: true
                padding: Theme.space.xl
                visible: _rooms.length > 0

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        Layout.fillWidth: true
                        title: page._selectedRoom ? page._selectedRoom.name : ""
                        subtitle: page._selectedRoom ? "Reading: " + page._selectedRoom.bookTitle : ""
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.lg

                        // Left: synchronized progress
                        ColumnLayout {
                            Layout.fillWidth: true
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
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40
                                    spacing: Theme.space.md

                                    Rectangle {
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: modelData.color || Theme.color.accent
                                        Layout.alignment: Qt.AlignVCenter
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
                                        Layout.preferredWidth: 100
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    // Mini progress — fills the leftover width
                                    Item {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 6
                                        Layout.alignment: Qt.AlignVCenter
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
                                        Layout.preferredWidth: 60
                                        horizontalAlignment: Text.AlignRight
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.sm }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.md

                                PrimaryButton {
                                    text: "Open in reader"
                                    iconName: "menu_book"
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: {
                                        if (page._selectedRoom) {
                                            page.openReaderRequested(page._selectedRoom.bookId)
                                        }
                                    }
                                }
                                SecondaryButton {
                                    text: "Invite friends"
                                    iconName: "person_add"
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: _inviteDialog.open()
                                }
                                SecondaryButton {
                                    text: "Shared notes"
                                    iconName: "sticky_note_2"
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: _notesPopup.open()
                                }
                                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
                            }
                        }

                        // Right: chat
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md

                            Text {
                                text: "Room chat"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightBold
                            }

                            // Chat history — scrollable ListView
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 360
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

                                    // Auto-scroll to the newest message
                                    onCountChanged: Qt.callLater(function() {
                                        positionViewAtEnd()
                                    })

                                    delegate: RowLayout {
                                        width: _chatList.width - anchors.margins * 2
                                        spacing: Theme.space.sm
                                        layoutDirection: model.self ? Qt.RightToLeft : Qt.LeftToRight

                                        Rectangle {
                                            width: 28
                                            height: 28
                                            radius: 14
                                            color: model.color
                                            Layout.alignment: Qt.AlignTop
                                            Text {
                                                anchors.centerIn: parent
                                                text: model.initials
                                                color: Theme.color.textOnAccent
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                font.weight: Theme.font.weightBold
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            RowLayout {
                                                Layout.fillWidth: true
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
                                                    Layout.alignment: Qt.AlignVCenter
                                                }
                                                Item { Layout.fillWidth: true; Layout.preferredHeight: 1 }
                                            }

                                            Rectangle {
                                                Layout.preferredWidth: Math.min(_msgText.implicitWidth + 2 * Theme.space.md, parent.width)
                                                Layout.preferredHeight: _msgText.implicitHeight + 2 * Theme.space.sm
                                                radius: Theme.radius.md
                                                color: model.self ? Theme.color.accent : Theme.color.cardBackground
                                                border.color: model.self ? "transparent" : Theme.color.border
                                                border.width: 1

                                                Text {
                                                    id: _msgText
                                                    anchors.fill: parent
                                                    anchors.margins: Theme.space.sm
                                                    text: model.text
                                                    color: model.self ? Theme.color.textOnAccent : Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBody
                                                    wrapMode: Text.WordWrap
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Compose row — message input pinned at the bottom
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm

                                InputField {
                                    id: _chatInput
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: Theme.size.fieldHeight
                                    placeholder: "Type a message…"
                                    text: page._chatInput
                                    // BUG FIX: capture the signal's `newText` parameter
                                    // explicitly. The InputField component deliberately
                                    // does NOT update its own `text` property on user
                                    // typing (see InputField.qml for the long comment),
                                    // so writing `onTextEdited: page._chatInput = text`
                                    // resolved `text` to InputField.text — which still
                                    // held the OLD value — and the keystroke was lost.
                                    // Using the named parameter `newText` reads the new
                                    // value the signal was emitted with.
                                    onTextEdited: function(newText) { page._chatInput = newText }
                                    onAccepted: page._sendMessage()
                                }
                                IconButton {
                                    iconName: "send"
                                    Layout.preferredWidth: Theme.size.fieldHeight
                                    Layout.preferredHeight: Theme.size.fieldHeight
                                    Layout.alignment: Qt.AlignVCenter
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
    // v10: the dialog now calls viewModel.createRoom(...) directly and emits
    // either `roomCreated` (success — show a toast) or `createFailed` (error —
    // show an error toast; the dialog stays open for retry).
    GroupReadingCreateRoomDialog {
        id: _createRoomDialog
        viewModel: page.viewModel
        onRoomCreated: function(room) {
            page.toastRequested("success", "Room created",
                                 "'" + room.name + "' is now live. Invite friends to start reading together.")
        }
        onCreateFailed: function(reason) {
            page.toastRequested("error", "Couldn't create room", reason)
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
