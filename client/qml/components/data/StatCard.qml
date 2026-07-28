// =============================================================================
//  StatCard.qml  (v3 polish — adds optional sparkline + delta indicator)
// =============================================================================
//  Compact stat display: icon, value, label, optional delta, optional
//  sparkline.
//
//  Public API:
//      iconName : string
//      value    : string  — formatted value ("2,310", "$1,420")
//      label    : string  — what the value means
//      delta    : string  — optional ("+12% vs last week")
//      deltaUp  : bool    — green if up, red if down
//      accent   : color   — icon-circle background tint
//      spark    : var     — optional list of numbers; renders a small
//                            inline sparkline below the value
//      loading  : bool    — when true, shows a shimmer over the value
//
//  v3 polish improvements:
//    • Sparkline support — drop a `spark: [...]` array and a tiny line chart
//      renders between the icon and the label, giving the dashboard KPI cards
//      a real "stat dashboard" feel without taking extra vertical space.
//    • Loading state — when `loading` is true, the value/label area is
//      covered by a shimmer so the user sees motion instead of stale data.
//    • Delta arrow now uses an AppIcon glyph instead of a unicode triangle
//      for crisper rendering at small sizes.
//    • Hover state — subtle accent-soft tint over the card border so the
//      card reacts when the user mouses over it.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import ".."
import "../surfaces"
import "../progress"

Card {
    id: root
    elevation: "none"
    bordered: true
    padding: Theme.space.lg

    property string iconName: "trending_up"
    property string value: "0"
    property string label: ""
    property string delta: ""
    property bool deltaUp: true
    property color accent: Theme.color.accent
    property var spark: []
    property bool loading: false

    // Subtle hover state — border tints to accent on hover.
    QtObject {
        id: _priv
        property bool hovered: false
    }
    HoverHandler {
        id: _hover
        cursorShape: Qt.ArrowCursor
        onHoveredChanged: _priv.hovered = hovered
    }
    border.color: _priv.hovered ? Theme.color.borderStrong : Theme.color.border
    Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }

    RowLayout {
        anchors.fill: parent
        spacing: Theme.space.md

        // Icon circle
        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Layout.alignment: Qt.AlignVCenter
            radius: 12
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.14)

            AppIcon {
                anchors.centerIn: parent
                name: root.iconName
                size: 22
                color: root.accent
            }
        }

        // Value + label + delta column
        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                text: root.value
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightBold
            }
            Text {
                text: root.label
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                elide: Text.ElideRight
                width: parent.width
            }
            RowLayout {
                spacing: 4
                visible: root.delta.length > 0
                AppIcon {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    name: root.deltaUp ? "trending_up" : "trending_down"
                    size: 12
                    color: root.deltaUp ? Theme.color.success : Theme.color.error
                }
                Text {
                    text: root.delta
                    color: root.deltaUp ? Theme.color.success : Theme.color.error
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightMedium
                }
            }
        }

        // Optional sparkline
        Item {
            id: _spark
            visible: root.spark.length > 0
            Layout.preferredWidth: visible ? 64 : 0
            Layout.preferredHeight: 32
            Layout.alignment: Qt.AlignVCenter

            Canvas {
                anchors.fill: parent
                readonly property var _data: root.spark
                readonly property real _max: _data.length > 0 ? Math.max.apply(null, _data) : 0
                readonly property real _min: _data.length > 0 ? Math.min.apply(null, _data) : 0
                on_DataChanged: requestPaint()
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    if (_data.length < 2 || _max <= _min) return
                    const w = width, h = height, pad = 2
                    const stepX = (w - 2 * pad) / (_data.length - 1)
                    const range = _max - _min

                    // Stroke
                    ctx.beginPath()
                    for (let i = 0; i < _data.length; ++i) {
                        const x = pad + i * stepX
                        const y = pad + (1 - (_data[i] - _min) / range) * (h - 2 * pad)
                        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                    }
                    ctx.lineWidth = 1.6
                    ctx.strokeStyle = root.accent
                    ctx.stroke()

                    // Subtle fill below the line
                    ctx.lineTo(w - pad, h - pad)
                    ctx.lineTo(pad, h - pad)
                    ctx.closePath()
                    ctx.fillStyle = Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.12)
                    ctx.fill()
                }
            }
        }
    }

    // Loading shimmer
    SkeletonLoader {
        anchors.fill: parent
        active: root.loading
        radius: Theme.radius.xl
        z: 10
    }
}
