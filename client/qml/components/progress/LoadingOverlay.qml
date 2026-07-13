// =============================================================================
//  LoadingOverlay.qml
// =============================================================================
//  Full-card translucent overlay with centered spinner + label. Used to block
//  the form during async ViewModel operations (login, register, etc.).
//
//  Public API:
//      active : bool   — show/hide
//      label  : string — message under the spinner
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property bool active: false
    property string label: "Please wait…"

    anchors.fill: parent
    visible: active
    z: Theme.z.modal

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(Theme.color.cardBackground.r, Theme.color.cardBackground.g, Theme.color.cardBackground.b, 0.78)
        radius: parent && parent.radius ? parent.radius : 0

        Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.space.md

        Spinner {
            anchors.horizontalCenter: parent.horizontalCenter
            size: 32
            color: Theme.color.primary
            progress: -1
        }

        Text {
            text: root.label
            color: Theme.color.textSecondary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightMedium
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Behavior on opacity { NumberAnimation { duration: Theme.motion.durationFast } }
}
