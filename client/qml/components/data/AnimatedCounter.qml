// =============================================================================
//  AnimatedCounter.qml
// =============================================================================
//  Number display that animates from old value → new value over `duration`.
//  Used for stats, ratings, totals — anywhere a number changes.
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property int value: 0
    property string prefix: ""
    property string suffix: ""
    property int duration: Theme.motion.durationSlow
    property color color: Theme.color.textPrimary
    property int pixelSize: Theme.font.sizeDisplay
    property int fontWeight: Theme.font.weightBold

    implicitWidth: _text.implicitWidth
    implicitHeight: _text.implicitHeight

    property int _displayValue: value

    Text {
        id: _text
        anchors.fill: parent
        text: root.prefix + root._displayValue + root.suffix
        color: root.color
        font.family: Theme.font.family
        font.pixelSize: root.pixelSize
        font.weight: root.fontWeight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Behavior on _displayValue {
        NumberAnimation {
            duration: root.duration
            easing.type: Easing.OutQuint
        }
    }

    onValueChanged: {
        // Trigger animation via the Behavior
    }
}
