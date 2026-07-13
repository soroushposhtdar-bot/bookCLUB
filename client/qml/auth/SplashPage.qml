// =============================================================================
//  SplashPage.qml
// =============================================================================
//  Application launch screen. Shows the brand mark with a subtle pulse +
//  progress arc while AppViewModel decides where to route next (already
//  authenticated → dashboard; otherwise → WelcomePage).
//
//  Auto-dismisses after the splash minimum duration; routing is driven by
//  the parent StackView / AppViewModel.
// =============================================================================
import QtQuick 2.15
import "../theme"
import "../components/branding"
import "../components/progress"

Item {
    id: root

    property int splashDurationMs: 1600
    signal finished()

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.space.xxl

        // Brand mark with gentle pulse
        BrandLogo {
            id: _logo
            size: 84
            anchors.horizontalCenter: parent.horizontalCenter

            SequentialAnimation {
                loops: Animation.Infinite
                NumberAnimation { target: _logo; property: "scale"; to: 1.05; duration: 1200; easing.type: Easing.InOutSine }
                NumberAnimation { target: _logo; property: "scale"; to: 1.0;  duration: 1200; easing.type: Easing.InOutSine }
            }
        }

        Text {
            text: "BookClub"
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeHeadline
            font.weight: Theme.font.weightBold
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Spinner {
            size: 24
            thickness: 2
            color: Theme.color.textMuted
            progress: -1
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // Footer brand line
    Text {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: Theme.space.xl
        text: "© 2026 BookClub"
        color: Theme.color.textMuted
        font.family: Theme.font.family
        font.pixelSize: Theme.font.sizeCaption
    }

    Timer {
        interval: root.splashDurationMs
        running: true
        repeat: false
        onTriggered: root.finished()
    }
}
