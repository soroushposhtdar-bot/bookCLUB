// =============================================================================
//  ServerProfilePage.qml
// =============================================================================
//  Profile page for the server-operator role. Shows the operator's account
//  info, role, and a summary of the current server health.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
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

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // Header card
            Card {
                width: parent.width
                padding: Theme.space.xl

                Row {
                    anchors.fill: parent
                    spacing: Theme.space.xl

                    Rectangle {
                        width: 96; height: 96; radius: 24
                        color: Theme.color.success
                        anchors.verticalCenter: parent.verticalCenter
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
                        anchors.verticalCenter: parent.verticalCenter

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

                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                }
            }

            // KPI cards
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "dns"
                    value: (page.viewModel ? page.viewModel.connectedClientCount : 0).toString()
                    label: "Connected clients"
                    delta: "Live"
                    deltaUp: true
                    accent: Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "memory"
                    value: (page.viewModel ? page.viewModel.cpuLoad : 0) + "%"
                    label: "CPU load"
                    delta: "Real-time"
                    deltaUp: true
                    accent: Theme.color.info
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "storage"
                    value: (page.viewModel ? page.viewModel.ramUsage : 0) + "%"
                    label: "RAM usage"
                    delta: "Real-time"
                    deltaUp: true
                    accent: Theme.color.warning
                }
            }

            // About card
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
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
