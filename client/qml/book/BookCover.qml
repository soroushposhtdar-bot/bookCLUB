// =============================================================================
//  BookCover.qml
// =============================================================================
//  Synthetic book cover — the mock catalog has no real cover images, so we
//  render a clean monochrome cover from a primary hue + accent. Looks
//  consistent with the minimal brand aesthetic.
//
//  Public API:
//      book         : BookDto* (or any object with title/authorName/coverColor/coverAccent)
//      width        : int (square driven by parent; height = width * 1.5)
//      cornerRadius : int
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property var book: null
    property int cornerRadius: Theme.radius.sm

    implicitWidth: 120
    implicitHeight: width * Theme.size.bookCoverRatio

    Rectangle {
        id: _cover
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.book ? root.book.coverColor : Theme.color.primary
        clip: true

        // Subtle accent stripe down the left edge (book spine effect)
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 6
            color: root.book ? root.book.coverAccent : Theme.color.accent
            opacity: 0.85
        }

        // Faint diagonal accent in the top-right
        Canvas {
            anchors.fill: parent
            opacity: 0.10
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                var w = width, h = height
                ctx.fillStyle = "#FFFFFF"
                ctx.beginPath()
                ctx.moveTo(w * 0.55, 0)
                ctx.lineTo(w, 0)
                ctx.lineTo(w, h * 0.42)
                ctx.closePath()
                ctx.fill()
            }
        }

        // Title + author
        Column {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            anchors.topMargin: 22
            anchors.bottomMargin: 16
            spacing: 6

            Text {
                text: root.book ? root.book.title : ""
                color: "#FFFFFF"
                font.family: Theme.font.family
                font.pixelSize: Math.max(11, root.width * 0.10)
                font.weight: Theme.font.weightBold
                wrapMode: Text.WordWrap
                width: parent.width
                maximumLineCount: 3
                elide: Text.ElideRight
                lineHeight: 1.1
            }

            Item { width: 1; height: 4 }

            Text {
                text: root.book ? root.book.authorName : ""
                color: "rgba(255, 255, 255, 0.78)"
                font.family: Theme.font.family
                font.pixelSize: Math.max(9, root.width * 0.075)
                font.weight: Theme.font.weightRegular
                wrapMode: Text.WordWrap
                width: parent.width
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Item { width: 1; height: 1; Layout.fillHeight: true }

            // Bottom badge — publisher initial
            Rectangle {
                width: _pubText.implicitWidth + 16
                height: 22
                radius: Theme.radius.pill
                color: "rgba(255, 255, 255, 0.14)"
                anchors.left: parent.left
                anchors.bottom: parent.bottom

                Text {
                    id: _pubText
                    anchors.centerIn: parent
                    text: root.book && root.book.publisherName.length > 0
                          ? root.book.publisherName.charAt(0).toUpperCase()
                          : ""
                    color: "#FFFFFF"
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeMicro2
                    font.weight: Theme.font.weightBold
                }
            }
        }
    }
}
