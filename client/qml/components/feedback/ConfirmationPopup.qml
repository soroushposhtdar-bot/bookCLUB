// =============================================================================
//  ConfirmationPopup.qml
// =============================================================================
//  Modal confirmation dialog — title + message + (optional) detail + two
//  buttons (cancel / confirm). Use for "Discard changes?", "Logout?", etc.
//
//  Public API:
//      title        : string
//      message      : string
//      detail       : string   (optional muted detail paragraph)
//      confirmLabel : string   (default "Confirm")
//      cancelLabel  : string   (default "Cancel")
//      confirmStyle : string   — "primary" | "danger"
//      icon         : string   (optional leading icon)
//
//  Signals:
//      confirmed()
//      cancelled()
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../surfaces"
import "../"
import "../buttons"

Popup {
    id: root

    property string title: ""
    property string message: ""
    property string detail: ""
    property string confirmLabel: "Confirm"
    property string cancelLabel: "Cancel"
    property string confirmStyle: "primary"   // primary | danger
    property string icon: "warning_amber"
    property int cardWidth: 420

    signal confirmed()
    signal cancelled()

    anchors.centerIn: parent
    width: cardWidth
    padding: 0
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Card {
        radius: Theme.radius.xl
        elevation: "xl"
        bordered: false
        backgroundColor: Theme.color.cardBackground
        padding: 0
    }

    onClosed: cancelled()

    Column {
        id: _column
        anchors.fill: parent
        anchors.margins: Theme.space.xxl
        spacing: Theme.space.lg

        // Icon + Title
        Row {
            width: parent.width
            spacing: Theme.space.md

            Rectangle {
                width: 44
                height: 44
                radius: width / 2
                color: root.confirmStyle === "danger" ? Theme.color.errorSoft
                                                       : Theme.color.warningSoft
                anchors.verticalCenter: parent.verticalCenter

                AppIcon {
                    name: root.icon
                    size: Theme.size.iconLg
                    color: root.confirmStyle === "danger" ? Theme.color.error : Theme.color.warning
                    anchors.centerIn: parent
                }
            }

            Text {
                text: root.title
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightSemibold
                anchors.verticalCenter: parent.verticalCenter
                wrapMode: Text.WordWrap
                width: parent.width - 44 - Theme.space.md
            }
        }

        // Message
        Text {
            text: root.message
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightRegular
            wrapMode: Text.WordWrap
            width: parent.width
            visible: root.message.length > 0
        }

        // Detail (muted)
        Text {
            text: root.detail
            color: Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeSmall
            font.weight: Theme.font.weightRegular
            wrapMode: Text.WordWrap
            width: parent.width
            visible: root.detail.length > 0
        }

        // Spacer
        Item { width: 1; height: Theme.space.xs }

        // Buttons
        Row {
            width: parent.width
            spacing: Theme.space.md
            layoutDirection: Qt.RightToLeft

            PrimaryButton {
                text: root.confirmLabel
                fullWidth: false
                width: (parent.width - Theme.space.md) / 2
                onClicked: {
                    root.confirmed()
                    root.close()
                }
            }

            SecondaryButton {
                text: root.cancelLabel
                width: (parent.width - Theme.space.md) / 2
                onClicked: {
                    root.cancelled()
                    root.close()
                }
            }
        }
    }

    // Scrim animation
    enter: Transition { NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.motion.durationBase } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.motion.durationFast } }
}
