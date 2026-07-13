// =============================================================================
//  Sidebar.qml
// =============================================================================
//  Dashboard left navigation rail. Renders the brand logo, primary nav items,
//  a spacer, and a footer (settings + logout).
//
//  Public API:
//      activeRoute    : string — currently selected route key
//      collapsed      : bool   — collapse to icon-only rail (narrow viewports)
//      cartCount      : int    — items in cart (badge on Cart nav item)
//      notificationCount : int — unread notifications (badge on Notifications)
//
//  Signals:
//      routeRequested(string route)
//      logoutRequested()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../branding"
import "../buttons"
import "./"

Item {
    id: root

    property string activeRoute: "home"
    property bool collapsed: false
    property int cartCount: 0
    property int notificationCount: 0

    signal routeRequested(string route)
    signal logoutRequested()

    Rectangle {
        anchors.fill: parent
        color: Theme.color.sidebarBackground
        border.color: Theme.color.divider
        border.width: 0

        // Right edge divider
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: Theme.space.lg
        anchors.rightMargin: Theme.space.lg
        anchors.topMargin: Theme.space.xl
        anchors.bottomMargin: Theme.space.lg
        spacing: 0

        // ----- Brand -----
        Row {
            width: parent.width
            height: 48
            spacing: Theme.space.md
            layoutDirection: Qt.LeftToRight

            BrandLogo {
                size: root.collapsed ? 32 : 36
                anchors.verticalCenter: parent.verticalCenter
                x: root.collapsed ? (parent.width - 32) / 2 : 0
            }

            Text {
                visible: !root.collapsed
                text: "BookClub"
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeTitle
                font.weight: Theme.font.weightBold
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Item { width: 1; height: Theme.space.xxl }

        // ----- Primary nav -----
        Column {
            width: parent.width
            spacing: 2

            NavItem {
                width: parent.width
                iconName: "home"
                label: "Home"
                active: root.activeRoute === "home"
                collapsed: root.collapsed
                onClicked: root.routeRequested("home")
            }
            NavItem {
                width: parent.width
                iconName: "explore"
                label: "Discover"
                active: root.activeRoute === "search"
                collapsed: root.collapsed
                onClicked: root.routeRequested("search")
            }
            NavItem {
                width: parent.width
                iconName: "library_books"
                label: "Library"
                active: root.activeRoute === "library"
                collapsed: root.collapsed
                onClicked: root.routeRequested("library")
            }
            NavItem {
                width: parent.width
                iconName: "shelves"
                label: "Shelves"
                active: root.activeRoute === "shelves"
                collapsed: root.collapsed
                onClicked: root.routeRequested("shelves")
            }
            NavItem {
                width: parent.width
                iconName: "groups"
                label: "Group Reading"
                active: root.activeRoute === "groupReading"
                collapsed: root.collapsed
                onClicked: root.routeRequested("groupReading")
            }
            NavItem {
                width: parent.width
                iconName: "shopping_cart"
                label: "Cart"
                active: root.activeRoute === "cart"
                collapsed: root.collapsed
                badgeCount: root.cartCount
                onClicked: root.routeRequested("cart")
            }
            NavItem {
                width: parent.width
                iconName: "notifications"
                label: "Notifications"
                active: root.activeRoute === "notifications"
                collapsed: root.collapsed
                badgeCount: root.notificationCount
                onClicked: root.routeRequested("notifications")
            }
        }

        Item { width: 1; Layout.fillHeight: true; height: 1 }

        // ----- Footer nav -----
        Column {
            width: parent.width
            spacing: 2

            NavItem {
                width: parent.width
                iconName: "favorite_border"
                label: "Wishlist"
                active: root.activeRoute === "wishlist"
                collapsed: root.collapsed
                onClicked: root.routeRequested("wishlist")
            }
            NavItem {
                width: parent.width
                iconName: "account_circle"
                label: "Profile"
                active: root.activeRoute === "profile"
                collapsed: root.collapsed
                onClicked: root.routeRequested("profile")
            }
            NavItem {
                width: parent.width
                iconName: "settings"
                label: "Settings"
                active: root.activeRoute === "settings"
                collapsed: root.collapsed
                onClicked: root.routeRequested("settings")
            }
            NavItem {
                width: parent.width
                iconName: "logout"
                label: "Sign out"
                collapsed: root.collapsed
                onClicked: root.logoutRequested()
            }
        }
    }
}
