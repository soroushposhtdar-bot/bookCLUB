// =============================================================================
//  OfflineBanner.qml
// =============================================================================
//  Slim banner that slides down from the top when the app loses connectivity.
//  Stays visible until connectivity is restored.
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Rectangle {
    id: root

    property bool offline: false
    property string message: "You're offline. Some features may be unavailable."

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    anchors.top: parent ? parent.top : undefined
    height: 36
    color: Theme.color.warning
    visible: offline

    Row {
        anchors.centerIn: parent
        spacing: Theme.space.sm

        AppIcon {
            name: "wifi_off"
            size: 18
            color: "#FFFFFF"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.message
            color: "#FFFFFF"
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeCaption
            font.weight: Theme.font.weightMedium
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // Slide-down entrance / slide-up exit
    transform: Translate { id: _tx; y: -root.height }
    states: State {
        name: "visible"
        when: root.offline
        PropertyChanges { target: _tx; y: 0 }
    }
    transitions: Transition {
        NumberAnimation { property: "y"; duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
    }
}
