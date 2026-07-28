// =============================================================================
//  GenreSelectionPage.qml
// =============================================================================
//  Post-registration / first-login step — user picks 1-3 favourite genres so
//  the recommendation engine has a starting signal.
//
//  Multi-select grid of genre chips; canSubmit becomes true once the user has
//  selected at least `minSelection` (default 1). The grid enforces a hard
//  cap of `maxSelection` (default 3) — chips beyond the cap are disabled.
//  On submit, GenreSelectionViewModel stores the choices and emits completed().
//
//  This page replaces the typical split-screen layout — it's wider and uses a
//  full-bleed centered card with a grid of selectable chips.
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/surfaces"
import "../components/branding"
import "../components/buttons"
import "../components/progress"
import "../components/feedback"
import "../components/effects"
import "../components"

Item {
    id: root

    property var viewModel: null   // GenreSelectionViewModel
    property bool isBusy: viewModel ? viewModel.isSubmitting : false
    property int minSelection: viewModel ? viewModel.minSelection : 1
    property int maxSelection: 3   // spec: "1-3 genres"

    signal completed()
    signal backRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    Card {
        id: _card
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.space.xl, 760)
        height: Math.min(parent.height - 2 * Theme.space.xl, _content.implicitHeight + 2 * Theme.space.xxxl)
        elevation: "lg"
        radius: Theme.radius.xl
        padding: Theme.space.xxxl

        Column {
            id: _content
            anchors.fill: parent
            spacing: Theme.space.xl

            // ----- Back + brand row -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.md

                IconButton {
                    iconName: "arrow_back"
                    iconColor: Theme.color.textSecondary
                    onClicked: root.backRequested()
                    visible: root.viewModel && root.viewModel.canGoBack
                }

                Item { Layout.fillWidth: true }

                BrandLogo { size: 36 }
            }

            // ----- Title block -----
            Column {
                width: parent.width
                spacing: Theme.space.xs

                Text {
                    text: "Pick your favourite genres"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeDisplay
                    font.weight: Theme.font.weightSemibold
                    horizontalAlignment: Text.AlignHCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "We'll use these to recommend books you'll love. Pick at least " + root.minSelection + "."
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // ----- Selection counter -----
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.space.sm

                Text {
                    text: (root.viewModel ? root.viewModel.selectedCount : 0) + " / " + root.maxSelection
                    color: (root.viewModel ? root.viewModel.selectedCount : 0) >= root.minSelection
                           ? Theme.color.success
                           : Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBody
                    font.weight: Theme.font.weightSemibold
                    anchors.verticalCenter: parent.verticalCenter
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                }

                AppIcon {
                    name: "check_circle"
                    size: Theme.size.iconSm
                    color: (root.viewModel ? root.viewModel.selectedCount : 0) >= root.minSelection
                           ? Theme.color.success
                           : Theme.color.textMuted
                    visible: (root.viewModel ? root.viewModel.selectedCount : 0) >= root.minSelection
                    anchors.verticalCenter: parent.verticalCenter
                    Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                }
            }

            // ----- Genre chips grid -----
            GridView {
                id: _grid
                width: parent.width
                // BUG FIX: `count * 50` used the total model count (not row
                // count) and a wrong row height (50 vs cellHeight 48). Now
                // computes the actual row count × cellHeight.
                height: Math.min(340, Math.ceil(count / 3) * cellHeight + Theme.space.xxs * 2)
                cellWidth: width / 3 - Theme.space.sm
                cellHeight: 48
                clip: true
                interactive: false
                model: root.viewModel ? root.viewModel.availableGenres : []

                delegate: Item {
                    width: _grid.cellWidth
                    height: _grid.cellHeight

                    // BUG FIX (genre chip binding): the previous binding
                    // `root.viewModel.isSelected(modelData)` called a
                    // Q_INVOKABLE method inside a property binding. QML's
                    // binding engine cannot track dependencies inside a
                    // C++ method call, so the binding was evaluated only
                    // once at delegate creation time — clicking a chip
                    // never updated its highlight color or check icon.
                    // Bind to the `selectedGenres` list property (which
                    // has a NOTIFY signal) so the binding re-evaluates on
                    // every toggle.
                    property bool isSelected: root.viewModel
                                              && root.viewModel.selectedGenres.indexOf(modelData) >= 0

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: Theme.space.xxs
                        radius: Theme.radius.pill
                        color: isSelected ? Theme.color.primary : Theme.color.cardBackground
                        border.color: isSelected ? Theme.color.primary : (mouseArea.containsMouse ? Theme.color.accent : Theme.color.border)
                        // BUG FIX (Issue 22): dead ternary — both branches were `1`.
                        // Selected chips now get a thicker border for emphasis.
                        border.width: isSelected ? 2 : 1

                        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast } }

                        Row {
                            anchors.centerIn: parent
                            spacing: Theme.space.xs

                            AppIcon {
                                name: "check"
                                size: Theme.size.iconSm
                                color: Theme.color.onPrimary
                                visible: isSelected
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: modelData
                                color: isSelected ? Theme.color.onPrimary : Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                font.weight: isSelected ? Theme.font.weightSemibold : Theme.font.weightMedium
                                anchors.verticalCenter: parent.verticalCenter
                                Behavior on color { ColorAnimation { duration: Theme.motion.durationFast } }
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            // Enforce max-3 cap: if already at max and this genre
                            // is not yet selected, block the click.
                            enabled: isSelected || (root.viewModel ? root.viewModel.selectedCount : 0) < root.maxSelection
                            onClicked: {
                                if (root.viewModel) root.viewModel.toggleGenre(modelData)
                            }
                        }

                        // Subtle scale on hover
                        transform: Scale {
                            origin.x: width / 2
                            origin.y: height / 2
                            xScale: mouseArea.containsMouse && !isSelected ? 1.02 : 1.0
                            yScale: mouseArea.containsMouse && !isSelected ? 1.02 : 1.0
                            Behavior on xScale { NumberAnimation { duration: Theme.motion.durationInstant } }
                            Behavior on yScale { NumberAnimation { duration: Theme.motion.durationInstant } }
                        }
                    }
                }
            }

            // ----- Spacer -----
            Item { width: 1; height: Theme.space.xs }

            // ----- Action row -----
            Row {
                width: parent.width
                spacing: Theme.space.md

                PrimaryButton {
                    text: "Continue"
                    iconName: "arrow_forward"
                    iconPosition: "trailing"
                    width: parent.width
                    enabled: !root.isBusy && (root.viewModel ? root.viewModel.canSubmit : false)
                    loading: root.isBusy
                    onClicked: if (root.viewModel) root.viewModel.submit()
                }
            }
        }
    }

    Connections {
        target: root.viewModel
        ignoreUnknownSignals: true
        onCompleted: root.completed()
    }

    // BUG FIX: add a loading overlay so the user gets visual feedback while
    // the genre selection is being saved to the server. Previously the only
    // feedback was the button's BusyIndicator, which freezes because the
    // GUI thread is blocked by the synchronous sendRequest call.
    LoadingOverlay {
        anchors.fill: parent
        active: root.isBusy
        label: "Saving your preferences…"
    }
}
