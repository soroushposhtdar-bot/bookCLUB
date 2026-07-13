// =============================================================================
//  ServerSettingsPage.qml
// =============================================================================
//  Settings page for the server-operator role. Provides theme toggle,
//  auto-refresh info, and sign-out.
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

            // Appearance
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

            // Monitoring
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Monitoring"; subtitle: "Real-time refresh settings" }

                    Text {
                        text: "The server console auto-refreshes every 5 seconds to show live CPU, RAM, client connections, and log activity. This interval is fixed for the mock build."
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBody
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            // Account
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
                            text: "Signed in as @" + (AuthService.currentUsername || "server")
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
        }
    }
}
