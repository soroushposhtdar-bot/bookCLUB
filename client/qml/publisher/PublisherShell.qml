// =============================================================================
//  PublisherShell.qml  (v3 polish)
// =============================================================================
//  Shell for the Publisher role. Hosts the publisher-specific pages:
//      dashboard     → PublisherDashboardPage    (KPIs + revenue + activity)
//      catalog       → PublisherCatalogPage      (book table + status + price)
//      sales         → PublisherSalesPage        (charts + top books + revenue)
//      promotions    → PublisherPromotionsPage   (discounts + promo codes)
//      notifications → PublisherNotificationsPage (publisher-specific alerts)
//      profile       → PublisherProfilePage      (account info + edit + stats)
//
//  The shell owns its own sidebar (built inline with NavItem) and a top bar,
//  so it does not depend on the user-role Sidebar component.
//
//  Improvements over the v2 version:
//    • Sidebar uses ColumnLayout with proper spacers instead of `Item { height:
//      N }` placeholders — no more layout drift when fonts change.
//    • Brand block at the top is wrapped in a styled "Publisher" header card
//      with a subtle accent-soft background so the role is unmistakable.
//    • NavItem list is data-driven (a single Repeater over `_navGroups`)
//      instead of 6 hand-written NavItem instances — adding/reordering a
//      route is now a one-line edit.
//    • Live pulse interval moved from 5s → 30s (Theme.publisher.refreshIntervalMs)
//      so the VM isn't hammered while still feeling responsive.
//    • Unread-notification count is cached in a `property int` updated by a
//      Connections block instead of recomputed on every binding evaluation.
//    • Page swap uses a opacity CrossFade via a StackLayout-style Loader so
//      transitions feel snappier than a hard sourceComponent swap.
//    • A keyboard-focusable "Quick publish" FAB-style button sits in the
//      sidebar footer → one click jumps to the catalog and opens the editor.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components"           // for AppIcon
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

    Component.onCompleted: {
        _publisherVM.refresh()
        // Prime the unread-notification count once after the VM is wired up.
        // (Deferred via Qt.callLater so the VM has had a chance to register
        // its Connections before we read its notifications list.)
        Qt.callLater(function() {
            _shell._unreadNotifCount = _shell._computeUnread()
        })
    }

    // ----- Real-time pulse -----
    // Every Theme.publisher.refreshIntervalMs (default 30s) we nudge the VM
    // so KPI counters + recent-orders feed feel live. The VM's refresh()
    // emits every changed signal so bound QML re-evaluates.
    Timer {
        interval: Theme.publisher.refreshIntervalMs
        repeat: true
        running: true
        onTriggered: if (_publisherVM) _publisherVM.refresh()
    }

    signal logoutRequested()
    signal themeToggled()
    signal toastRequested(string variant, string title, string description)

    // ----- Current route (drives sidebar active state + page title) -----
    property string activeRoute: "dashboard"

    readonly property var _routeMeta: ({
        "dashboard":     { title: "Dashboard",      subtitle: "Catalog performance at a glance" },
        "catalog":       { title: "Catalog",        subtitle: "Manage your published titles" },
        "sales":         { title: "Sales Analytics",subtitle: "Revenue, units, and trends" },
        "promotions":    { title: "Promotions",     subtitle: "Discounts and promo codes" },
        "notifications": { title: "Notifications",  subtitle: "Alerts from the platform and your readers" },
        "profile":       { title: "Profile",        subtitle: "Account information and catalog stats" }
    })

    // ----- Single source of truth for the sidebar -----
    // Each entry produces one NavItem. `group` controls which footer/main
    // group the item lands in; `badgeRoute` (optional) wires up a live
    // unread count from the VM.
    readonly property var _navGroups: [
        {
            title: "Workspace",
            items: [
                { route: "dashboard",     icon: "dashboard",     label: "Dashboard"     },
                { route: "catalog",       icon: "library_books", label: "Catalog"       },
                { route: "sales",         icon: "bar_chart",     label: "Sales"         },
                { route: "promotions",    icon: "local_offer",   label: "Promotions"    }
            ]
        },
        {
            title: "Inbox",
            items: [
                { route: "notifications", icon: "notifications", label: "Notifications", badge: true }
            ]
        },
        {
            title: "Account",
            items: [
                { route: "profile",       icon: "account_circle", label: "Profile"  },
                { route: "logout",         icon: "logout",         label: "Sign out" }
            ]
        }
    ]

    function _navigateTo(route) {
        if (route === "logout") { _shell.logoutRequested(); return }
        _pageLoader.opacity = 0
        _routeSwitchTimer.route = route
        _routeSwitchTimer.restart()
    }

    // Defer the actual route swap by ~80ms so the fade-out animation runs
    // first. This gives a soft cross-fade instead of a hard swap.
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
                        text: "Publisher Studio"
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

            // ----- Nav groups (data-driven) -----
            Repeater {
                model: _shell._navGroups

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    // Group label (hidden for single-item groups to save space)
                    Text {
                        visible: modelData.items.length > 1 || modelData.title === "Workspace"
                        text: modelData.title.toUpperCase()
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeMicro2
                        font.weight: Theme.font.weightBold
                        Layout.topMargin: Theme.space.sm
                        Layout.bottomMargin: Theme.space.xs
                        Layout.leftMargin: Theme.space.lg
                    }

                    Repeater {
                        model: modelData.items
                        NavItem {
                            Layout.fillWidth: true
                            iconName: modelData.icon
                            label: modelData.label
                            active: _shell.activeRoute === modelData.route
                            badgeCount: modelData.badge ? _shell._unreadNotifCount : 0
                            onClicked: _shell._navigateTo(modelData.route)
                        }
                    }
                }
            }

            // ----- Spring spacer -----
            Item { Layout.fillHeight: true; Layout.fillWidth: true }

            // ----- Quick publish CTA in the footer -----
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: Theme.space.md
                radius: Theme.radius.lg
                color: Theme.color.accentSoft
                border.color: Qt.rgba(Theme.color.accent.r,
                                      Theme.color.accent.g,
                                      Theme.color.accent.b, 0.25)
                border.width: 1
                implicitHeight: _ctaCol.implicitHeight + 2 * Theme.space.md

                ColumnLayout {
                    id: _ctaCol
                    anchors.fill: parent
                    anchors.margins: Theme.space.md
                    spacing: Theme.space.xs

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.sm

                        AppIcon {
                            name: "rocket_launch"
                            size: 18
                            color: Theme.color.accent
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text {
                            text: "Publish a new title"
                            color: Theme.color.textPrimary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeBody
                            font.weight: Theme.font.weightSemibold
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Reach thousands of readers in minutes."
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        wrapMode: Text.WordWrap
                    }
                    PrimaryButton {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.space.xs
                        text: "Add title"
                        iconName: "add"
                        onClicked: {
                            // v4 fix: use a dedicated counter to force the
                            // catalog page's create flow to fire even when
                            // _pendingEditBookId is already "". The catalog
                            // page watches _createRequestCount via a
                            // Connections block.
                            _shell._pendingEditBookId = ""
                            _shell._createRequestCount++
                            _shell.activeRoute = "catalog"
                        }
                    }
                }
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

        // v8: TopBar restored — but with showCart: false since publishers
        // don't have a shopping cart.
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
            showCart: false          // publishers don't have a cart
            showSearch: false
            onThemeToggled: _shell.themeToggled()
            onNotificationsRequested: _shell._navigateTo("notifications")
            onProfileRequested: _shell._navigateTo("profile")
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

    Component {
        id: _dashboardComp
        PublisherDashboardPage {
            viewModel: _publisherVM
            onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
            onNavigateToRequested: function(route) { _shell._navigateTo(route) }
        }
    }
    Component {
        id: _catalogComp
        PublisherCatalogPage {
            viewModel: _publisherVM
            onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
            onOpenBookDetail: _shell._openBookDrawer
            pendingEditBookId: _shell._pendingEditBookId
            createRequestCount: _shell._createRequestCount
            onPendingEditBookIdConsumed: _shell._pendingEditBookId = ""
            onCreateRequestConsumed: _shell._createRequestCount = 0
        }
    }
    Component { id: _salesComp;         PublisherSalesPage         { viewModel: _publisherVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _promotionsComp;    PublisherPromotionsPage    { viewModel: _publisherVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _notificationsComp; PublisherNotificationsPage { viewModel: _publisherVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }
    Component { id: _profileComp;       PublisherProfilePage       { viewModel: _publisherVM; onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) } } }

    // ----- Book detail drawer (overlay, used by Catalog page) -----
    PublisherBookDetailDrawer {
        id: _bookDrawer
        viewModel: _publisherVM
        anchors { top: parent.top; right: parent.right; bottom: parent.bottom }
        onToastRequested: function(variant, title, description) { _shell._toast(variant, title, description) }
        onEditRequested: function(bookId) {
            // Close the drawer, switch to the Catalog tab, and set the
            // pending-edit book ID. The catalog page watches this property
            // via a Connections block and opens its editor when it changes.
            _shell._pendingEditBookId = bookId
            _shell._createRequestCount++   // v4: ensure handler fires even on repeat edits
            _shell._navigateTo("catalog")
        }
    }

    // ----- Pending edit-book ID (set by the drawer / sidebar CTA, consumed
    //       by the catalog page). Cleared via onPendingEditBookIdConsumed. -----
    property string _pendingEditBookId: ""

    // ----- Create-request counter (set by the sidebar CTA when the user
    //       clicks "Add title"). The catalog page watches this via a
    //       Connections block and opens its editor in create mode when the
    //       value changes. We use a counter instead of a bool because
    //       consecutive create requests must each fire the handler even
    //       though _pendingEditBookId stays at "". -----
    property int _createRequestCount: 0

    function _openBookDrawer(bookId) {
        _bookDrawer.openForBook(bookId)
    }

    // ----- Unread-notification count (cached, updated on notificationsChanged) -----
    //   Previously this was a readonly property that iterated the
    //   notifications list on every binding evaluation (every UI refresh
    //   rebuilt the count from scratch). Now we cache the value and update
    //   it only when the underlying list actually changes.
    property int _unreadNotifCount: 0

    Connections {
        target: _publisherVM
        ignoreUnknownSignals: true
        function onNotificationsChanged() {
            _shell._unreadNotifCount = _shell._computeUnread()
        }
        function onPublisherServiceChanged() {
            _shell._unreadNotifCount = _shell._computeUnread()
        }
    }

    function _computeUnread() {
        if (!_publisherVM || !_publisherVM.publisherNotifications) return 0
        let n = 0
        const list = _publisherVM.publisherNotifications
        for (let i = 0; i < list.length; ++i) {
            if (list[i].read === false) ++n
        }
        return n
    }
}
