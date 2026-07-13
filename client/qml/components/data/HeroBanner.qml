// =============================================================================
//  HeroBanner.qml
// =============================================================================
//  Large greeting hero used at the top of the Home page. Black card with a
//  decorative row of book spines on the right + greeting + subtitle + CTA.
//
//  Public API:
//      greeting       : string  — the personalized greeting line
//      subtext        : string  — supporting copy
//      primaryActionLabel : string (default "Continue reading")
//
//  Signals:
//      primaryAction()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"

Rectangle {
    id: root

    property string greeting: ""
    property string subtext: ""
    property string primaryActionLabel: "Continue reading"

    signal primaryAction()

    implicitWidth: parent ? parent.width : 800
    height: 180
    radius: Theme.radius.xxl
    color: Theme.color.primary
    clip: true

    // Decorative book spines on the right
    Canvas {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 240
        opacity: 0.22
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var colors = ["#FFFFFF", "#1A73E8", "#5F6368", "#FFFFFF", "#1A73E8", "#5F6368", "#FFFFFF"]
            var w = 22, gap = 6, baseH = 200
            var x = width - colors.length * (w + gap) - 20
            for (var i = 0; i < colors.length; i++) {
                ctx.fillStyle = colors[i]
                var h = baseH - i * 4
                ctx.fillRect(x + i * (w + gap), height - h - 8, w, h)
            }
        }
    }

    // Subtle diagonal accent
    Canvas {
        anchors.fill: parent
        opacity: 0.06
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.fillStyle = "#FFFFFF"
            ctx.beginPath()
            ctx.moveTo(width * 0.50, 0)
            ctx.lineTo(width * 0.65, 0)
            ctx.lineTo(width * 0.40, height)
            ctx.lineTo(width * 0.25, height)
            ctx.closePath()
            ctx.fill()
        }
    }

    Column {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.space.xxxl
        spacing: Theme.space.md

        Text {
            text: root.greeting
            color: Theme.color.onPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeDisplay
            font.weight: Theme.font.weightBold
        }

        Text {
            text: root.subtext
            color: "rgba(255, 255, 255, 0.78)"
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBodyLarge
            font.weight: Theme.font.weightRegular
        }

        Row {
            spacing: Theme.space.md

            PrimaryButton {
                text: root.primaryActionLabel
                iconName: "menu_book"
                iconPosition: "leading"
                // Inverted color treatment on the dark hero
                background: Item {
                    anchors.fill: parent
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radius.md
                        color: parent.parent.hovered ? "rgba(255,255,255,0.18)" : "rgba(255,255,255,0.14)"
                        border.color: "rgba(255,255,255,0.30)"
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                    }
                }
                onClicked: root.primaryAction()
            }

            Text {
                text: "2 books in progress"
                color: "rgba(255, 255, 255, 0.6)"
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
