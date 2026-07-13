// =============================================================================
//  AdminSettingsPage.qml
// =============================================================================
//  Settings page for the admin role. Provides theme toggle, refresh-interval
//  control, and sign-out.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/navigation"
import "../components/feedback"
import BookClub.Services 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal logoutRequested()

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Appearance -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Appearance"; subtitle: "Theme and display" }

                    SettingToggleRow {
                        width: parent.width
                        iconName: "dark_mode"
                        title: "Dark mode"
                        description: "Switch between light and dark themes."
                        checked: Theme.isDark
                        onToggled: {
                            Theme.mode = checked ? "dark" : "light"
                            page.toastRequested("info", "Theme", "Switched to " + (checked ? "dark" : "light") + " mode.")
                        }
                    }
                }
            }

            // ----- Monitoring -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Monitoring"; subtitle: "Real-time refresh settings" }

                    Text {
                        text: "The admin dashboard auto-refreshes every 5 seconds to show live KPIs, system health, and audit-log updates. This interval is fixed for the mock build."
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    SettingToggleRow {
                        width: parent.width
                        iconName: "sync"
                        title: "Auto-refresh"
                        description: "Pulse the dashboard every 5 seconds for live data."
                        checked: true
                        onToggled: page.toastRequested("info", "Auto-refresh", "Toggle is " + (checked ? "on" : "off") + ". (Mock — always on.)")
                    }
                }
            }

            // ----- Account -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Account"; subtitle: "Session management" }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Text {
                            text: "Signed in as @" + (AuthService.currentUsername || "admin")
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightMedium
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                        PrimaryButton {
                            text: "Sign out"
                            iconName: "logout"
                            onClicked: page.logoutRequested()
                        }
                    }
                }
            }

            // ----- About -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.sm

                    SectionHeader { width: parent.width; title: "About"; subtitle: "System information" }

                    Text {
                        text: "BookClub Admin Console"
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
                        text: "Uptime: " + (page.viewModel ? page.viewModel.systemUptime : "—")
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }
            }

            // ----- Bottom footer spacer (prevents the last card from
            //       sitting flush against the scroll viewport edge) -----
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
