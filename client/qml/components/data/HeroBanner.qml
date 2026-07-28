// =============================================================================
//  HeroBanner.qml
// =============================================================================
//  Large greeting hero used at the top of the Home page. Black card with a
//  decorative row of book spines on the right + greeting + subtitle + CTA.
//
//  Public API:
//      greeting            : string  — the personalized greeting line
//      subtext             : string  — supporting copy
//      primaryActionLabel  : string  (default "Continue reading")
//      continueReadingBook : var     — the current in-progress book (BookDto*),
//                                       or null. When present, the banner
//                                       shows a mini-progress card.
//      inProgressCount     : int     — number of in-progress books
//
//  Signals:
//      primaryAction()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import ".."
import "../buttons"
import "../book"

Rectangle {
    id: root

    property string greeting: ""
    property string subtext: ""
    property string primaryActionLabel: "Continue reading"
    property var continueReadingBook: null
    property int inProgressCount: 0

    signal primaryAction()

    implicitWidth: parent ? parent.width : 800
    height: 200
    radius: Theme.radius.xxl
    // Issue 17 — the hero banner is always deep-black regardless of theme
    // mode, so the white text + decorative book spines read consistently
    // in both light and dark mode. Previously this used Theme.color.primary
    // which flipped to near-white in dark mode, making the banner look
    // inverted and clashing with the rest of the dark UI.
    color: Theme.isDark ? "#000000" : Theme.color.primary
    clip: true

    // Decorative book spines on the right (clipped by parent)
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

    // Content layout: greeting + subtext + CTA on the left, optional
    // mini-progress card on the right (Issue 5: previously the banner was
    // empty even when the user had in-progress books).
    Row {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.space.xxxl
        anchors.rightMargin: Theme.space.xxxl
        spacing: Theme.space.xxl

        // ----- Left: greeting + CTA -----
        Column {
            width: root.continueReadingBook ? parent.width * 0.55 - Theme.space.xxl
                                             : parent.width
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.space.md

            Text {
                text: root.greeting
                color: Theme.color.onPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeDisplay
                font.weight: Theme.font.weightBold
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Text {
                text: root.subtext
                color: Qt.rgba(255/255, 255/255, 255/255, 0.78)
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightRegular
                width: parent.width
                wrapMode: Text.WordWrap
            }

            Row {
                spacing: Theme.space.md

                PrimaryButton {
                    id: _cta
                    text: root.primaryActionLabel
                    iconName: "menu_book"
                    iconPosition: "leading"
                    // Inverted color treatment on the dark hero
                    background: Item {
                        anchors.fill: parent
                        Rectangle {
                            anchors.fill: parent
                            radius: Theme.radius.md
                            color: parent.parent.hovered ? Qt.rgba(255/255, 255/255, 255/255, 0.18) : Qt.rgba(255/255, 255/255, 255/255, 0.14)
                            border.color: Qt.rgba(255/255, 255/255, 255/255, 0.30)
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                        }
                    }
                    // Issue 5 — never let the CTA crash the app. If the
                    // banner has no continue-reading book, the button is
                    // still tappable but the handler in HomePage will route
                    // to a safe fallback (toast / first bestseller / etc.).
                    onClicked: root.primaryAction()
                }

                Text {
                    text: root.inProgressCount > 0
                          ? (root.inProgressCount === 1 ? "1 book in progress"
                                                         : root.inProgressCount + " books in progress")
                          : "Find your next read"
                    color: Qt.rgba(255/255, 255/255, 255/255, 0.6)
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // ----- Right: mini progress card (only when there's a book) -----
        Item {
            width: root.continueReadingBook ? parent.width * 0.45 : 0
            height: root.continueReadingBook ? _progressCard.height : 0
            anchors.verticalCenter: parent.verticalCenter
            visible: root.continueReadingBook !== null

            Rectangle {
                id: _progressCard
                width: parent.width
                radius: Theme.radius.lg
                color: Qt.rgba(255/255, 255/255, 255/255, 0.10)
                border.color: Qt.rgba(255/255, 255/255, 255/255, 0.20)
                border.width: 1
                implicitHeight: _progressCol.implicitHeight + 2 * Theme.space.lg

                Column {
                    id: _progressCol
                    anchors.fill: parent
                    anchors.margins: Theme.space.lg
                    spacing: Theme.space.sm

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        // Tiny book cover swatch
                        Rectangle {
                            width: 36
                            height: 54
                            radius: Theme.radius.xs
                            color: root.continueReadingBook
                                   ? (root.continueReadingBook.coverColor || Theme.color.accent)
                                   : Theme.color.accent
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                anchors.centerIn: parent
                                text: root.continueReadingBook
                                       ? (root.continueReadingBook.title || "").charAt(0).toUpperCase()
                                       : ""
                                color: "#FFFFFF"
                                font.family: Theme.font.family
                                font.pixelSize: 18
                                font.weight: Theme.font.weightBold
                            }
                        }

                        Column {
                            width: parent.width - 36 - Theme.space.md
                            spacing: 2
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                text: root.continueReadingBook ? root.continueReadingBook.title : ""
                                color: Theme.color.onPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: Theme.font.weightSemibold
                                width: parent.width
                                elide: Text.ElideRight
                            }
                            Text {
                                text: root.continueReadingBook ? root.continueReadingBook.authorName : ""
                                color: Qt.rgba(255/255, 255/255, 255/255, 0.70)
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                width: parent.width
                                elide: Text.ElideRight
                            }
                        }
                    }

                    // Progress bar
                    Rectangle {
                        width: parent.width
                        height: 4
                        radius: 2
                        color: Qt.rgba(255/255, 255/255, 255/255, 0.20)

                        Rectangle {
                            width: parent.width * Math.max(0.05, Math.min(1.0,
                                root.continueReadingBook
                                  ? (root.continueReadingBook.readingProgress || 0)
                                  : 0))
                            height: parent.height
                            radius: parent.radius
                            color: Theme.color.accent
                            Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } }
                        }
                    }

                    Text {
                        text: root.continueReadingBook && root.continueReadingBook.readingPageCount > 0
                              ? (root.continueReadingBook.readingPage + " / " +
                                 root.continueReadingBook.readingPageCount + " pages")
                              : "Tap to start reading"
                        color: Qt.rgba(255/255, 255/255, 255/255, 0.70)
                        font.family: Theme.font.familyMono
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.primaryAction()
            }
        }
    }
}
