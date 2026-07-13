// =============================================================================
//  UserShell.qml
// =============================================================================
//  Owns the post-login dashboard experience for the Regular User role.
//
//  Responsibilities:
//      • Instantiates every User ViewModel + injects the shared services.
//      • Routes between the 9 User pages via a StackView.
//      • Holds the active route key so the Sidebar highlights the right item.
//      • Forwards top-level events (search, theme toggle, sign out, cart
//        count changes, real-time notification toasts) to/from App.qml.
//
//  Page → route map:
//      home           → HomePage
//      search         → SearchPage
//      bookDetail     → BookDetailPage (pushed on top of any page)
//      cart           → CartPage
//      checkoutSuccess→ SuccessPage (brief, then pops to library)
//      library        → LibraryPage
//      reader         → PdfReaderPage (pushed full-screen)
//      notifications  → NotificationsPage
//      profile        → ProfilePage
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/feedback"
import "../layouts"

import BookClub.ViewModels 1.0
import BookClub.Services 1.0

Item {
    id: _shell

    // ----- Service singletons (injected from App.qml via context property) -----
    // BookService, CartService, LibraryService, NotificationService, ReaderService,
    // UserService are all QML singletons registered in main.cpp.

    signal logoutRequested()
    signal themeToggled()

    // ----- User ViewModels -----
    HomeViewModel           { id: _homeVM;           bookService: BookService; userService: UserService }
    SearchViewModel         { id: _searchVM;         bookService: BookService; cartService: CartService }
    BookDetailViewModel     { id: _bookDetailVM;     bookService: BookService; cartService: CartService; readerService: ReaderService }
    CartViewModel           { id: _cartVM;           cartService: CartService }
    LibraryViewModel        { id: _libraryVM;        libraryService: LibraryService }
    ReaderViewModel         { id: _readerVM;         readerService: ReaderService }
    NotificationsViewModel  { id: _notificationsVM;  service: NotificationService }
    ProfileViewModel        { id: _profileVM;        userService: UserService }
    WishlistViewModel       { id: _wishlistVM;       libraryService: LibraryService; cartService: CartService }
    SettingsViewModel       { id: _settingsVM;       userService: UserService }
    ShelfViewModel          { id: _shelfVM;          libraryService: LibraryService }
    StudySessionViewModel   { id: _studySessionVM }

    // ----- Current route (drives Sidebar active state + TopBar title) -----
    property string activeRoute: "home"

    // ----- Toast helper (forwarded to App.qml) -----
    signal toastRequested(string variant, string title, string description)

    // ----- Page title/subtitle per route -----
    readonly property var _routeMeta: ({
        "home":          { title: "Home",            subtitle: "Your reading world, all in one place" },
        "search":        { title: "Discover",        subtitle: "Find your next great read" },
        "bookDetail":    { title: "",                subtitle: "" },
        "cart":          { title: "Cart",            subtitle: "Review your selections" },
        "library":       { title: "Library",         subtitle: "Your books, saved, and shelves" },
        "shelves":       { title: "My Shelves",      subtitle: "Organize your library your way" },
        "groupReading":  { title: "Group Reading",   subtitle: "Read together, in sync" },
        "wishlist":      { title: "Wishlist",        subtitle: "Books you've saved for later" },
        "notifications": { title: "Notifications",   subtitle: "Stay up to date" },
        "profile":       { title: "Profile",         subtitle: "Manage your account" },
        "settings":      { title: "Settings",        subtitle: "Make BookClub yours" }
    })

    // =========================================================================
    //  Root container — every User page (except the reader) lives inside a
    //  DashboardLayout that provides the sidebar + topbar chrome.
    //  The reader is rendered as a full-screen overlay on top.
    //
    //  The page Loader is declared INSIDE DashboardLayout so it becomes a
    //  child of the layout's content Column via the default property alias.
    // =========================================================================

    DashboardLayout {
        id: _layout
        anchors.fill: parent

        activeRoute: _shell.activeRoute
        cartCount: CartService.itemCount
        unreadCount: NotificationService.unreadCount
        userName: UserService.displayName
        userInitials: UserService.initials
        pageTitle: _shell._routeMeta[_shell.activeRoute] ? _shell._routeMeta[_shell.activeRoute].title : ""
        pageSubtitle: _shell._routeMeta[_shell.activeRoute] ? _shell._routeMeta[_shell.activeRoute].subtitle : ""

        onRouteRequested: function(route) {
            _shell._navigateTo(route)
        }
        onLogoutRequested: _shell.logoutRequested()
        onSearchRequested: function(query) {
            _searchVM.query = query
            _shell._navigateTo("search")
        }
        onNotificationsRequested: _shell._navigateTo("notifications")
        onCartRequested: _shell._navigateTo("cart")
        onProfileRequested: _shell._navigateTo("profile")
        onThemeToggled: _shell.themeToggled()

        // ----- Page Loader (child of DashboardLayout → child of content Item) -----
        Loader {
            id: _pageLoader
            anchors.fill: parent
            sourceComponent: _shell._componentForRoute(_shell.activeRoute)
            opacity: _readerOverlay.visible ? 0.0 : 1.0
            Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }
        }
    }

    // Component map (kept inline for simplicity; could be split into files
    // for larger codebases).
    readonly property var _componentMap: ({
        "home":          _homeComp,
        "search":        _searchComp,
        "bookDetail":    _bookDetailComp,
        "cart":          _cartComp,
        "library":       _libraryComp,
        "shelves":       _shelvesComp,
        "groupReading":  _groupReadingComp,
        "wishlist":      _wishlistComp,
        "notifications": _notificationsComp,
        "profile":       _profileComp,
        "settings":      _settingsComp
    })

    function _componentForRoute(route) {
        return _componentMap[route] || _homeComp
    }

    function _navigateTo(route) {
        // Don't push bookDetail or reader as primary routes — they're overlays.
        if (route === "bookDetail" || route === "reader") return
        activeRoute = route
    }

    function _openBookDetail(bookId) {
        _bookDetailVM.loadBook(bookId)
        activeRoute = "bookDetail"
    }

    function _openReader(bookId) {
        _readerVM.openBook(bookId)
        _readerOverlay.visible = true
    }

    function _closeReader() {
        _readerVM.close()
        _readerOverlay.visible = false
    }

    function _addToCart(bookId) {
        CartService.add(bookId)
        _shell.toastRequested("success", "Added to cart", "The book is now in your cart.")
    }

    function _buyNow(bookId) {
        CartService.add(bookId)
        _shell._navigateTo("cart")
    }

    // =========================================================================
    //  Page components
    // =========================================================================

    Component {
        id: _homeComp
        HomePage {
            viewModel: _homeVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
            onSeeAllRequested: function(section) {
                _shell._navigateTo("search")
            }
            onSearchWithGenreRequested: function(genre) {
                _searchVM.clearGenres()
                _searchVM.toggleGenre(genre)
                _searchVM.search()
                _shell._navigateTo("search")
            }
            onSearchWithPublisherRequested: function(publisher) {
                _searchVM.query = publisher
                _searchVM.field = "publisher"
                _searchVM.search()
                _shell._navigateTo("search")
            }
            onOpenReaderRequested: function(bookId) { _shell._openReader(bookId) }
            onOpenCartRequested: _shell._navigateTo("cart")
            onOpenWishlistRequested: _shell._navigateTo("wishlist")
            onToastRequested: function(variant, title, description) {
                _shell.toastRequested(variant, title, description)
            }
        }
    }

    Component {
        id: _searchComp
        SearchPage {
            viewModel: _searchVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
        }
    }

    Component {
        id: _bookDetailComp
        BookDetailPage {
            viewModel: _bookDetailVM
            onBackRequested: _shell._navigateTo("home")
            onOpenCartRequested: _shell._navigateTo("cart")
            onOpenReaderRequested: function(bookId) { _shell._openReader(bookId) }
            onCheckoutWithBookRequested: function(bookId) { _shell._buyNow(bookId) }
            onShareRequested: function(title) {
                // Clipboard write via Qt's QGuiApplication. In QML, the
                // clipboard is accessible via Qt.application.clipboard in
                // Qt 5.10+ (but only if the QML environment exposes it).
                // Fallback: just show the toast with the link.
                var link = "https://bookclub.app/books/" + (title || "").replace(/\s+/g, "-").toLowerCase()
                // Try clipboard — works on most Qt5 builds.
                try {
                    if (typeof Qt.application !== "undefined" && Qt.application.clipboard) {
                        Qt.application.clipboard.setText(link)
                    }
                } catch(e) { /* clipboard not available — toast only */ }
                _app.toast("info", "Share", "Link to '" + title + "' copied to clipboard.")
            }
            onToastRequested: function(variant, title, description) {
                _app.toast(variant, title, description)
            }
            Connections {
                target: _bookDetailVM
                ignoreUnknownSignals: true
                onAddedToCart: _shell.toastRequested("success", "Added to cart", "Tap the cart to checkout.")
            }
        }
    }

    Component {
        id: _cartComp
        CartPage {
            viewModel: _cartVM
            onBackRequested: _shell._navigateTo("home")
            onContinueShoppingRequested: _shell._navigateTo("home")
            onCheckoutSuccessRequested: {
                _shell.toastRequested("success", "Purchase complete!", "Your books are now in your library.")
                _shell._navigateTo("library")
            }
        }
    }

    Component {
        id: _libraryComp
        LibraryPage {
            viewModel: _libraryVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
            onOpenReaderRequested: _shell._openReader(bookId)
        }
    }

    Component {
        id: _notificationsComp
        NotificationsPage {
            viewModel: _notificationsVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
        }
    }

    Component {
        id: _profileComp
        ProfilePage {
            viewModel: _profileVM
            bookService: BookService
            darkMode: Theme.isDark
            onLogoutRequested: _shell.logoutRequested()
            onThemeToggled: _shell.themeToggled()
            Connections {
                target: _profileVM
                ignoreUnknownSignals: true
                onProfileSaved: _shell.toastRequested("success", "Profile updated", "Your changes have been saved.")
                onGenresSaved: _shell.toastRequested("success", "Genres updated", "Your home feed will reflect these changes.")
                onPasswordChanged: _shell.toastRequested("success", "Password changed", "Use your new password next time you sign in.")
                onPasswordChangeFailed: function(err) {
                    _shell.toastRequested("error", "Could not change password", err)
                }
            }
        }
    }

    Component {
        id: _wishlistComp
        WishlistPage {
            viewModel: _wishlistVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
            onOpenCartRequested: _shell._navigateTo("cart")
            onContinueShoppingRequested: _shell._navigateTo("search")
        }
    }

    Component {
        id: _settingsComp
        SettingsPage {
            viewModel: _settingsVM
            userService: UserService
            darkMode: Theme.isDark
            onLogoutRequested: _shell.logoutRequested()
            onThemeToggled: _shell.themeToggled()
            onThemeChanged: function(mode) {
                if (mode === "light") Theme.mode = "light"
                else if (mode === "dark") Theme.mode = "dark"
                _shell.toastRequested("info", "Theme preference saved", "Applied: " + mode + " mode.")
            }
            onAccentChanged: function(name) {
                _shell.toastRequested("info", "Accent color", "Saved — restart to apply fully.")
            }
            onToastRequested: function(variant, title, description) {
                _shell.toastRequested(variant, title, description)
            }
            onNavigateToProfileRequested: {
                _shell._navigateTo("profile")
            }
            Connections {
                target: _settingsVM
                ignoreUnknownSignals: true
                onCacheCleared: _shell.toastRequested("success", "Cache cleared", "Freed up disk space.")
            }
        }
    }

    Component {
        id: _shelvesComp
        ShelvesPage {
            viewModel: _shelfVM
            onBookDetailRequested: _shell._openBookDetail(bookId)
            onOpenReaderRequested: function(bookId) { _shell._openReader(bookId) }
            Connections {
                target: _shelfVM
                ignoreUnknownSignals: true
                onShelfCreated: _shell.toastRequested("success", "Shelf created", "Your new shelf is ready.")
                onShelfDeleted: _shell.toastRequested("info", "Shelf deleted", "The shelf has been removed.")
            }
        }
    }

    Component {
        id: _groupReadingComp
        GroupReadingPage {
            viewModel: _studySessionVM
            onOpenReaderRequested: function(bookId) { _shell._openReader(bookId) }
            onToastRequested: function(variant, title, description) {
                _shell.toastRequested(variant, title, description)
            }
        }
    }

    // =========================================================================
    //  PDF Reader overlay (full-screen, on top of the dashboard)
    // =========================================================================
    Item {
        id: _readerOverlay
        anchors.fill: parent
        visible: false
        z: Theme.z.modal

        PdfReaderPage {
            anchors.fill: parent
            viewModel: _readerVM
            onCloseRequested: _shell._closeReader()
        }
    }

    // =========================================================================
    //  Real-time notification toast
    // =========================================================================
    Connections {
        target: _notificationsVM
        ignoreUnknownSignals: true
        onRealtimeNotificationReceived: function(dto) {
            if (dto && dto.title.length > 0) {
                _shell.toastRequested("info", dto.title, dto.body)
            }
        }
    }

    // ----- Initial load -----
    Component.onCompleted: {
        _homeVM.refresh()
    }

    // ----- Real-time pulse -----
    // Every 10 seconds we refresh the Home VM so the continue-reading,
    // recently-viewed, and recommended sections stay fresh. We use a
    // longer interval than the admin/server shells (5s) because the user
    // dashboard changes less frequently and we don't want to distract.
    Timer {
        interval: 10000
        repeat: true
        running: true
        onTriggered: {
            if (_homeVM) _homeVM.refresh()
        }
    }
}
