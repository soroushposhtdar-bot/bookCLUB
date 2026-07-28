// =============================================================================
//  CalendarGrid.qml  (new in v3 polish)
// =============================================================================
//  Lightweight month-grid calendar used by the publisher Promotions page
//  for picking start/end dates. Hand-rolled so it works on both Qt 5 and
//  Qt 6 without depending on QtQuick.Controls Calendar (which is
//  deprecated in Qt 6) or Tumbler.
//
//  Public API:
//      initialDate : var (Date) — the date to center the view on / highlight
//      selectedDate : var (Date, read-only) — the most recently picked date
//
//  Signals:
//      dateSelected(var date) — fired when the user clicks a day cell
//
//  Layout:
//      ┌──────────────────────────────────────┐
//      │  ◄  March 2026                       ► │
//      ├──────────────────────────────────────┤
//      │  S   M   T   W   T   F   S           │
//      │  ·   ·   ·   ·   ·   ·   1           │
//      │  2   3   4   5   6   7   8           │
//      │  ...                                 │
//      └──────────────────────────────────────┘
//
//  Today's date gets a thin accent border; the initial date gets a solid
//  accent fill so the user can see both at a glance.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import ".."
import "../buttons"

Item {
    id: root

    property var initialDate: new Date()
    property var selectedDate: null

    signal dateSelected(var date)

    // Internal month being viewed (1st of the month).
    property var _viewDate: new Date(initialDate.getFullYear(),
                                     initialDate.getMonth(), 1)

    Component.onCompleted: {
        _viewDate = new Date(initialDate.getFullYear(),
                             initialDate.getMonth(), 1)
    }

    onInitialDateChanged: {
        _viewDate = new Date(initialDate.getFullYear(),
                             initialDate.getMonth(), 1)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.space.sm

        // ----- Header row: month + nav arrows -----
        RowLayout {
            Layout.fillWidth: true
            IconButton {
                iconName: "chevron_left"
                onClicked: {
                    root._viewDate = new Date(root._viewDate.getFullYear(),
                                              root._viewDate.getMonth() - 1, 1)
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                text: root._viewDate.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeBodyLarge
                font.weight: Theme.font.weightBold
                horizontalAlignment: Text.AlignHCenter
            }
            IconButton {
                iconName: "chevron_right"
                onClicked: {
                    root._viewDate = new Date(root._viewDate.getFullYear(),
                                              root._viewDate.getMonth() + 1, 1)
                }
            }
        }

        // ----- Day-of-week header -----
        RowLayout {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: ["S", "M", "T", "W", "T", "F", "S"]
                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    text: modelData
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightBold
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // ----- Day grid -----
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 7
            rowSpacing: 2
            columnSpacing: 2

            Repeater {
                model: {
                    const year = root._viewDate.getFullYear()
                    const month = root._viewDate.getMonth()
                    const firstDay = new Date(year, month, 1)
                    const startOffset = firstDay.getDay()
                    const daysInMonth = new Date(year, month + 1, 0).getDate()
                    const cells = []
                    for (let i = 0; i < startOffset; ++i) cells.push(null)
                    for (let d = 1; d <= daysInMonth; ++d) cells.push(new Date(year, month, d))
                    while (cells.length % 7 !== 0) cells.push(null)
                    return cells
                }
                delegate: Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    visible: modelData !== null && modelData !== undefined

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: _dayHover.hovered ? Theme.color.accentSoft
                             : (modelData && root.initialDate &&
                                modelData.toDateString() === root.initialDate.toDateString())
                               ? Theme.color.accent
                               : "transparent"
                        border.color: (modelData && new Date().toDateString() === modelData.toDateString())
                                      ? Theme.color.accent
                                      : "transparent"
                        border.width: 1

                        HoverHandler { id: _dayHover; cursorShape: Qt.PointingHandCursor }

                        Text {
                            anchors.centerIn: parent
                            text: modelData ? modelData.getDate() : ""
                            color: (modelData && root.initialDate &&
                                    modelData.toDateString() === root.initialDate.toDateString())
                                   ? Theme.color.textOnAccent
                                   : Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightMedium
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (modelData) {
                                    root.selectedDate = modelData
                                    root.dateSelected(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
