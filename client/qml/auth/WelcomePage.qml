// =============================================================================
//  WelcomePage.qml
// =============================================================================
//  Pre-auth landing page. Single centered card (no split hero) introducing
//  the brand and offering the two top-level actions: Login / Register.
//
//  Used as the entry point after the splash — when no auth decision has been
//  made yet.
// =============================================================================
import QtQuick 2.15
import "../theme"
import "../components/surfaces"
import "../components/branding"
import "../components/buttons"
import "../components/effects"

Item {
    id: root

    signal loginRequested()
    signal registerRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    Card {
        id: _card
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.space.xl, 460)
        height: Math.min(parent.height - 2 * Theme.space.xl, _content.implicitHeight + 2 * Theme.space.xxxl)
        elevation: "lg"
        radius: Theme.radius.xl
        padding: Theme.space.xxxl

        Column {
            id: _content
            anchors.fill: parent
            spacing: Theme.space.xl

            BrandLogo {
                size: 72
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Welcome to BookClub"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightBold
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Your digital reading companion.\nDiscover, collect, and read — all in one place."
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightRegular
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.WordWrap
                width: parent.width
                lineHeight: 1.5
            }

            Item { width: 1; height: Theme.space.xs }

            Column {
                width: parent.width
                spacing: Theme.space.md

                PrimaryButton {
                    text: "Login"
                    iconName: "login"
                    iconPosition: "leading"
                    fullWidth: true
                    onClicked: root.loginRequested()
                }

                SecondaryButton {
                    text: "Create account"
                    iconName: "how_to_reg"
                    iconPosition: "leading"
                    fullWidth: true
                    onClicked: root.registerRequested()
                }
            }

            Text {
                text: "By continuing you agree to our Terms of Service and Privacy Policy."
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightRegular
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                width: parent.width
                lineHeight: 1.5
            }
        }
    }
}
