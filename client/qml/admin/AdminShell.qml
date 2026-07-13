// =============================================================================
//  AdminShell.qml
// =============================================================================
//  Shell for the Admin role. Mirrors PublisherShell's structure but routes
//  between admin-specific pages:
//      dashboard  → AdminDashboardPage   (KPIs + user growth + system health)
//      users      → AdminUsersPage        (user table + roles + status + detail drawer)
//      books      → AdminBooksPage        (book & content management §4-3)
//      publishers → AdminPublishersPage   (approvals + active publishers)
//      moderation → AdminModerationPage   (flagged reviews + reported content)
//      reports    → AdminReportsPage      (reports queue + filters)
//      analytics  → AdminAnalyticsPage    (DAU/MAU + bar charts + geo)
//
//  The shell owns its own sidebar (built inline with NavItem) and a top bar,
//  so it does not depend on the user-role Sidebar component. A 5-second
//  real-time Timer pulses the VM's refresh() so KPIs + queues update live.
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

    // ----- Admin ViewModel -----
    AdminViewModel {
        id: _adminVM
        adminService: AdminService
    }

    Component.onCompleted: _adminVM.refresh()

    // ----- Real-time pulse -----
    // Every 5 seconds we nudge the VM so KPI counters + queue badges feel
    // live. The VM's refresh() emits every changed signal so bound QML
    // re-evaluates. (The mock seeds don't change, but a real socket-backed
    // VM would push new data here.)
    Timer {
        interval: 5000
        repeat: true
        running: true
        onTriggered: {
            if (_adminVM) _adminVM.refresh()
        }
    }

    // ----- Current route (drives sidebar active state + page title) -----
    property string activeRoute: "dashboard"

    readonly property var _routeMeta: ({
        "dashboard":  { title: "Dashboard",        subtitle: "Platform health at a glance" },
        "users":      { title: "Users",            subtitle: "Manage members, roles, and access" },
        "books":      { title: "Books & content",  subtitle: "Inspect, modify, or remove any title in the system" },
        "publishers": { title: "Publishers",       subtitle: "Approvals and publisher accounts" },
        "moderation": { title: "Moderation",       subtitle: "Flagged reviews and reported content" },
        "reports":    { title: "Reports",          subtitle: "Triage incoming user reports" },
        "analytics":  { title: "Analytics",        subtitle: "Usage, engagement, and geography" },
        "profile":    { title: "Profile",          subtitle: "Admin account information" },
        "settings":   { title: "Settings",         subtitle: "Admin preferences" }
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
                text: "ADMIN"
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

                NavItem { width: parent.width; iconName: "dashboard";            label: "Dashboard";  active: _shell.activeRoute === "dashboard";  onClicked: _shell._navigateTo("dashboard") }
                NavItem { width: parent.width; iconName: "manage_accounts";      label: "Users";      active: _shell.activeRoute === "users";      onClicked: _shell._navigateTo("users") }
                NavItem { width: parent.width; iconName: "library_books";        label: "Books";      active: _shell.activeRoute === "books";      onClicked: _shell._navigateTo("books") }
                NavItem { width: parent.width; iconName: "business";             label: "Publishers"; active: _shell.activeRoute === "publishers"; onClicked: _shell._navigateTo("publishers") }
                NavItem { width: parent.width; iconName: "gavel";                label: "Moderation"; active: _shell.activeRoute === "moderation"; badgeCount: _adminVM.pendingReports; onClicked: _shell._navigateTo("moderation") }
                NavItem { width: parent.width; iconName: "report";               label: "Reports";    active: _shell.activeRoute === "reports";    badgeCount: _adminVM.pendingReports; onClicked: _shell._navigateTo("reports") }
                NavItem { width: parent.width; iconName: "analytics";            label: "Analytics";  active: _shell.activeRoute === "analytics";  onClicked: _shell._navigateTo("analytics") }
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
        "dashboard":  _dashboardComp,
        "users":      _usersComp,
        "books":      _booksComp,
        "publishers": _publishersComp,
        "moderation": _moderationComp,
        "reports":    _reportsComp,
        "analytics":  _analyticsComp,
        "profile":    _profileComp,
        "settings":   _settingsComp
    })

    function _componentForRoute(route) {
        return _componentMap[route] || _dashboardComp
    }

    Component { id: _dashboardComp;  AdminDashboardPage  { viewModel: _adminVM; onToastRequested: _shell._toast; onNavigateToRequested: function(route) { _shell._navigateTo(route) } } }
    Component { id: _usersComp;      AdminUsersPage      { viewModel: _adminVM; onToastRequested: _shell._toast; onOpenUserDetail: _shell._openUserDrawer } }
    Component { id: _booksComp;      AdminBooksPage      { viewModel: _adminVM; onToastRequested: _shell._toast; onOpenBookDetail: _shell._openBookDrawer } }
    Component { id: _publishersComp; AdminPublishersPage { viewModel: _adminVM; onToastRequested: _shell._toast } }
    Component { id: _moderationComp; AdminModerationPage { viewModel: _adminVM; onToastRequested: _shell._toast } }
    Component { id: _reportsComp;    AdminReportsPage    { viewModel: _adminVM; onToastRequested: _shell._toast } }
    Component { id: _analyticsComp;  AdminAnalyticsPage  { viewModel: _adminVM; onToastRequested: _shell._toast } }
    Component { id: _profileComp;    AdminProfilePage    { viewModel: _adminVM; onToastRequested: _shell._toast } }
    Component { id: _settingsComp;   AdminSettingsPage   { viewModel: _adminVM; onToastRequested: _shell._toast; onLogoutRequested: _shell.logoutRequested } }

    // ----- User detail drawer (overlay, used by Users page) -----
    AdminUserDetailDrawer {
        id: _userDrawer
        viewModel: _adminVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: _shell._toast
    }

    function _openUserDrawer(username) {
        _userDrawer.openForUser(username)
    }

    // ----- Book detail drawer (overlay, used by Books page) -----
    AdminBookDetailDrawer {
        id: _bookDrawer
        viewModel: _adminVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: _shell._toast
    }

    function _openBookDrawer(bookId) {
        _bookDrawer.openForBook(bookId)
    }
}
