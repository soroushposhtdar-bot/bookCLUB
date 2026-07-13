// =============================================================================
//  PublisherShell.qml
// =============================================================================
//  Shell for the Publisher role. Mirrors UserShell's structure but routes
//  between publisher-specific pages:
//      dashboard   → PublisherDashboardPage   (KPIs + revenue + recent activity)
//      catalog     → PublisherCatalogPage     (book table + status + price)
//      sales       → PublisherSalesPage       (charts + top books + revenue)
//      promotions  → PublisherPromotionsPage  (discounts + promo codes)
//      notifications → PublisherNotificationsPage (publisher-specific alerts)
//      profile     → PublisherProfilePage     (account info + edit + stats)
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

    // ----- Publisher ViewModel (owned by the shell, passed down to each
    //       page via the `viewModel` property) -----
    PublisherViewModel {
        id: _publisherVM
        publisherService: PublisherService
    }

    Component.onCompleted: _publisherVM.refresh()

    // ----- Real-time pulse -----
    // Every 5 seconds we nudge the VM so KPI counters + recent-orders feed
    // feel live. The VM's refresh() emits every changed signal so bound QML
    // re-evaluates. (The mock seeds don't change, but a real socket-backed
    // VM would push new data here.)
    Timer {
        interval: 5000
        repeat: true
        running: true
        onTriggered: {
            if (_publisherVM) _publisherVM.refresh()
        }
    }

    signal logoutRequested()
    signal themeToggled()
    signal toastRequested(string variant, string title, string description)

    // ----- Current route (drives sidebar active state + page title) -----
    property string activeRoute: "dashboard"

    readonly property var _routeMeta: ({
        "dashboard":     { title: "Dashboard",       subtitle: "Catalog performance at a glance" },
        "catalog":       { title: "Catalog",         subtitle: "Manage your published titles" },
        "sales":         { title: "Sales Analytics", subtitle: "Revenue, units, and trends" },
        "promotions":    { title: "Promotions",      subtitle: "Discounts and promo codes" },
        "notifications": { title: "Notifications",   subtitle: "Alerts from the platform and your readers" },
        "profile":       { title: "Profile",         subtitle: "Account information and catalog stats" }
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
                text: "PUBLISHER"
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

                NavItem { width: parent.width; iconName: "dashboard";     label: "Dashboard";     active: _shell.activeRoute === "dashboard";     onClicked: _shell._navigateTo("dashboard") }
                NavItem { width: parent.width; iconName: "library_books"; label: "Catalog";       active: _shell.activeRoute === "catalog";       onClicked: _shell._navigateTo("catalog") }
                NavItem { width: parent.width; iconName: "bar_chart";     label: "Sales";         active: _shell.activeRoute === "sales";         onClicked: _shell._navigateTo("sales") }
                NavItem { width: parent.width; iconName: "local_offer";   label: "Promotions";    active: _shell.activeRoute === "promotions";    onClicked: _shell._navigateTo("promotions") }
                NavItem { width: parent.width; iconName: "notifications"; label: "Notifications"; active: _shell.activeRoute === "notifications"; badgeCount: _shell._unreadNotifCount; onClicked: _shell._navigateTo("notifications") }
            }

            Item { width: 1; Layout.fillHeight: true; height: 1 }

            Column {
                width: parent.width
                spacing: 2

                NavItem { width: parent.width; iconName: "account_circle"; label: "Profile";  active: _shell.activeRoute === "profile"; onClicked: _shell._navigateTo("profile") }
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
            unreadCount: _shell._unreadNotifCount
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
        "dashboard":     _dashboardComp,
        "catalog":       _catalogComp,
        "sales":         _salesComp,
        "promotions":    _promotionsComp,
        "notifications": _notificationsComp,
        "profile":       _profileComp
    })

    function _componentForRoute(route) {
        return _componentMap[route] || _dashboardComp
    }

    Component { id: _dashboardComp;     PublisherDashboardPage     { viewModel: _publisherVM; onToastRequested: _shell._toast; onNavigateToRequested: function(route) { _shell._navigateTo(route) } } }
    Component { id: _catalogComp;       PublisherCatalogPage       { viewModel: _publisherVM; onToastRequested: _shell._toast; onOpenBookDetail: _shell._openBookDrawer; pendingEditBookId: _shell._pendingEditBookId } }
    Component { id: _salesComp;         PublisherSalesPage         { viewModel: _publisherVM; onToastRequested: _shell._toast } }
    Component { id: _promotionsComp;    PublisherPromotionsPage    { viewModel: _publisherVM; onToastRequested: _shell._toast } }
    Component { id: _notificationsComp; PublisherNotificationsPage { viewModel: _publisherVM; onToastRequested: _shell._toast } }
    Component { id: _profileComp;       PublisherProfilePage       { viewModel: _publisherVM; onToastRequested: _shell._toast } }

    // ----- Book detail drawer (overlay, used by Catalog page) -----
    PublisherBookDetailDrawer {
        id: _bookDrawer
        viewModel: _publisherVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: _shell._toast
        onEditRequested: function(bookId) {
            // Close the drawer, switch to the Catalog tab, and set the
            // pending-edit book ID. The catalog page watches this property
            // via a Connections block and opens its editor when it changes.
            _shell._pendingEditBookId = bookId
            _shell.activeRoute = "catalog"
        }
    }

    // ----- Pending edit-book ID (set by the drawer, consumed by the catalog page) -----
    //   When the user clicks "Edit metadata" in the drawer, we set this
    //   property and switch to the Catalog route. The catalog page's
    //   Connections block picks it up and opens the editor in edit mode.
    property string _pendingEditBookId: ""

    function _openBookDrawer(bookId) {
        _bookDrawer.openForBook(bookId)
    }

    // ----- Unread-notification count (live binding from the VM) -----
    readonly property int _unreadNotifCount: {
        if (!_publisherVM || !_publisherVM.publisherNotifications) return 0
        let n = 0
        const list = _publisherVM.publisherNotifications
        for (let i = 0; i < list.length; ++i) {
            if (list[i].read === false) ++n
        }
        return n
    }
}
