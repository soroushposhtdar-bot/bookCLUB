// =============================================================================
//  ServerShell.qml
// =============================================================================
//  Shell for the Server operator role. Mirrors PublisherShell's structure but
//  routes between server-specific pages:
//      overview  → ServerOverviewPage  (KPIs + traffic + services + activity)
//      clients   → ServerClientsPage   (connected clients table)
//      sessions  → ServerSessionsPage  (active sessions + group rooms)
//      database  → ServerDatabasePage  (tables + pool + slow queries)
//      logs      → ServerLogsPage      (server log stream)
//      analytics → ServerAnalyticsPage (requests + endpoints + errors)
//
//  The shell owns its own sidebar (built inline with NavItem) and a top bar,
//  so it does not depend on the user-role Sidebar component.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/navigation"
import "../components/buttons"
import "../components/branding"
import "../components/feedback"
import "../components/data"
import "../components/surfaces"
import "../components/progress"
import "../components/inputs"

import BookClub.Services 1.0
import BookClub.ViewModels 1.0

Item {
    id: _shell

    signal logoutRequested()
    signal themeToggled()
    signal toastRequested(string variant, string title, string description)
    signal settingsRequested()

    // ----- Server ViewModel -----
    ServerViewModel {
        id: _serverVM
        serverService: ServerService
    }

    Component.onCompleted: _serverVM.refresh()

    // ----- Real-time pulse (spec §6.7: "All data must be Real-Time") -----
    // Every 5 seconds we nudge the VM so KPIs (CPU, RAM, client count,
    // query rate) + the activity feed refresh. This matches the pattern
    // already used by AdminShell and PublisherShell.
    Timer {
        interval: 5000
        repeat: true
        running: true
        onTriggered: {
            if (_serverVM) _serverVM.refresh()
        }
    }

    // ----- Current route (drives sidebar active state + page title) -----
    property string activeRoute: "overview"

    readonly property var _routeMeta: ({
        "overview":  { title: "Overview",         subtitle: "Cluster health at a glance" },
        "clients":   { title: "Clients",          subtitle: "Currently connected client sessions" },
        "sessions":  { title: "Sessions",         subtitle: "Active user sessions and reading rooms" },
        "database":  { title: "Database",         subtitle: "Storage, connections, and slow queries" },
        "logs":      { title: "Logs",             subtitle: "Real-time server log stream" },
        "analytics": { title: "Analytics",        subtitle: "Traffic, endpoints, and error breakdown" },
        "profile":   { title: "Profile",          subtitle: "Operator account information" },
        "settings":  { title: "Settings",         subtitle: "Operator preferences" }
    })

    function _navigateTo(route) {
        if (route === "logout") { _shell.logoutRequested(); return }
        activeRoute = route
    }

    function _toast(variant, title, description) {
        _shell.toastRequested(variant, title, description)
    }

    // =========================================================================
    //  Layout: sidebar + topbar + page Loader
    // =========================================================================
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // ----- Sidebar -----
    Rectangle {
        id: _sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Theme.size.sidebarWidth
        color: Theme.color.sidebarBackground

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.space.lg
            anchors.rightMargin: Theme.space.lg
            anchors.topMargin: Theme.space.xl
            anchors.bottomMargin: Theme.space.lg
            spacing: 0

            Row {
                width: parent.width
                height: 48
                spacing: Theme.space.md

                BrandLogo { size: 36; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    text: "BookClub"
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeTitle
                    font.weight: Theme.font.weightBold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item { width: 1; height: Theme.space.md }

            Text {
                width: parent.width
                text: "SERVER OPS"
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeMicro2
                font.weight: Theme.font.weightBold
                elide: Text.ElideRight
            }

            Item { width: 1; height: Theme.space.md }

            Column {
                width: parent.width
                spacing: 2

                NavItem { width: parent.width; iconName: "dashboard";     label: "Overview";  active: _shell.activeRoute === "overview";  onClicked: _shell._navigateTo("overview") }
                NavItem { width: parent.width; iconName: "group";         label: "Clients";   active: _shell.activeRoute === "clients";   onClicked: _shell._navigateTo("clients") }
                NavItem { width: parent.width; iconName: "verified";      label: "Sessions";  active: _shell.activeRoute === "sessions";  onClicked: _shell._navigateTo("sessions") }
                NavItem { width: parent.width; iconName: "database";      label: "Database";  active: _shell.activeRoute === "database";  onClicked: _shell._navigateTo("database") }
                NavItem { width: parent.width; iconName: "terminal";      label: "Logs";      active: _shell.activeRoute === "logs";      badgeCount: 0;  onClicked: _shell._navigateTo("logs") }
                NavItem { width: parent.width; iconName: "analytics";     label: "Analytics"; active: _shell.activeRoute === "analytics"; onClicked: _shell._navigateTo("analytics") }
            }

            Item { width: 1; Layout.fillHeight: true; height: 1 }

            Column {
                width: parent.width
                spacing: 2

                NavItem { width: parent.width; iconName: "account_circle"; label: "Profile";  active: _shell.activeRoute === "profile"; onClicked: _shell._navigateTo("profile") }
                NavItem { width: parent.width; iconName: "settings";       label: "Settings"; active: _shell.activeRoute === "settings"; onClicked: _shell._navigateTo("settings") }
                NavItem { width: parent.width; iconName: "logout";         label: "Sign out"; onClicked: _shell._navigateTo("logout") }
            }
        }
    }

    // ----- Right column -----
    Item {
        id: _rightCol
        anchors.left: _sidebar.right
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        TopBar {
            id: _topbar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Theme.size.topbarHeight

            title: _shell._routeMeta[_shell.activeRoute] ? _shell._routeMeta[_shell.activeRoute].title : ""
            subtitle: _shell._routeMeta[_shell.activeRoute] ? _shell._routeMeta[_shell.activeRoute].subtitle : ""
            userName: AuthService.currentDisplayName
            userInitials: AuthService.currentDisplayName.length > 0 ? AuthService.currentDisplayName.charAt(0).toUpperCase() : "?"
            unreadCount: 0
            cartCount: 0
            showSearch: false
            onThemeToggled: _shell.themeToggled()
        }

        Item {
            id: _content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: _topbar.bottom
            anchors.bottom: parent.bottom
            clip: true

            Loader {
                id: _pageLoader
                anchors.fill: parent
                sourceComponent: _shell._componentForRoute(_shell.activeRoute)
            }
        }
    }

    // =========================================================================
    //  Page components
    // =========================================================================
    readonly property var _componentMap: ({
        "overview":  _overviewComp,
        "clients":   _clientsComp,
        "sessions":  _sessionsComp,
        "database":  _databaseComp,
        "logs":      _logsComp,
        "analytics": _analyticsComp,
        "profile":   _profileComp,
        "settings":  _settingsComp
    })

    function _componentForRoute(route) {
        return _componentMap[route] || _overviewComp
    }

    Component { id: _overviewComp;  ServerOverviewPage  { viewModel: _serverVM; onToastRequested: _shell._toast; onNavigateToRequested: function(route) { _shell._navigateTo(route) } } }
    Component { id: _clientsComp;   ServerClientsPage   { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _sessionsComp;  ServerSessionsPage  { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _databaseComp;  ServerDatabasePage  { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _logsComp;      ServerLogsPage      { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _analyticsComp; ServerAnalyticsPage { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _profileComp;   ServerProfilePage   { viewModel: _serverVM; onToastRequested: _shell._toast } }
    Component { id: _settingsComp;  ServerSettingsPage  { viewModel: _serverVM; onToastRequested: _shell._toast; onLogoutRequested: _shell.logoutRequested } }
}
