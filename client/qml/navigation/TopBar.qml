// =============================================================================
//  TopBar.qml
// =============================================================================
//  Dashboard top bar — page title (or breadcrumb), global search affordance,
//  theme toggle, notifications bell, user avatar menu trigger.
//
//  Public API:
//      title          : string
//      subtitle       : string  (optional)
//      userName       : string
//      userInitials   : string
//      unreadCount    : int
//      cartCount      : int
//      showSearch     : bool    (render the inline search field)
//      searchQuery    : string  (bound to the search field)
//
//  Signals:
//      searchRequested(string query)
//      notificationsRequested()
//      cartRequested()
//      profileRequested()
//      themeToggled()
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"
import "../inputs"
import "./"

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    property string userName: ""
    property string userInitials: "?"
    property int unreadCount: 0
    property int cartCount: 0
    property bool showSearch: true
    property string searchQuery: ""

    signal searchRequested(string query)
    signal notificationsRequested()
    signal cartRequested()
    signal profileRequested()
    signal themeToggled()

    implicitHeight: Theme.size.topbarHeight

    Rectangle {
        anchors.fill: parent
        color: Theme.color.topbarBackground

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.color.divider
        }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.space.xxl
        anchors.rightMargin: Theme.space.xl
        spacing: Theme.space.lg

        // ----- Title block -----
        Column {
            visible: root.title.length > 0
            spacing: 0
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: root.title
                color: Theme.color.textPrimary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeHeadline
                font.weight: Theme.font.weightBold
                elide: Text.ElideRight
            }
            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.color.textSecondary
                font.family: Theme.font.family
                font.pixelSize: Theme.font.sizeCaption
                font.weight: Theme.font.weightRegular
            }
        }

        // ----- Search (flexes to fill) -----
        Item {
            id: _searchWrap
            width: root.showSearch ? Math.min(420, root.width * 0.40) : 0
            height: 40
            anchors.verticalCenter: parent.verticalCenter
            visible: root.showSearch

            SearchField {
                anchors.fill: parent
                text: root.searchQuery
                placeholder: "Search books, authors, publishers..."
                onAccepted: root.searchRequested(text)
                onTextEdited: root.searchQuery = newText
            }
        }

        Item { width: 1; height: 1; Layout.fillWidth: true }

        // ----- Theme toggle -----
        IconButton {
            iconName: "dark_mode"
            iconColor: Theme.color.textSecondary
            hoverIconColor: Theme.color.textPrimary
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.themeToggled()
        }

        // ----- Cart icon -----
        Item {
            width: 40
            height: 40
            anchors.verticalCenter: parent.verticalCenter

            IconButton {
                anchors.fill: parent
                iconName: "shopping_cart"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                onClicked: root.cartRequested()
            }

            Rectangle {
                visible: root.cartCount > 0
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 4
                anchors.rightMargin: 4
                width: Math.max(18, _cartBadge.implicitWidth + 10)
                height: 18
                radius: 9
                color: Theme.color.primary

                Text {
                    id: _cartBadge
                    anchors.centerIn: parent
                    text: root.cartCount > 99 ? "99+" : String(root.cartCount)
                    color: Theme.color.onPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeMicro
                    font.weight: Theme.font.weightBold
                }
            }
        }

        // ----- Notifications bell -----
        Item {
            width: 40
            height: 40
            anchors.verticalCenter: parent.verticalCenter

            IconButton {
                anchors.fill: parent
                iconName: "notifications"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                onClicked: root.notificationsRequested()
            }

            Rectangle {
                visible: root.unreadCount > 0
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 4
                anchors.rightMargin: 4
                width: Math.max(18, _bellBadge.implicitWidth + 10)
                height: 18
                radius: 9
                color: Theme.color.accent

                Text {
                    id: _bellBadge
                    anchors.centerIn: parent
                    text: root.unreadCount > 99 ? "99+" : String(root.unreadCount)
                    color: Theme.color.textOnAccent
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeMicro
                    font.weight: Theme.font.weightBold
                }
            }
        }

        // ----- User avatar -----
        Item {
            width: Theme.size.avatarSize
            height: Theme.size.avatarSize
            anchors.verticalCenter: parent.verticalCenter

            Avatar {
                anchors.fill: parent
                initials: root.userInitials
                size: Theme.size.avatarSize
                online: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.profileRequested()
            }
        }
    }
}
