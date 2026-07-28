// =============================================================================
//  DashboardLayout.qml
// =============================================================================
//  Shell layout for every User-role dashboard page. Renders the signature
//  sidebar + topbar + content frame described by the design system:
//
//      ┌─────────────────────────────────────────────────────────────┐
//      │ ┌──────────┬──────────────────────────────────────────────┐ │
//      │ │          │  TopBar (search · theme · cart · bell · avatar)│ │
//      │ │ Sidebar  ├──────────────────────────────────────────────┤ │
//      │ │          │                                              │ │
//      │ │  Home    │            page child (slotted)              │ │
//      │ │  Discover│                                              │ │
//      │ │  Library │                                              │ │
//      │ │  Cart    │                                              │ │
//      │ │  Notif   │                                              │ │
//      │ │  Profile │                                              │ │
//      │ │  Logout  │                                              │ │
//      │ └──────────┴──────────────────────────────────────────────┘ │
//      └─────────────────────────────────────────────────────────────┘
//
//  Responsive behaviour:
//      • ≥ 1100px : full sidebar (248px), topbar with inline search
//      • 760–1100 : full sidebar, search collapses to icon
//      • < 760    : sidebar collapses to icon-only rail (72px)
//
//  Public API:
//      activeRoute        : string
//      cartCount          : int
//      notificationCount  : int
//      unreadCount        : int
//      userName           : string
//      userInitials       : string
//      pageTitle          : string
//      pageSubtitle       : string
//      default property alias content : _content.data
//
//  Signals (forwarded to the parent router):
//      routeRequested(string route)
//      logoutRequested()
//      searchRequested(string query)
//      notificationsRequested()
//      cartRequested()
//      profileRequested()
//      themeToggled()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/navigation"
import "../components/buttons"

Item {
    id: root

    property string activeRoute: "home"
    property int cartCount: 0
    property int notificationCount: 0
    property int unreadCount: 0
    property string userName: ""
    property string userInitials: "?"
    property string pageTitle: ""
    property string pageSubtitle: ""

    // Sticky search query (kept on the layout so it survives page switches)
    property string searchQuery: ""

    signal routeRequested(string route)
    signal logoutRequested()
    signal searchRequested(string query)
    signal notificationsRequested()
    signal cartRequested()
    signal profileRequested()
    signal themeToggled()

    // Auto-collapse the sidebar on narrow viewports
    readonly property bool _sidebarCollapsed: root.width < 760

    default property alias content: _content.data

    // ----- Page background -----
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // ----- Sidebar -----
    Sidebar {
        id: _sidebar
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root._sidebarCollapsed ? Theme.size.sidebarCollapsedWidth : Theme.size.sidebarWidth
        activeRoute: root.activeRoute
        collapsed: root._sidebarCollapsed
        cartCount: root.cartCount
        notificationCount: root.unreadCount

        Behavior on width {
            NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
        }

        onRouteRequested: root.routeRequested(route)
        onLogoutRequested: root.logoutRequested()
    }

    // ----- Right column (topbar + content) -----
    Item {
        id: _rightCol
        anchors.left: _sidebar.right
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        // ----- TopBar -----
        TopBar {
            id: _topbar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Theme.size.topbarHeight

            title: root.pageTitle
            subtitle: root.pageSubtitle
            userName: root.userName
            userInitials: root.userInitials
            unreadCount: root.unreadCount
            cartCount: root.cartCount
            showSearch: root.width >= 760
            searchQuery: root.searchQuery

            onSearchRequested: root.searchRequested(query)
            onNotificationsRequested: root.notificationsRequested()
            onCartRequested: root.cartRequested()
            onProfileRequested: root.profileRequested()
            onThemeToggled: root.themeToggled()
        }

        // ----- Content slot (plain Item — pages handle their own scrolling) -----
        Item {
            id: _content
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: _topbar.bottom
            anchors.bottom: parent.bottom
            clip: true
        }
    }
}
