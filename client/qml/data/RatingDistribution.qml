// =============================================================================
//  RatingDistribution.qml
// =============================================================================
//  Horizontal bar chart showing the breakdown of star ratings for a book.
//  Each row: "5★" label, bar (filled proportionally), count.
//
//  Public API:
//      distribution : var  — list of { stars: int, count: int }, 5 down to 1
//      totalRatings : int  — sum of all counts (used for bar proportions)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../book"

Column {
    id: root
    spacing: 6

    property var distribution: []
    property int totalRatings: 0

    Repeater {
        model: root.distribution
        delegate: Row {
            width: root.width
            spacing: Theme.space.md

            Text {
                text: modelData.stars + "★"
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightMedium
                width: 28
                anchors.verticalCenter: parent.verticalCenter
            }

            // Bar
            Rectangle {
                width: root.width - 28 - Theme.space.md - _countText.implicitWidth - Theme.space.md
                height: 8
                radius: 4
                color: Theme.color.fieldFilled
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    width: parent.width * (root.totalRatings > 0 ? modelData.count / root.totalRatings : 0)
                    height: parent.height
                    radius: parent.radius
                    color: Theme.color.warning
                    Behavior on width { NumberAnimation { duration: Theme.motion.durationSlow; easing.type: Easing.OutQuint } }
                }
            }

            Text {
                id: _countText
                text: modelData.count >= 1000 ? (modelData.count / 1000).toFixed(1) + "k" : String(modelData.count)
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                width: 40
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
