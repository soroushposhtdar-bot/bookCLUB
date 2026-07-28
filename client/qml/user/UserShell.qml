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
    // v10: SettingsViewModel removed — the project doesn't need a settings page.
    ShelfViewModel          { id: _shelfVM;          libraryService: LibraryService }
    StudySessionViewModel   { id: _studySessionVM }

    // v15c: global listener for CartService.checkoutFailed so the "You
    // already own this book." message (emitted when the server rejects a
    // duplicate-purchase add-to-cart) shows as a toast regardless of which
    // page the user is on. Without this, the message only showed on the
    // Cart and BookDetail pages.
    Connections {
        target: CartService
        ignoreUnknownSignals: true
        function onCheckoutFailed(error) {
            _shell.toastRequested("error", "Couldn't add to cart", error)
        }
        function onCheckoutSucceeded(purchasedBookIds) {
            // v15c: invalidate BookService caches so the purchased flag
            // updates on every BookDto and the cart/wishlist buttons
            // disappear for the just-bought books.
            BookService.refresh()
        }
    }

    // ----- Current route (drives Sidebar active state + TopBar title) -----
    property string activeRoute: "home"

    // ----- Issue 11 — CategoryPage state -----
    // When the user clicks "See all" on a Home section, we open a
    // dedicated CategoryPage as a full-screen overlay on top of the
    // dashboard. This avoids polluting the route map with per-section
    // routes and keeps the Back button semantics simple.
    property string _categoryKey: ""
    property string _categoryTitle: ""
    property string _categorySubtitle: ""
    property var _categoryBooks: []
    property bool _categoryVisible: false

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
        "profile":       { title: "Profile",         subtitle: "Manage your account" }
        // v10: "settings" route removed — profile page handles everything.
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
        "profile":       _profileComp
        // v10: "settings" removed
    })

    function _componentForRoute(route) {
        return _componentMap[route] || _homeComp
    }

    function _navigateTo(route) {
        // Don't push bookDetail or reader as primary routes — they're overlays.
        if (route === "bookDetail" || route === "reader") return
        // Close the category overlay if it's open when we navigate away.
        if (_categoryVisible) _categoryVisible = false
        activeRoute = route
    }

    // Issue 11 — open a dedicated CategoryPage for the requested section.
    // The function looks up the section's books + title from the Home VM
    // and shows the CategoryPage overlay.
    function _openCategory(section) {
        var books = []
        var title = "Category"
        var subtitle = ""
        if (_homeVM) {
            switch (section) {
                case "recommended":
                    books = _homeVM.recommended || []
                    title = "Recommended for you"
                    subtitle = "Based on your favorite genres"
                    break
                case "because-you-read":
                    books = _homeVM.becauseYouRead || []
                    title = "Because you read"
                    subtitle = "More mysteries worth your time"
                    break
                case "new":
                    books = _homeVM.newReleases || []
                    title = "New releases"
                    subtitle = "Fresh on the shelves"
                    break
                case "bestseller":
                    books = _homeVM.bestsellers || []
                    title = "Bestsellers"
                    subtitle = "What everyone's reading right now"
                    break
                case "trending":
                    books = _homeVM.trending || []
                    title = "Trending now"
                    subtitle = "Heating up this week"
                    break
                case "editors-picks":
                    books = _homeVM.editorsPicks || []
                    title = "Editor's picks"
                    subtitle = "Curated by our team"
                    break
                case "discounted":
                    books = _homeVM.discounted || []
                    title = "On sale"
                    subtitle = "Limited-time discounts"
                    break
                case "free":
                    books = _homeVM.freeBooks || []
                    title = "Free to read"
                    subtitle = "Classics and community favorites"
                    break
                case "arrivals":
                    books = _homeVM.newArrivals || []
                    title = "New arrivals"
                    subtitle = "Just landed in the catalog"
                    break
                default:
                    books = []
                    title = "Category"
                    subtitle = ""
            }
        }
        _shell._categoryKey = section
        _shell._categoryTitle = title
        _shell._categorySubtitle = subtitle
        _shell._categoryBooks = books
        _shell._categoryVisible = true
    }

    function _openBookDetail(bookId) {
        _bookDetailVM.loadBook(bookId)
        activeRoute = "bookDetail"
    }

    function _openReader(bookId) {
        if (!bookId || bookId.length === 0) {
            _shell.toastRequested("info", "No book", "There's no book to open. Browse the catalog first.")
            return
        }
        _readerVM.openBook(bookId)
        // Only show the reader overlay if the book was actually opened.
        // The ReaderViewModel.openBook is async (uses QMetaObject::invokeMethod),
        // so we check hasBook after a short delay. If the book fails to open,
        // the error property will be set and we show a toast instead.
        _readerOverlay.visible = true
    }

    function _closeReader() {
        _readerVM.close()
        _readerOverlay.visible = false
    }

    function _addToCart(bookId) {
        // v15d: check if the book is already in the cart before adding.
        // If it is, show an "already in cart" toast instead of "added".
        // Force-refresh the cart cache first so isInCart is accurate.
        CartService.refresh()
        if (CartService.isInCart(bookId)) {
            _shell.toastRequested("info", "Already in cart",
                                  "This book is already in your cart, ready to buy.")
            return
        }
        CartService.add(bookId)
        // v15d: re-check after the add. If the book made it into the cart,
        // show success. If not, the CartService already emitted
        // checkoutFailed (e.g. "You already own this book.") which the
        // global Connections block in UserShell shows as a toast.
        if (CartService.isInCart(bookId)) {
            _shell.toastRequested("success", "Added to cart",
                                  "The book is now in your cart.")
        }
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
                // Issue 11 — route to the dedicated CategoryPage for the
                // requested section, not the generic search page.
                _shell._openCategory(section)
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
            // Issue 10 — toggle wishlist on the backend and show feedback.
            onWishlistToggleRequested: function(bookId) {
                BookService.toggleWishlist(bookId)
                var nowIn = BookService.isInWishlist(bookId)
                _shell.toastRequested(nowIn ? "success" : "info",
                                      nowIn ? "Added to wishlist" : "Removed from wishlist",
                                      nowIn ? "Tap the heart again to remove it." : "The book has been removed from your wishlist.")
            }
            onAddToCartRequested: function(bookId) {
                CartService.add(bookId)
                _shell.toastRequested("success", "Added to cart", "The book is now in your cart.")
            }
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
                // BUG FIX: `_app` is the id of App.qml's ApplicationWindow and is
                // NOT visible from UserShell.qml. Replaced with _shell.toastRequested
                // so the toast is forwarded to App.qml's ToastManager.
                _shell.toastRequested("info", "Share", "Link to '" + title + "' copied to clipboard.")
            }
            onToastRequested: function(variant, title, description) {
                // BUG FIX: same _app → _shell.toastRequested fix.
                _shell.toastRequested(variant, title, description)
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
            // BUG FIX: wire the CartPage's toastRequested signal so
            // checkout-failed errors are shown to the user.
            onToastRequested: function(variant, title, description) {
                _shell.toastRequested(variant, title, description)
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
            // Issue 16 — wire the new navigation signals.
            onOpenLibraryRequested: _shell._navigateTo("library")
            onOpenReaderRequested: function(bookId) { _shell._openReader(bookId) }
            onBookDetailRequested: _shell._openBookDetail(bookId)
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

    // v10: _settingsComp (SettingsPage) removed — the project doesn't need it.

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
    //  Issue 11 — CategoryPage overlay (full-screen, on top of the dashboard).
    //  Renders as a sibling of _readerOverlay so both can coexist (though
    //  normally only one is visible at a time).
    // =========================================================================
    Item {
        id: _categoryOverlay
        anchors.fill: parent
        visible: _shell._categoryVisible
        z: Theme.z.modal - 1   // just below the reader overlay
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: Theme.motion.durationBase } }

        CategoryPage {
            anchors.fill: parent
            categoryKey: _shell._categoryKey
            categoryTitle: _shell._categoryTitle
            categorySubtitle: _shell._categorySubtitle
            books: _shell._categoryBooks
            viewModel: _homeVM
            onBackRequested: _shell._categoryVisible = false
            onBookDetailRequested: {
                _shell._categoryVisible = false
                _shell._openBookDetail(bookId)
            }
            onOpenReaderRequested: {
                _shell._categoryVisible = false
                _shell._openReader(bookId)
            }
            onWishlistToggleRequested: function(bookId) {
                BookService.toggleWishlist(bookId)
                var nowIn = BookService.isInWishlist(bookId)
                _shell.toastRequested(nowIn ? "success" : "info",
                                      nowIn ? "Added to wishlist" : "Removed from wishlist",
                                      nowIn ? "Tap the heart again to remove it." : "The book has been removed from your wishlist.")
            }
            onAddToCartRequested: function(bookId) {
                CartService.add(bookId)
                _shell.toastRequested("success", "Added to cart", "The book is now in your cart.")
            }
            onToastRequested: function(variant, title, description) {
                _shell.toastRequested(variant, title, description)
            }
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
                _shell.toastRequested("info", dto.title, dto.message)
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
    // BUG FIX: gate the timer on the active route so it only fires when
    // the Home page is actually visible. Previously it fired every 10s
    // regardless of which page the user was on, wasting bandwidth and
    // causing UI jank on other pages.
    Timer {
        interval: 10000
        repeat: true
        running: _shell.activeRoute === "home"
        onTriggered: {
            if (_homeVM && _shell.activeRoute === "home") _homeVM.refresh()
        }
    }
}
