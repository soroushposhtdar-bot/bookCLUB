// =============================================================================
//  RatingStars.qml
// =============================================================================
//  Read-only star rating display (0–5, supports half stars).
//
//  Public API:
//      rating   : real   (0.0 – 5.0)
//      size     : int    (pixel size of each star)
//      spacing  : int
//      showNumber : bool (render "4.6 (2,310)" next to the stars)
//      count    : int    (rating count, shown when showNumber is true)
//      color    : color  (star color; defaults to warning yellow)
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"

Row {
    id: root
    spacing: 2

    property real rating: 0.0
    property int size: Theme.size.iconSm
    property int starSpacing: 2
    property bool showNumber: false
    property int count: 0
    property color color: Theme.color.warning

    spacing: starSpacing

    Repeater {
        model: 5
        Item {
            width: root.size
            height: root.size
            anchors.verticalCenter: parent.verticalCenter

            // Background (outline) star
            AppIcon {
                anchors.fill: parent
                name: "star_outline"
                size: root.size
                color: Qt.rgba(Theme.color.textMuted.r, Theme.color.textMuted.g, Theme.color.textMuted.b, 0.35)
            }

            // Foreground filled star — clipped to the fractional portion
            Item {
                anchors.fill: parent
                visible: root.rating > index
                clip: true

                AppIcon {
                    x: 0
                    y: 0
                    width: Math.min(root.size, root.size * (root.rating - index))
                    height: root.size
                    name: "star"
                    size: root.size
                    color: root.color
                }
            }
        }
    }

    Text {
        visible: root.showNumber
        text: root.count > 0
              ? "%1 (%2)".arg(root.rating.toFixed(1)).arg(_formatCount(root.count))
              : root.rating > 0 ? root.rating.toFixed(1) : "No ratings"
        color: Theme.color.textSecondary
        font.family: Theme.font.family
        font.pixelSize: Theme.font.sizeCaption
        font.weight: Theme.font.weightMedium
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: 4
    }

    function _formatCount(n) {
        if (n >= 1000) return (n / 1000).toFixed(1) + "k"
        return String(n)
    }
}
