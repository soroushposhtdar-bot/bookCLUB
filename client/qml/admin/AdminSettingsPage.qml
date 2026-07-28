// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)

// =============================================================================
//  AdminSettingsPage.qml
// =============================================================================
//  Settings page for the admin role. Provides theme toggle, refresh-interval
//  control, and sign-out.
// =============================================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/navigation"
import "../components/feedback"
import BookClub.Services 1.0
import "../components"

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)
    signal logoutRequested()

    ScrollView {
        id: _scrollView
        anchors.fill: parent
        contentWidth: _scrollView.availableWidth > 0 ? _scrollView.availableWidth : width
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Top breathing room -----
            Item { width: 1; height: Theme.space.xl }

            // ----- Page header -----
            Item {
                width: parent.width
                height: 56
                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.space.md
                    Rectangle {
                        width: 44; height: 44; radius: 12
                        color: Qt.rgba(Theme.color.accent.r, Theme.color.accent.g, Theme.color.accent.b, 0.14)
                        anchors.verticalCenter: parent.verticalCenter
                        AppIcon {
                            anchors.centerIn: parent
                            name: "settings"
                            size: 22
                            color: Theme.color.accent
                        }
                    }
                    Column {
                        spacing: 2
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: "Settings"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "Admin preferences"
                            color: Theme.color.textSecondary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeCaption
                        }
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                }
            }

            // ----- Appearance -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    width: parent.width
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
                    width: parent.width
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Monitoring"; subtitle: "Real-time refresh settings" }

                    Text {
                        text: "The admin dashboard auto-refreshes every 30 seconds to show live KPIs, system health, and audit-log updates. This interval is configurable via the publisher theme token (Theme.publisher.refreshIntervalMs)."
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
                        // QA-249: corrected the interval text from
                        // "5 seconds" to "30 seconds" to match the
                        // actual AdminShell Timer interval
                        // (Theme.publisher.refreshIntervalMs = 30000).
                        description: "Pulse the dashboard every 30 seconds for live data."
                        checked: true
                        onToggled: page.toastRequested("info", "Auto-refresh", "Auto-refresh is " + (checked ? "on" : "off") + ".")
                    }
                }
            }

            // ----- Account -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Account"; subtitle: "Session management" }

                    RowLayout {
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
                    width: parent.width
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
