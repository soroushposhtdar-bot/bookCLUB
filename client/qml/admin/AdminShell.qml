// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Added "Server Console" route + NavItem + full-screen ServerShell Loader
//   - Sidebar + right column hidden when server route is active
//   - ServerShell.logoutRequested → navigate back to admin dashboard
//   - ServerShell embedded mode for back-navigation

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
//  The shell owns its own sidebar (built inline with NavItem); the TopBar was
//  removed (matching the PublisherShell pattern), so each page renders its
//  own in-page header. A 5-second real-time Timer pulses the VM's refresh()
//  so KPIs + queues update live.
// =============================================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/navigation"
import "../components/buttons"
import "../components/branding"
import "../components/feedback"
import "../components/data"
import "../components/surfaces"
import "../components/progress"
import "../components/inputs"
import "../server"

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
    // Every 60 seconds we nudge the VM so KPI counters + queue badges feel
    // live. The VM's refresh() emits every changed signal so bound QML
    // re-evaluates.
    //
    // BUG 15 (bugfix round 7.8): gate on AuthService.isLoggedIn so the
    // timer STOPS firing after the admin signs out. Previously the timer
    // kept firing every 30s after logout, calling _adminVM.refresh()
    // with an empty userId → server-side handlers crashed or returned
    // junk → client crashed.
    //
    // BUG FIX (refresh timing): changed from Theme.publisher.refreshIntervalMs
    // (30s) to a fixed 60000ms (60s) per user request. The 30s interval was
    // too aggressive — it caused too many synchronous network requests that
    // froze the UI and sometimes crashed the app.
    Timer {
        interval: 60000
        repeat: true
        running: AuthService.isLoggedIn
        onTriggered: {
            if (_adminVM && AuthService.isLoggedIn) _adminVM.refresh()
        }
    }

    // ----- Current route (drives sidebar active state + page title) -----
    property string activeRoute: "dashboard"

    readonly property var _routeMeta: ({
        "dashboard":  { title: "Dashboard",        subtitle: "Platform health at a glance" },
        "users":      { title: "Users",            subtitle: "Manage members, roles, and access" },
        "books":      { title: "Books & content",  subtitle: "Inspect, modify, or remove any title in the system" },
        "publishers": { title: "Publishers",       subtitle: "Filter by publisher role in Users page" },  // v21: hidden nav
        "moderation": { title: "Moderation",       subtitle: "Flagged reviews and reported content" },
        "reports":    { title: "Reports",          subtitle: "Triage incoming user reports" },
        "analytics":  { title: "Analytics",        subtitle: "Usage, engagement, and geography" },
        "server":     { title: "Server Console",   subtitle: "System operations and monitoring" },
        "profile":    { title: "Profile",          subtitle: "Admin account information" },
        "settings":   { title: "Settings",         subtitle: "Admin preferences" }
    })

    function _navigateTo(route) {
        if (route === "logout") { _shell.logoutRequested(); return }
        // QA-196: close any open drawer before swapping routes so the
        // new page doesn't load behind an open drawer.
        if (_userDrawer && _userDrawer.visible) _userDrawer.close()
        if (_bookDrawer && _bookDrawer.visible) _bookDrawer.close()
        // Server console is full-screen — skip the fade and show immediately.
        if (route === "server") {
            _shell.activeRoute = route
            return
        }
        _pageLoader.opacity = 0
        _routeSwitchTimer.route = route
        _routeSwitchTimer.restart()
    }

    // Defer the actual route swap by ~120ms so the fade-out runs first.
    Timer {
        id: _routeSwitchTimer
        property string route: ""
        interval: Theme.motion.durationInstant
        repeat: false
        onTriggered: {
            _shell.activeRoute = route
            _pageLoader.opacity = 1
        }
    }

    function _toast(variant, title, description) {
        _shell.toastRequested(variant, title, description)
    }

    // =========================================================================
    //  Layout: sidebar + page Loader
    // =========================================================================
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // ----- Sidebar -----
    Rectangle {
        id: _sidebar
        visible: _shell.activeRoute !== "server"
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

            // ----- Brand + role chip -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.md

                BrandLogo {
                    size: 36
                    Layout.alignment: Qt.AlignVCenter
                }
                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Text {
                        text: "BookClub"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                    }
                    Text {
                        text: "Admin Console"
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeMicro2
                        font.weight: Theme.font.weightMedium
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: Theme.space.lg
                Layout.bottomMargin: Theme.space.sm
                height: 1
                color: Theme.color.divider
            }

            // ----- Main nav items -----
            Text {
                width: parent.width
                text: "WORKSPACE"
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeMicro2
                font.weight: Theme.font.weightBold
                Layout.topMargin: Theme.space.sm
                Layout.bottomMargin: Theme.space.xs
                Layout.leftMargin: Theme.space.lg
            }

            NavItem { Layout.fillWidth: true; iconName: "dashboard";            label: "Dashboard";  active: _shell.activeRoute === "dashboard";  onClicked: _shell._navigateTo("dashboard") }
            NavItem { Layout.fillWidth: true; iconName: "manage_accounts";      label: "Users";      active: _shell.activeRoute === "users";      onClicked: _shell._navigateTo("users") }
            NavItem { Layout.fillWidth: true; iconName: "library_books";        label: "Books";      active: _shell.activeRoute === "books";      onClicked: _shell._navigateTo("books") }
            NavItem { Layout.fillWidth: true; iconName: "business";             label: "Publishers"; visible: false; active: _shell.activeRoute === "publishers"; onClicked: _shell._navigateTo("publishers") }  // v21: hidden
            NavItem { Layout.fillWidth: true; iconName: "gavel";                label: "Moderation"; active: _shell.activeRoute === "moderation"; badgeCount: _adminVM.flaggedReviewsCount; onClicked: _shell._navigateTo("moderation") }
            NavItem { Layout.fillWidth: true; iconName: "report";               label: "Reports";    active: _shell.activeRoute === "reports";    badgeCount: _adminVM.pendingReports; onClicked: _shell._navigateTo("reports") }
            NavItem { Layout.fillWidth: true; iconName: "analytics";            label: "Analytics";  active: _shell.activeRoute === "analytics";  onClicked: _shell._navigateTo("analytics") }
            NavItem { Layout.fillWidth: true; iconName: "dns";                   label: "Server";     active: _shell.activeRoute === "server";     onClicked: _shell._navigateTo("server") }

            // ----- Spring spacer -----
            Item { Layout.fillHeight: true; Layout.fillWidth: true }

            // ----- Account nav items -----
            Text {
                width: parent.width
                text: "ACCOUNT"
                color: Theme.color.textMuted
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeMicro2
                font.weight: Theme.font.weightBold
                Layout.topMargin: Theme.space.sm
                Layout.bottomMargin: Theme.space.xs
                Layout.leftMargin: Theme.space.lg
            }

            NavItem { Layout.fillWidth: true; iconName: "account_circle"; label: "Profile";  active: _shell.activeRoute === "profile"; onClicked: _shell._navigateTo("profile") }
            NavItem { Layout.fillWidth: true; iconName: "settings";       label: "Settings"; active: _shell.activeRoute === "settings"; onClicked: _shell._navigateTo("settings") }
            NavItem { Layout.fillWidth: true; iconName: "logout";         label: "Sign out"; onClicked: _shell._navigateTo("logout") }
        }
    }

    // ----- Right column -----
    // NOTE: TopBar was removed (matching the PublisherShell pattern). The
    // page components render their own in-page header (SectionHeader etc.)
    // and the sidebar already provides navigation + branding.
    Item {
        id: _rightCol
        visible: _shell.activeRoute !== "server"
        anchors.left: _sidebar.right
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Item {
            id: _content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            clip: true

            Loader {
                id: _pageLoader
                anchors.fill: parent
                sourceComponent: _shell._componentForRoute(_shell.activeRoute)
                opacity: 1.0
                Behavior on opacity {
                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                }
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
        if (route === "server") return null  // Handled by _serverShellLoader
        return _componentMap[route] || _dashboardComp
    }

    Component { id: _dashboardComp;  AdminDashboardPage  { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }; onNavigateToRequested: function(route) { _shell._navigateTo(route) } } }
    Component { id: _usersComp;      AdminUsersPage      { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }; onOpenUserDetail: _shell._openUserDrawer } }
    Component { id: _booksComp;      AdminBooksPage      { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }; onOpenBookDetail: _shell._openBookDrawer } }
    Component { id: _publishersComp; AdminPublishersPage { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _moderationComp; AdminModerationPage { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _reportsComp;    AdminReportsPage    { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _analyticsComp;  AdminAnalyticsPage  { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _profileComp;    AdminProfilePage    { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _settingsComp;   AdminSettingsPage   { viewModel: _adminVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }; onLogoutRequested: _shell.logoutRequested } }

    // ----- User detail drawer (overlay, used by Users page) -----
    AdminUserDetailDrawer {
        id: _userDrawer
        viewModel: _adminVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
    }

    function _openUserDrawer(username) {
        _userDrawer.openForUser(username)
    }

    // ----- Book detail drawer (overlay, used by Books page) -----
    AdminBookDetailDrawer {
        id: _bookDrawer
        viewModel: _adminVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
    }

    function _openBookDrawer(bookId) {
        _bookDrawer.openForBook(bookId)
    }

    // =========================================================================
    //  Server Console (full-screen, shown when route is "server")
    // =========================================================================
    //  The admin can open the Server Console from the sidebar. It loads
    //  ServerShell as a full-screen overlay (covering the admin sidebar +
    //  content area). When the operator clicks "Back to Admin" (the Sign
    //  out NavItem in embedded mode), we navigate back to the admin
    //  dashboard. Toasts + theme toggles are forwarded to the parent.
    // =========================================================================
    Loader {
        id: _serverShellLoader
        anchors.fill: parent
        active: _shell.activeRoute === "server"
        visible: active
        z: 100
        sourceComponent: Component {
            ServerShell {
                embedded: true
                onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
                onLogoutRequested: _shell._navigateTo("dashboard")
                onThemeToggled: _shell.themeToggled()
            }
        }
    }
}
