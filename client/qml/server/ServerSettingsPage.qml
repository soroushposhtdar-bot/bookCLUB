// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// LAYOUT FIXES (this pass):
//   - All three Cards (Appearance, Monitoring, Account): inner Column
//     anchors.fill: parent → width: parent.width (fixes Card collapse —
//     childrenRect.height circular dependency was hiding all content past
//     the SectionHeader).
//   - Account Row → RowLayout so Layout.fillWidth on the "Signed in as"
//     Text actually pushes the Sign out button to the right.
//   - Added top spacer as first child of Column so Cards don't start
//     flush at y=0 under the TopBar.

// =============================================================================
//  ServerSettingsPage.qml
// =============================================================================
//  Settings page for the server-operator role. Provides theme toggle,
//  auto-refresh info, and sign-out.
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

            // Top breathing room (was flush at y=0 under the TopBar)
            Item { width: 1; height: Theme.space.xl }

            // Appearance
            Card {
                width: parent.width
                padding: Theme.space.xl

                // FIX — width-only (NOT anchors.fill)
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

            // Monitoring
            Card {
                width: parent.width
                padding: Theme.space.xl

                // FIX — width-only (NOT anchors.fill)
                Column {
                    width: parent.width
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Monitoring"; subtitle: "Real-time refresh settings" }

                    Text {
                        text: "The server console auto-refreshes every 5 seconds to show live CPU, RAM, client connections, and log activity."
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

                // FIX — width-only (NOT anchors.fill)
                Column {
                    width: parent.width
                    spacing: Theme.space.lg

                    SectionHeader { width: parent.width; title: "Account"; subtitle: "Session management" }

                    // FIX — RowLayout so Layout.fillWidth works
                    RowLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        Text {
                            text: "Signed in as @" + (AuthService.currentUsername || "server")
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightMedium
                            Layout.fillWidth: true
                        }
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
