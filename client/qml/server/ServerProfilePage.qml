// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Defensive value clamping for CPU/RAM (handles -1, undefined, NaN)
//   - All KPI values converted to strings for safe display
//
// LAYOUT FIXES (this pass):
//   - BUG A: Added top spacer + replaced header Card Row { anchors.fill }
//     with RowLayout so the avatar + name aren't clipped at the top.
//   - BUG B: KPI Row → RowLayout with Layout.fillWidth + Layout.preferredWidth: 1
//     on each StatCard; set height: Theme.size.kpiCardHeight.
//   - BUG C: About Card inner Column: anchors.fill → width-only so the Card
//     doesn't collapse (childrenRect.height circular dependency).

// =============================================================================
//  ServerProfilePage.qml
// =============================================================================
//  Profile page for the server-operator role. Shows the operator's account
//  info, role, and a summary of the current server health.
// =============================================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"
import BookClub.Services 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    // Defensive value clamping (handles -1, undefined, NaN from the VM)
    function _clampPercent(v) {
        if (v === undefined || v === null) return 0
        var n = Number(v)
        if (isNaN(n)) return 0
        if (n < 0) return 0
        if (n > 100) return 100
        return Math.round(n)
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // BUG A — Top breathing room (was flush at y=0 under the TopBar)
            Item { width: 1; height: Theme.space.xl }

            // Header card (BUG A — RowLayout instead of Row { anchors.fill })
            Card {
                width: parent.width
                padding: Theme.space.xl

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.xl

                    Rectangle {
                        width: 96; height: 96; radius: 24
                        color: Theme.color.success
                        Layout.alignment: Qt.AlignVCenter
                        Text {
                            anchors.centerIn: parent
                            text: {
                                const name = AuthService.currentDisplayName || "S"
                                return name.charAt(0).toUpperCase()
                            }
                            color: Theme.color.textOnAccent
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeMega
                            font.weight: Theme.font.weightBold
                        }
                    }

                    Column {
                        spacing: Theme.space.sm
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true

                        Text {
                            text: AuthService.currentDisplayName || "Server Operator"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "@" + (AuthService.currentUsername || "server")
                            color: Theme.color.textMuted
                            font.family: Theme.font.familyMono
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Rectangle {
                            width: _roleLbl.implicitWidth + 16; height: 24; radius: 12
                            color: Theme.color.successSoft
                            Text {
                                id: _roleLbl
                                anchors.centerIn: parent
                                text: "Server Operator"
                                color: Theme.color.success
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightBold
                            }
                        }
                    }
                }
            }

            // KPI cards (BUG B — RowLayout)
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "dns"
                    value: (page.viewModel ? page.viewModel.connectedClientCount : 0).toString()
                    label: "Connected clients"
                    delta: "Live"
                    deltaUp: true
                    accent: Theme.color.accent
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "memory"
                    value: page._clampPercent(page.viewModel ? page.viewModel.cpuLoad : 0) + "%"
                    label: "CPU load"
                    delta: "Real-time"
                    deltaUp: true
                    accent: Theme.color.info
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "storage"
                    value: page._clampPercent(page.viewModel ? page.viewModel.ramUsage : 0) + "%"
                    label: "RAM usage"
                    delta: "Real-time"
                    deltaUp: true
                    accent: Theme.color.warning
                }
            }

            // About card (BUG C — width-only, NOT anchors.fill)
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    width: parent.width
                    spacing: Theme.space.sm

                    SectionHeader { width: parent.width; title: "About"; subtitle: "System information" }

                    Text {
                        text: "BookClub Server Console"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        font.weight: Theme.font.weightMedium
                    }
                    Text {
                        text: "Version 1.0.0 (build 2025.07)"
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                    Text {
                        text: "Auto-refresh: every 5 seconds"
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
