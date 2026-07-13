// =============================================================================
//  ConfirmDialog.qml
// =============================================================================
//  Callback-based confirmation dialog. Drop anywhere, call `open()` with
//  config + callbacks. Auto-closes after a button is clicked.
//
//  Public API:
//      title, message, detail, icon, confirmLabel, cancelLabel, confirmStyle
//
//  Signals:
//      confirmed()
//      cancelled()
//
//  Usage:
//      _dialog.title = "Delete shelf?"
//      _dialog.message = "This action cannot be undone."
//      _dialog.confirmStyle = "danger"
//      _dialog.confirmLabel = "Delete"
//      _dialog.onConfirmed = () => { ... }
//      _dialog.open()
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../../theme"
import "../"
import "../surfaces"
import "../buttons"

Popup {
    id: root

    property string title: "Are you sure?"
    property string message: ""
    property string detail: ""
    property string iconName: "warning_amber"
    property string confirmLabel: "Confirm"
    property string cancelLabel: "Cancel"
    property string confirmStyle: "primary"   // "primary" | "danger"
    property var onConfirmed: null
    property var onCancelled: null

    signal confirmed()
    signal cancelled()

    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: 420
    height: _col.implicitHeight + 2 * Theme.space.xxl
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

    function openDialog(cfg) {
        if (cfg.title)        root.title = cfg.title
        if (cfg.message)      root.message = cfg.message
        if (cfg.detail)       root.detail = cfg.detail
        if (cfg.iconName)     root.iconName = cfg.iconName
        if (cfg.confirmLabel) root.confirmLabel = cfg.confirmLabel
        if (cfg.cancelLabel)  root.cancelLabel = cfg.cancelLabel
        if (cfg.confirmStyle) root.confirmStyle = cfg.confirmStyle
        if (cfg.onConfirmed)  root.onConfirmed = cfg.onConfirmed
        if (cfg.onCancelled)  root.onCancelled = cfg.onCancelled
        open()
    }

    Column {
        id: _col
        anchors.fill: parent
        anchors.margins: Theme.space.xxl
        spacing: Theme.space.lg

        Row {
            width: parent.width
            spacing: Theme.space.md

            Rectangle {
                width: 44; height: 44; radius: 22
                color: root.confirmStyle === "danger" ? Theme.color.errorSoft : Theme.color.warningSoft
                anchors.verticalCenter: parent.verticalCenter
                AppIcon {
                    anchors.centerIn: parent
                    name: root.iconName
                    size: Theme.size.iconLg
                    color: root.confirmStyle === "danger" ? Theme.color.error : Theme.color.warning
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

        Text {
            text: root.message
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            wrapMode: Text.WordWrap
            width: parent.width
            visible: root.message.length > 0
        }

        Text {
            text: root.detail
            color: Theme.color.textMuted
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeSmall
            wrapMode: Text.WordWrap
            width: parent.width
            visible: root.detail.length > 0
        }

        Item { width: 1; height: Theme.space.xs }

        Row {
            width: parent.width
            spacing: Theme.space.md
            layoutDirection: Qt.RightToLeft

            PrimaryButton {
                text: root.confirmLabel
                width: (parent.width - Theme.space.md) / 2
                onClicked: {
                    root.confirmed()
                    if (root.onConfirmed) root.onConfirmed()
                    root.close()
                }
            }
            SecondaryButton {
                text: root.cancelLabel
                width: (parent.width - Theme.space.md) / 2
                onClicked: {
                    root.cancelled()
                    if (root.onCancelled) root.onCancelled()
                    root.close()
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
