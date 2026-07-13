// =============================================================================
//  FloatingActionButton.qml
// =============================================================================
//  Material-style FAB — circular elevated button used for the primary
//  context action (e.g. "Add to shelf", "New review").
//
//  Public API:
//      iconName : string (Material Symbols)
//      label    : string — text shown when `extended` is true
//      extended : bool   — pill shape with label, else circular icon-only
//      elevation: int    — 1..5 (Theme.elevation scale)
//
//  Signals:
//      clicked()
// =============================================================================
import QtQuick 2.15
import "../../theme"
import "../"
import "../effects"

Item {
    id: root

    property string iconName: "add"
    property string label: ""
    property bool extended: false
    property int elevation: 3

    implicitWidth: root.extended ? _row.implicitWidth + 2 * Theme.space.lg : 56
    implicitHeight: 56

    signal clicked()

    Rectangle {
        id: _bg
        anchors.fill: parent
        radius: root.extended ? Theme.radius.pill : width / 2
        color: _ma.pressed ? Theme.color.primaryPressed
             : _ma.containsMouse ? Theme.color.primaryHover
             : Theme.color.primary
        scale: _ma.pressed ? 0.95 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.motion.durationInstant; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }

        layer.enabled: true
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.lg }
    }

    Row {
        id: _row
        anchors.centerIn: parent
        spacing: Theme.space.sm

        AppIcon {
            name: root.iconName
            size: 24
            color: Theme.color.onPrimary
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.label
            color: Theme.color.onPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBodyLarge
            font.weight: Theme.font.weightSemibold
            visible: root.extended
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: _ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
