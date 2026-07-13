// =============================================================================
//  AdminProfilePage.qml
// =============================================================================
//  Profile page for the admin role. Shows the admin's account info, role,
//  access level, and recent admin actions from the audit log.
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

    readonly property var _auditLog: page.viewModel ? (page.viewModel.auditLog || []) : []

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header card -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Row {
                    anchors.fill: parent
                    spacing: Theme.space.xl

                    Rectangle {
                        width: 96; height: 96; radius: 24
                        color: Theme.color.error
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.centerIn: parent
                            text: {
                                const name = AuthService.currentDisplayName || "A"
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
                            text: AuthService.currentDisplayName || "Admin"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeTitle
                            font.weight: Theme.font.weightBold
                        }
                        Text {
                            text: "@" + (AuthService.currentUsername || "admin")
                            color: Theme.color.textMuted
                            font.family: Theme.font.familyMono
                            font.pixelSize: Theme.font.sizeBody
                        }
                        Row {
                            spacing: Theme.space.sm
                            Rectangle {
                                width: _roleLbl.implicitWidth + 16; height: 24; radius: 12
                                color: Theme.color.errorSoft
                                Text {
                                    id: _roleLbl
                                    anchors.centerIn: parent
                                    text: "Administrator"
                                    color: Theme.color.error
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }
                            Rectangle {
                                width: _accessLbl.implicitWidth + 16; height: 24; radius: 12
                                color: Theme.color.successSoft
                                Text {
                                    id: _accessLbl
                                    anchors.centerIn: parent
                                    text: "Full access"
                                    color: Theme.color.success
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }
                        }
                    }

                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                }
            }

            // ----- KPI cards -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "group"
                    value: (page.viewModel ? page.viewModel.totalUsers : 0).toString()
                    label: "Users managed"
                    delta: "Platform-wide"
                    deltaUp: true
                    accent: Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "library_books"
                    value: (page.viewModel ? page.viewModel.totalBooks : 0).toString()
                    label: "Books overseen"
                    delta: "All publishers"
                    deltaUp: true
                    accent: Theme.color.info
                }
                StatCard {
                    width: (parent.width - 2 * Theme.space.lg) / 3
                    iconName: "report"
                    value: (page.viewModel ? page.viewModel.pendingReports : 0).toString()
                    label: "Pending reports"
                    delta: "Awaiting triage"
                    deltaUp: false
                    accent: Theme.color.warning
                }
            }

            // ----- Recent admin actions -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Recent admin actions"
                        subtitle: "Your last 10 actions from the audit log"
                    }

                    ListView {
                        width: parent.width
                        height: Math.min(400, Math.max(0, page._auditLog.length) * 60)
                        clip: true
                        interactive: true
                        model: page._auditLog.slice(0, 10)
                        spacing: Theme.space.xs

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Rectangle {
                                width: 28; height: 28; radius: 8
                                color: {
                                    if (modelData.severity === "critical") return Theme.color.errorSoft
                                    if (modelData.severity === "warning") return Theme.color.warningSoft
                                    return Theme.color.infoSoft
                                }
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "gavel"
                                    size: 14
                                    color: {
                                        if (modelData.severity === "critical") return Theme.color.error
                                        if (modelData.severity === "warning") return Theme.color.warning
                                        return Theme.color.info
                                    }
                                }
                            }

                            Column {
                                width: parent.width - 28 - Theme.space.md
                                spacing: 1
                                Text {
                                    width: parent.width
                                    text: modelData.action || "Action"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    font.weight: Theme.font.weightMedium
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: (modelData.details || "") + " · " + (modelData.timestamp || "")
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    elide: Text.ElideRight
                                    width: parent.width
                                }
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
