// =============================================================================
//  NetworkImage.qml
// =============================================================================
//  Async image with three states: loading (skeleton), loaded (image),
//  error (fallback icon). Caches decoded results via Qt's image cache.
//
//  Public API:
//      source      : url / string — image source (http(s):// or qrc:/)
//      fillMode    : enum — Image.Stretch | PreserveAspectFit | PreserveAspectCrop | ...
//      placeholder : string — Material Symbols icon shown on error
//      fallbackColor : color — background tint when no image / error
//
//  Signals:
//      loaded()   — image finished loading
//      failed()   — error / no source
// =============================================================================
import QtQuick 2.15
import "../theme"
import "../"
import "./progress"

Item {
    id: root

    property string source: ""
    property int fillMode: Image.PreserveAspectCrop
    property string placeholder: "auto_stories"
    property color fallbackColor: Theme.color.fieldFilled

    signal loaded()
    signal failed()

    // ----- Background (always visible, tints the area while loading) -----
    Rectangle {
        anchors.fill: parent
        color: root.fallbackColor
        radius: parent && parent.radius ? parent.radius : 0
    }

    // ----- Skeleton (visible while loading) -----
    SkeletonLoader {
        anchors.fill: parent
        radius: parent && parent.radius ? parent.radius : 0
        active: _img.status === Image.Loading
    }

    // ----- Image -----
    Image {
        id: _img
        anchors.fill: parent
        source: root.source.length > 0 ? root.source : ""
        sourceSize: Qt.size(root.width * 2, root.height * 2)
        fillMode: root.fillMode
        asynchronous: true
        cache: true
        visible: status === Image.Ready
        opacity: status === Image.Ready ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic } }

        onStatusChanged: {
            if (status === Image.Ready) root.loaded()
            else if (status === Image.Error) root.failed()
        }
    }

    // ----- Error / empty fallback -----
    Item {
        anchors.fill: parent
        visible: _img.status === Image.Error || root.source.length === 0
        opacity: visible ? 1.0 : 0.0

        AppIcon {
            anchors.centerIn: parent
            name: root.placeholder
            size: Math.min(root.width, root.height) * 0.35
            color: Theme.color.textMuted
        }
    }
}
