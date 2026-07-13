// =============================================================================
//  GroupReadingInviteDialog.qml
// =============================================================================
//  Modal popup for inviting friends to a group-reading room. The user enters
//  usernames (comma- or Enter-separated) and the dialog emits
//  `invitationsSent(QStringList usernames)` on submit.
//
//  In a real app this would call StudySessionViewModel.inviteUsers(...) which
//  would POST to /study/invite over the socket. Here it just emits the list.
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

Popup {
    id: dialog
    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: Math.min(480, parent.width - 64)
    height: Math.min(540, parent.height - 64)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: Theme.space.xl

    signal invitationsSent(var usernames)

    property string roomName: ""
    property var _invites: []   // list of username strings

    function _addInvite() {
        const name = _input.text.trim()
        if (name.length === 0) return
        // Reject duplicates
        for (let i = 0; i < dialog._invites.length; ++i) {
            if (dialog._invites[i] === name) {
                _input.text = ""
                return
            }
        }
        dialog._invites = dialog._invites.concat([name])
        _input.text = ""
    }

    function _removeInvite(idx) {
        const copy = dialog._invites.slice()
        copy.splice(idx, 1)
        dialog._invites = copy
    }

    onAboutToShow: dialog._invites = []

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
            title: "Invite friends"
            subtitle: dialog.roomName.length > 0 ? "To: " + dialog.roomName : "Select people to invite"
        }

        // Input row
        Row {
            width: parent.width
            spacing: Theme.space.sm
            InputField {
                id: _input
                width: parent.width - 100 - Theme.space.sm
                placeholder: "@username or email"
                onAccepted: dialog._addInvite()
            }
            PrimaryButton {
                width: 100
                text: "Add"
                iconName: "add"
                enabled: _input.text.trim().length > 0
                onClicked: dialog._addInvite()
            }
        }

        // Invites list
        Text {
            text: "%1 pending invitation%2".arg(dialog._invites.length).arg(dialog._invites.length === 1 ? "" : "s")
            color: Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            visible: dialog._invites.length > 0
        }

        ScrollView {
            width: parent.width
            height: parent.height - 240
            clip: true
            contentWidth: availableWidth
            visible: dialog._invites.length > 0

            ListView {
                width: parent.width
                anchors.fill: parent
                model: dialog._invites
                spacing: Theme.space.xs
                delegate: Rectangle {
                    width: parent.width
                    height: 48
                    radius: Theme.radius.md
                    color: Theme.color.fieldFilled
                    border.color: Theme.color.divider

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.space.md
                        anchors.rightMargin: Theme.space.md
                        spacing: Theme.space.md

                        Rectangle {
                            width: 32; height: 32; radius: 16
                            color: Theme.color.accent
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent
                                text: modelData.charAt(0).toUpperCase()
                                color: Theme.color.textOnAccent
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightBold
                            }
                        }
                        Text {
                            text: modelData
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightMedium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                        IconButton {
                            iconName: "close"
                            iconColor: Theme.color.textMuted
                            anchors.verticalCenter: parent.verticalCenter
                            onClicked: dialog._removeInvite(index)
                        }
                    }
                }
            }
        }

        EmptyState {
            width: parent.width
            height: 180
            visible: dialog._invites.length === 0
            iconName: "person_add"
            title: "No invites yet"
            description: "Add usernames or emails above to invite friends to this room."
        }

        Row {
            width: parent.width
            spacing: Theme.space.md
            Item { width: 1; Layout.fillWidth: true; height: 1 }
            SecondaryButton { text: "Cancel"; onClicked: dialog.close() }
            PrimaryButton {
                text: "Send %1 invitation%2".arg(dialog._invites.length).arg(dialog._invites.length === 1 ? "" : "s")
                iconName: "send"
                enabled: dialog._invites.length > 0
                onClicked: {
                    dialog.invitationsSent(dialog._invites)
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
