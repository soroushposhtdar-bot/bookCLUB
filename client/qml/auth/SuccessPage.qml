// =============================================================================
//  SuccessPage.qml
// =============================================================================
//  Generic success state screen — used after registration, password reset,
//  email verification, etc. Centered card with animated check icon, headline,
//  short message, and a single primary CTA.
//
//  Public API:
//      title       : string
//      message     : string
//      ctaLabel    : string   — primary button label
//      ctaIcon     : string   — primary button icon
//      secondaryLabel : string — optional secondary action label
//      icon        : string   — override default "check" hero icon
//      variant     : string   — "success" | "info" | "warning"
// =============================================================================
import QtQuick 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/feedback"
import "../components/effects"

Item {
    id: root

    property string title: "Success!"
    property string message: "Your action has been completed successfully."
    property string ctaLabel: "Continue"
    property string ctaIcon: "arrow_forward"
    property string secondaryLabel: ""
    property string icon: "check_circle"
    property string variant: "success"

    signal primaryAction()
    signal secondaryAction()

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

            // ----- Animated hero icon -----
            Item {
                width: 96
                height: 96
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    id: _iconBg
                    anchors.fill: parent
                    radius: width / 2
                    color: root.variant === "success" ? Theme.color.successSoft
                         : root.variant === "warning" ? Theme.color.warningSoft
                         : Theme.color.infoSoft
                    scale: 0.0

                    SequentialAnimation {
                        running: true
                        NumberAnimation { target: _iconBg; property: "scale"; from: 0.0; to: 1.0; duration: 380; easing.type: Easing.OutBack }
                        PauseAnimation { duration: 0 }
                    }
                }

                AppIcon {
                    name: root.icon
                    size: 48
                    color: root.variant === "success" ? Theme.color.success
                         : root.variant === "warning" ? Theme.color.warning
                         : Theme.color.info
                    anchors.centerIn: _iconBg
                    scale: 0.0

                    SequentialAnimation {
                        running: true
                        PauseAnimation { duration: 220 }
                        NumberAnimation { target: parent; property: "scale"; from: 0.0; to: 1.0; duration: 280; easing.type: Easing.OutBack }
                    }
                }
            }

            // ----- Title -----
            Text {
                text: root.title
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightSemibold
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.WordWrap
                width: parent.width
            }

            // ----- Message -----
            Text {
                text: root.message
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightRegular
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                width: parent.width
                lineHeight: 1.5
            }

            Item { width: 1; height: Theme.space.xs }

            // ----- CTA -----
            PrimaryButton {
                text: root.ctaLabel
                iconName: root.ctaIcon
                iconPosition: "trailing"
                fullWidth: true
                onClicked: root.primaryAction()
            }

            // ----- Secondary action -----
            SecondaryButton {
                text: root.secondaryLabel
                fullWidth: true
                visible: root.secondaryLabel.length > 0
                onClicked: root.secondaryAction()
            }
        }
    }
}
