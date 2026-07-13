// =============================================================================
//  Pagination.qml
// =============================================================================
//  Page navigation control (Prev / 1 / 2 / 3 / ... / 10 / Next).
//
//  Public API:
//      currentPage : int
//      totalPages   : int
//      siblingCount : int — pages shown on either side of the current page
//
//  Signals:
//      pageRequested(int page)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"

Row {
    id: root
    spacing: 4

    property int currentPage: 1
    property int totalPages: 1
    property int siblingCount: 1

    signal pageRequested(int page)

    function _pageList() {
        var pages = []
        var last = root.totalPages
        var left = Math.max(1, root.currentPage - root.siblingCount)
        var right = Math.min(last, root.currentPage + root.siblingCount)

        pages.push(1)
        if (left > 2) pages.push(-1)   // -1 = ellipsis
        for (var i = left; i <= right; ++i) {
            if (i !== 1 && i !== last) pages.push(i)
        }
        if (right < last - 1) pages.push(-1)
        if (last > 1) pages.push(last)
        return pages
    }

    // ----- Prev -----
    Rectangle {
        width: 36; height: 36; radius: Theme.radius.md
        color: _prevMa.containsMouse ? Theme.color.fieldFilled : "transparent"
        border.color: Theme.color.border
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
        AppIcon {
            anchors.centerIn: parent
            name: "chevron_left"
            size: 20
            color: root.currentPage > 1 ? Theme.color.textPrimary : Theme.color.textMuted
        }
        MouseArea {
            id: _prevMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: root.currentPage > 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
            enabled: root.currentPage > 1
            onClicked: root.pageRequested(root.currentPage - 1)
        }
    }

    // ----- Numbered pages -----
    Repeater {
        model: root._pageList()
        delegate: Item {
            width: modelData === -1 ? 24 : 36
            height: 36
            anchors.verticalCenter: parent.verticalCenter

            Text {
                visible: modelData === -1
                anchors.centerIn: parent
                text: "…"
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBody
            }

            Rectangle {
                visible: modelData !== -1
                anchors.fill: parent
                radius: Theme.radius.md
                color: modelData === root.currentPage
                       ? Theme.color.primary
                       : (_numMa.containsMouse ? Theme.color.fieldFilled : "transparent")
                border.color: modelData === root.currentPage ? Theme.color.primary : Theme.color.border
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }

                Text {
                    anchors.centerIn: parent
                    text: String(modelData)
                    color: modelData === root.currentPage
                           ? Theme.color.onPrimary
                           : Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: modelData === root.currentPage ? Theme.font.weightSemibold : Theme.font.weightMedium
                }

                MouseArea {
                    id: _numMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.pageRequested(modelData)
                }
            }
        }
    }

    // ----- Next -----
    Rectangle {
        width: 36; height: 36; radius: Theme.radius.md
        color: _nextMa.containsMouse ? Theme.color.fieldFilled : "transparent"
        border.color: Theme.color.border
        border.width: 1
        anchors.verticalCenter: parent.verticalCenter
        AppIcon {
            anchors.centerIn: parent
            name: "chevron_right"
            size: 20
            color: root.currentPage < root.totalPages ? Theme.color.textPrimary : Theme.color.textMuted
        }
        MouseArea {
            id: _nextMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: root.currentPage < root.totalPages ? Qt.PointingHandCursor : Qt.ArrowCursor
            enabled: root.currentPage < root.totalPages
            onClicked: root.pageRequested(root.currentPage + 1)
        }
    }
}
