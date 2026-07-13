// =============================================================================
//  BookDetailPage.qml
// =============================================================================
//  Premium book detail page — two-column layout with:
//      Left (scrollable): hero (cover + info + CTAs) + tabs (Overview /
//      Reviews / Details / Preview) + related/same-author/same-publisher.
//      Right (sticky): action panel (price, buy, cart, wishlist, share,
//      reading progress, stats).
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import "../theme"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/selection"
import "../components/book"
import "../components/data"
import "../components/navigation"
import "../components/feedback"
import "../components/progress"
import "../components/effects"

Item {
    id: root
    anchors.fill: parent

    property var viewModel: null   // BookDetailViewModel

    // Inline-reply state for the reviews list. When the user clicks "Reply"
    // on a review, _replyingTo is set to that review's id and an inline
    // InputField appears beneath it. Setting _replyingTo to "" cancels.
    property string _replyingTo: ""
    property string _replyText: ""

    signal backRequested()
    signal openCartRequested()
    signal openReaderRequested(string bookId)
    signal checkoutWithBookRequested(string bookId)
    signal bookDetailRequested(string bookId)
    signal shareRequested(string bookTitle)
    signal toastRequested(string variant, string title, string description)

    readonly property int _horizontalPadding: Theme.space.xxxl
    readonly property var _book: root.viewModel ? root.viewModel.book : null
    readonly property bool _isBusy: root.viewModel && root.viewModel.isBusy

    Rectangle { anchors.fill: parent; color: Theme.color.pageBackground }

    LoadingOverlay {
        anchors.fill: parent
        visible: root._isBusy && (!root._book || root._book.id.length === 0)
    }

    // Breadcrumb + back
    Item {
        id: _topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 48

        Row {
            anchors.left: parent.left
            anchors.leftMargin: root._horizontalPadding
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.space.sm

            IconButton {
                iconName: "arrow_back"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                onClicked: root.backRequested()
            }
            Breadcrumb {
                segments: ["Home", root._book ? root._book.title : "Book details"]
                anchors.verticalCenter: parent.verticalCenter
                onSegmentClicked: function(index) {
                    if (index === 0) root.backRequested()
                }
            }
        }
    }

    // Two-column body
    Item {
        id: _body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: _topBar.bottom
        anchors.bottom: parent.bottom

        visible: root._book && root._book.id.length > 0

        Row {
            anchors.fill: parent
            anchors.leftMargin: root._horizontalPadding
            anchors.rightMargin: root._horizontalPadding
            spacing: Theme.space.xl

            // ----- Left: scrollable content -----
            Flickable {
                width: parent.width - 340 - Theme.space.xl
                height: parent.height
                contentWidth: width
                contentHeight: _leftCol.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Column {
                    id: _leftCol
                    width: parent.width
                    spacing: Theme.space.xxl

                    // ----- Hero (cover + info + CTAs) -----
                    Item {
                        width: parent.width
                        height: _heroRow.height

                        Row {
                            id: _heroRow
                            spacing: Theme.space.xxl

                            // Large cover
                            Item {
                                id: _coverWrap
                                width: root.width < 760 ? 160 : 220
                                height: width * Theme.size.bookCoverRatio

                                BookCover {
                                    anchors.fill: parent
                                    book: root._book
                                    cornerRadius: Theme.radius.lg
                                }
                                layer.enabled: true
                                layer.effect: DropShadowBase { colorSpec: Theme.shadow.lg }
                            }

                            Column {
                                width: parent.width - _coverWrap.width - Theme.space.xxl
                                spacing: Theme.space.md
                                anchors.top: parent.top

                                Row {
                                    spacing: Theme.space.xs
                                    Repeater {
                                        model: root._book ? root._book.genreIds : []
                                        delegate: Rectangle {
                                            width: _gText.implicitWidth + 16
                                            height: 24
                                            radius: Theme.radius.pill
                                            color: Theme.color.fieldFilled
                                            Text {
                                                id: _gText
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                font.weight: Theme.font.weightMedium
                                            }
                                        }
                                    }
                                }

                                Text {
                                    text: root._book ? root._book.title : ""
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeDisplay
                                    font.weight: Theme.font.weightBold
                                    width: parent.width
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.15
                                }

                                Text {
                                    width: parent.width
                                    text: root._book ? ("by " + root._book.authorName + "  ·  " + root._book.publisherName) : ""
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBodyLarge
                                    elide: Text.ElideRight
                                }

                                Row {
                                    spacing: Theme.space.md
                                    RatingStars {
                                        rating: root._book ? root._book.averageRating : 0
                                        count: root._book ? root._book.ratingCount : 0
                                        showNumber: true
                                        size: 18
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Rectangle {
                                        visible: root._book && root._book.totalSales > 0
                                        width: _salesText.implicitWidth + 16; height: 24; radius: 12
                                        color: Theme.color.fieldFilled
                                        Text {
                                            id: _salesText
                                            anchors.centerIn: parent
                                            text: (root._book ? root._book.totalSales : 0) + " sold"
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightMedium
                                        }
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                // Expandable description
                                Column {
                                    width: parent.width
                                    spacing: Theme.space.xs

                                    Text {
                                        id: _descText
                                        text: root._book ? root._book.description : ""
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        wrapMode: Text.WordWrap
                                        width: parent.width
                                        lineHeight: 1.5
                                        maximumLineCount: _descExpanded ? 9999 : 3
                                        elide: _descExpanded ? Text.ElideNone : Text.ElideRight
                                        property bool _descExpanded: false
                                        Behavior on maximumLineCount { NumberAnimation { duration: Theme.motion.durationBase } }
                                    }
                                    TextButton {
                                        text: _descText._descExpanded ? "Show less" : "Read more"
                                        onClicked: _descText._descExpanded = !_descText._descExpanded
                                    }
                                }

                                // CTAs (compact, also on sticky panel)
                                Row {
                                    spacing: Theme.space.md

                                    PrimaryButton {
                                        text: root._book && root._book.purchased ? "Open in reader" : "Buy now"
                                        iconName: root._book && root._book.purchased ? "menu_book" : "shopping_bag"
                                        iconPosition: "leading"
                                        enabled: !root._isBusy
                                        loading: root._isBusy
                                        onClicked: {
                                            if (root._book && root._book.purchased) root.openReaderRequested(root._book.id)
                                            else root.checkoutWithBookRequested(root._book.id)
                                        }
                                    }
                                    SecondaryButton {
                                        text: root.viewModel && root.viewModel.inCart ? "In cart" : "Add to cart"
                                        iconName: "add_shopping_cart"
                                        iconPosition: "leading"
                                        enabled: !(root.viewModel && root.viewModel.inCart) && !(root._book && root._book.purchased)
                                        onClicked: if (root.viewModel) root.viewModel.addToCart()
                                    }
                                    IconButton {
                                        iconName: root.viewModel && root.viewModel.inWishlist ? "favorite" : "favorite_border"
                                        iconColor: root.viewModel && root.viewModel.inWishlist ? Theme.color.error : Theme.color.textSecondary
                                        hoverIconColor: Theme.color.error
                                        width: 48; height: 48
                                        onClicked: if (root.viewModel) root.viewModel.toggleWishlist()
                                    }
                                }
                            }
                        }
                    }

                    Divider { width: parent.width; orientation: "horizontal" }

                    // ----- Tabs -----
                    TabBar {
                        id: _tabs
                        width: parent.width
                        height: 44
                        tabs: ["Overview", "Reviews (" + (root.viewModel ? root.viewModel.reviewCount : 0) + ")", "Details", "Preview"]
                        activeIndex: 0
                        onTabSelected: _stack.currentIndex = index
                    }

                    // ----- Tab content -----
                    StackLayout {
                        id: _stack
                        width: parent.width
                        currentIndex: 0

                        // Overview tab
                        Column {
                            width: parent.width
                            spacing: Theme.space.xl

                            // Reading progress
                            Card {
                                width: parent.width
                                bordered: true
                                padding: Theme.space.lg
                                visible: root.viewModel && root.viewModel.hasReadingProgress

                                Column {
                                    width: parent.width
                                    spacing: Theme.space.sm

                                    Row {
                                        width: parent.width
                                        Text {
                                            text: "Continue reading"
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightSemibold
                                        }
                                        Item { Layout.fillWidth: true; width: 1; height: 1 }
                                        Text {
                                            text: (root.viewModel ? root.viewModel.readingPage : 0) + " / " + (root.viewModel ? root.viewModel.readingPageCount : 0)
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.familyMono
                                            font.pixelSize: Theme.font.sizeCaption
                                        }
                                    }

                                    ProgressBar {
                                        width: parent.width
                                        height: 6
                                        value: root.viewModel ? root.viewModel.readingProgress : 0
                                    }

                                    PrimaryButton {
                                        text: "Continue"
                                        iconName: "menu_book"
                                        iconPosition: "leading"
                                        onClicked: if (root._book) root.openReaderRequested(root._book.id)
                                    }
                                }
                            }

                            // About the book
                            Text {
                                text: "About this book"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeHeadline
                                font.weight: Theme.font.weightBold
                            }
                            Text {
                                text: root._book ? root._book.description : ""
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeBody
                                wrapMode: Text.WordWrap
                                width: parent.width
                                lineHeight: 1.65
                            }

                            // Details grid
                            Card {
                                width: parent.width
                                bordered: true
                                padding: Theme.space.xl

                                Grid {
                                    width: parent.width
                                    columns: 2
                                    spacing: Theme.space.lg

                                        // Each detail row
                                        Repeater {
                                            model: [
                                                { label: "Author", value: root._book ? root._book.authorName : "" },
                                                { label: "Publisher", value: root._book ? root._book.publisherName : "" },
                                                { label: "Genres", value: root._book ? root._book.genreIds.join(", ") : "" },
                                                { label: "Released", value: root._book ? root._book.createdAtText : "" },
                                                { label: "Language", value: "English" },
                                                { label: "Pages", value: "320" },
                                                { label: "Format", value: "PDF" },
                                                { label: "Sales", value: (root._book ? root._book.totalSales : 0) + " copies" }
                                            ]
                                            delegate: Column {
                                                width: (parent.width - parent.spacing) / 2
                                                spacing: 2
                                                Text {
                                                    text: modelData.label
                                                    color: Theme.color.textMuted
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeCaption
                                                }
                                                Text {
                                                    text: modelData.value
                                                    color: Theme.color.textPrimary
                                                    font.family: Theme.font.family
                                                    font.pixelSize: Theme.font.sizeBody
                                                    font.weight: Theme.font.weightMedium
                                                    wrapMode: Text.WordWrap
                                                    width: parent.width
                                                }
                                            }
                                        }
                                }
                            }

                            // Related books
                            SectionCarousel {
                                width: parent.width
                                title: "Customers also bought"
                                subtitle: "Books related to this one"
                                books: root.viewModel ? root.viewModel.relatedBooks : []
                                showSeeAll: false
                                onBookClicked: root.bookDetailRequested(book.id)
                            }

                            SectionCarousel {
                                width: parent.width
                                title: "More from " + (root._book ? root._book.authorName : "")
                                books: root.viewModel ? root.viewModel.sameAuthor : []
                                showSeeAll: false
                                onBookClicked: root.bookDetailRequested(book.id)
                            }

                            SectionCarousel {
                                width: parent.width
                                title: "From " + (root._book ? root._book.publisherName : "")
                                books: root.viewModel ? root.viewModel.samePublisher : []
                                showSeeAll: false
                                onBookClicked: root.bookDetailRequested(book.id)
                            }
                        }

                        // Reviews tab
                        Column {
                            width: parent.width
                            spacing: Theme.space.xl

                            // Rating summary + distribution
                            Card {
                                width: parent.width
                                bordered: true
                                padding: Theme.space.xl

                                Row {
                                    width: parent.width
                                    spacing: Theme.space.xxl

                                        // Big score
                                        Column {
                                            width: 160
                                            spacing: Theme.space.xs
                                            anchors.verticalCenter: parent.verticalCenter

                                            Text {
                                                text: root._book ? root._book.averageRating.toFixed(1) : "0.0"
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.sizeMega
                                                font.weight: Theme.font.weightBold
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            RatingStars {
                                                rating: root._book ? root._book.averageRating : 0
                                                size: 22
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: (root.viewModel ? root.viewModel.totalRatings : 0) + " ratings"
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }

                                        // Distribution
                                        RatingDistribution {
                                            width: parent.width - 160 - Theme.space.xxl
                                            distribution: root.viewModel ? root.viewModel.ratingDistribution : []
                                            totalRatings: root.viewModel ? root.viewModel.totalRatings : 0
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }

                                // Write-a-review form
                                Card {
                                    width: parent.width
                                    bordered: true
                                    padding: Theme.space.xl

                                    Column {
                                        width: parent.width
                                        spacing: Theme.space.md

                                        Text {
                                            text: root.viewModel && root.viewModel.myReviewId.length > 0 ? "Edit your review" : "Write a review"
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeTitle
                                            font.weight: Theme.font.weightSemibold
                                        }

                                        Row {
                                            spacing: Theme.space.sm
                                            Text {
                                                text: "Your rating:"
                                                color: Theme.color.textSecondary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                anchors.verticalCenter: parent.verticalCenter
                                            }
                                            StarInput {
                                                value: root.viewModel ? root.viewModel.draftRating : 0
                                                size: 28
                                                onValueChanged: if (root.viewModel) root.viewModel.draftRating = value
                                            }
                                        }

                                        InputField {
                                            width: parent.width
                                            label: "Your review"
                                            placeholder: "Share your thoughts about this book..."
                                            text: root.viewModel ? root.viewModel.draftComment : ""
                                            maximumLength: 1000
                                            onTextEdited: if (root.viewModel) root.viewModel.draftComment = newText
                                        }

                                        Row {
                                            spacing: Theme.space.md
                                            PrimaryButton {
                                                text: root.viewModel && root.viewModel.myReviewId.length > 0 ? "Update review" : "Post review"
                                                iconName: "rate_review"
                                                iconPosition: "leading"
                                                enabled: root.viewModel && root.viewModel.canSubmitReview && !root._isBusy
                                                loading: root._isBusy
                                                onClicked: if (root.viewModel) root.viewModel.submitReview()
                                            }
                                            SecondaryButton {
                                                text: "Edit existing"
                                                visible: root.viewModel && root.viewModel.myReviewId.length > 0
                                                onClicked: if (root.viewModel) root.viewModel.loadMyReviewIntoDraft()
                                            }
                                            TextButton {
                                                text: "Delete my review"
                                                color: Theme.color.error
                                                hoverColor: Theme.color.error
                                                visible: root.viewModel && root.viewModel.myReviewId.length > 0
                                                onClicked: if (root.viewModel) root.viewModel.deleteMyReview()
                                            }
                                        }
                                    }
                                }

                                // Existing reviews
                                Column {
                                    width: parent.width
                                    spacing: Theme.space.md

                                    Repeater {
                                        model: root.viewModel ? root.viewModel.reviews : []
                                        delegate: Card {
                                            width: parent.width
                                            bordered: true
                                            padding: Theme.space.lg

                                            Column {
                                                width: parent.width
                                                spacing: Theme.space.sm

                                                Row {
                                                    width: parent.width
                                                    spacing: Theme.space.md

                                                        Rectangle {
                                                            width: 40; height: 40; radius: 20
                                                            color: Theme.color.primary
                                                            Text {
                                                                anchors.centerIn: parent
                                                                text: modelData.initial
                                                                color: Theme.color.onPrimary
                                                                font.family: Theme.font.family
                                                                font.pixelSize: Theme.font.sizeBody
                                                                font.weight: Theme.font.weightBold
                                                            }
                                                        }

                                                        Column {
                                                            width: parent.width - 40 - Theme.space.md - _revTime.implicitWidth - Theme.space.md
                                                            spacing: 2
                                                            Row {
                                                                spacing: Theme.space.sm
                                                                Text {
                                                                    text: modelData.displayName
                                                                    color: Theme.color.textPrimary
                                                                    font.family: Theme.font.family
                                                                    font.pixelSize: Theme.font.sizeBody
                                                                    font.weight: Theme.font.weightSemibold
                                                                }
                                                                Rectangle {
                                                                    visible: modelData.byCurrentUser
                                                                    width: _youText.implicitWidth + 12; height: 18; radius: 9
                                                                    color: Theme.color.accentSoft
                                                                    Text {
                                                                        id: _youText
                                                                        anchors.centerIn: parent
                                                                        text: "You"
                                                                        color: Theme.color.accent
                                                                        font.family: Theme.font.family
                                                                        font.pixelSize: Theme.font.sizeMicro2
                                                                        font.weight: Theme.font.weightBold
                                                                    }
                                                                    anchors.verticalCenter: parent.verticalCenter
                                                                }
                                                                // Verified purchase badge — only shown for reviews by
                                                                // users who actually purchased the book.
                                                                Rectangle {
                                                                    visible: modelData.verifiedPurchase === true
                                                                    width: visible ? _verifiedText.implicitWidth + 16 : 0
                                                                    height: 20
                                                                    radius: 10
                                                                    color: Theme.color.successSoft
                                                                    Text {
                                                                        id: _verifiedText
                                                                        anchors.centerIn: parent
                                                                        text: "✓ Verified purchase"
                                                                        color: Theme.color.success
                                                                        font.family: Theme.font.family
                                                                        font.pixelSize: Theme.font.sizeMicro2
                                                                        font.weight: Theme.font.weightBold
                                                                    }
                                                                    anchors.verticalCenter: parent.verticalCenter
                                                                }
                                                            }
                                                            RatingStars { rating: modelData.rating; size: 14 }
                                                        }

                                                        Text {
                                                            id: _revTime
                                                            text: modelData.relativeTime
                                                            color: Theme.color.textMuted
                                                            font.family: Theme.font.family
                                                            font.pixelSize: Theme.font.sizeCaption
                                                            anchors.verticalCenter: parent.verticalCenter
                                                        }
                                                    }

                                                    Text {
                                                        text: modelData.comment
                                                        color: Theme.color.textSecondary
                                                        font.family: Theme.font.family
                                                        font.pixelSize: Theme.font.sizeBody
                                                        wrapMode: Text.WordWrap
                                                        width: parent.width
                                                        lineHeight: 1.5
                                                    }

                                                    Row {
                                                        spacing: Theme.space.lg
                                                        // Helpful button — toggles helpful mark, updates count.
                                                        TextButton {
                                                            text: (modelData.currentUserHelpful === true ? "👍 " : "♡ ") + modelData.helpfulCount + " helpful"
                                                            color: modelData.currentUserHelpful === true ? Theme.color.accent : Theme.color.textMuted
                                                            font.pixelSize: Theme.font.sizeCaption
                                                            onClicked: {
                                                                if (root.viewModel) {
                                                                    if (modelData.currentUserHelpful === true) {
                                                                        root.viewModel.markNotHelpful(modelData.id)
                                                                    } else {
                                                                        root.viewModel.markHelpful(modelData.id)
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        // Reply button — opens an inline reply input.
                                                        TextButton {
                                                            text: "Reply"
                                                            font.pixelSize: Theme.font.sizeCaption
                                                            onClicked: {
                                                                // Toggle the reply input row for this review.
                                                                root._replyingTo = (root._replyingTo === modelData.id) ? "" : modelData.id
                                                                root._replyText = ""
                                                            }
                                                        }
                                                        // Report button — flags the review for moderation.
                                                        TextButton {
                                                            text: "Report"
                                                            color: Theme.color.textMuted
                                                            font.pixelSize: Theme.font.sizeCaption
                                                            onClicked: {
                                                                if (root.viewModel) {
                                                                    root.viewModel.reportReview(modelData.id)
                                                                    root.toastRequested("info", "Reported",
                                                                                         "Thanks — the review has been flagged for moderation.")
                                                                }
                                                            }
                                                        }
                                                    }

                                                    // Inline reply input (visible only when this review is being replied to).
                                                    Column {
                                                        width: parent.width
                                                        visible: root._replyingTo === modelData.id
                                                        spacing: Theme.space.xs

                                                        Row {
                                                            width: parent.width
                                                            spacing: Theme.space.sm
                                                            InputField {
                                                                id: _replyField
                                                                width: parent.width - 100 - Theme.space.sm
                                                                placeholder: "Write a reply…"
                                                                text: root._replyText
                                                                onTextEdited: root._replyText = newText
                                                                onAccepted: {
                                                                    if (root.viewModel && root._replyText.trim().length > 0) {
                                                                        root.viewModel.addReply(modelData.id, root._replyText.trim())
                                                                        root._replyingTo = ""
                                                                        root._replyText = ""
                                                                    }
                                                                }
                                                            }
                                                            PrimaryButton {
                                                                width: 100
                                                                text: "Post"
                                                                iconName: "send"
                                                                enabled: root._replyText.trim().length > 0
                                                                onClicked: {
                                                                    if (root.viewModel && root._replyText.trim().length > 0) {
                                                                        root.viewModel.addReply(modelData.id, root._replyText.trim())
                                                                        root._replyingTo = ""
                                                                        root._replyText = ""
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    EmptyIllustration {
                                        width: parent.width
                                        height: 200
                                        visible: root.viewModel && root.viewModel.reviewCount === 0
                                        iconName: "rate_review"
                                        title: "No reviews yet"
                                        description: "Be the first to share your thoughts."
                                    }
                                }
                            }

                            // Details tab
                            Column {
                                width: parent.width
                                spacing: Theme.space.xl

                                Text {
                                    text: "Book details"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeHeadline
                                    font.weight: Theme.font.weightBold
                                }

                                Card {
                                    width: parent.width
                                    bordered: true
                                    padding: Theme.space.xl

                                    Grid {
                                        width: parent.width
                                        columns: 2
                                        spacing: Theme.space.lg

                                            Repeater {
                                                model: [
                                                    { label: "Title", value: root._book ? root._book.title : "" },
                                                    { label: "Author", value: root._book ? root._book.authorName : "" },
                                                    { label: "Publisher", value: root._book ? root._book.publisherName : "" },
                                                    { label: "Genres", value: root._book ? root._book.genreIds.join(", ") : "" },
                                                    { label: "Released", value: root._book ? root._book.createdAtText : "" },
                                                    { label: "Age", value: root._book ? root._book.ageText : "" },
                                                    { label: "Language", value: "English" },
                                                    { label: "Pages", value: (root._book ? Math.max(8, Math.floor((root._book.description || "").length / 30)) : 0).toString() },
                                                    { label: "Format", value: "PDF" },
                                                    { label: "Sales", value: (root._book ? root._book.totalSales : 0) + " copies" },
                                                    { label: "Ratings", value: (root._book ? root._book.ratingCount : 0) + " total" },
                                                    { label: "Availability", value: root._book ? (root._book.isFree ? "Free download" : "In stock") : "—" }
                                                ]
                                                delegate: Column {
                                                    width: (parent.width - parent.spacing) / 2
                                                    spacing: 2
                                                    Text {
                                                        text: modelData.label
                                                        color: Theme.color.textMuted
                                                        font.family: Theme.font.family
                                                        font.pixelSize: Theme.font.sizeCaption
                                                    }
                                                    Text {
                                                        text: modelData.value
                                                        color: Theme.color.textPrimary
                                                        font.family: Theme.font.family
                                                        font.pixelSize: Theme.font.sizeBody
                                                        font.weight: Theme.font.weightMedium
                                                        wrapMode: Text.WordWrap
                                                        width: parent.width
                                                    }
                                                }
                                            }
                                    }
                                }
                            }

                            // Preview tab
                            Column {
                                width: parent.width
                                spacing: Theme.space.xl

                                Text {
                                    text: "Read a preview"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeHeadline
                                    font.weight: Theme.font.weightBold
                                }

                                Card {
                                    width: parent.width
                                    elevation: "md"
                                    padding: Theme.space.xxl

                                    Column {
                                        width: parent.width
                                        spacing: Theme.space.md

                                        Text {
                                            text: "Chapter 1"
                                            color: Theme.color.textMuted
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                        }
                                        Text {
                                            text: root._book ? root._book.title : ""
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeTitle
                                            font.weight: Theme.font.weightBold
                                        }
                                        Text {
                                            text: root._book ? root._book.description : ""
                                            color: Theme.color.textSecondary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBodyLarge
                                            wrapMode: Text.WordWrap
                                            width: parent.width
                                            lineHeight: 1.7
                                        }

                                        PrimaryButton {
                                            text: "Open in reader"
                                            iconName: "menu_book"
                                            iconPosition: "leading"
                                            onClicked: if (root._book) root.openReaderRequested(root._book.id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ----- Right: sticky action panel -----
                StickyPanel {
                    width: 340
                    height: parent.height
                    title: "Buy this book"

                    Column {
                        width: parent.width
                        spacing: Theme.space.lg

                        // Price
                        Row {
                            width: parent.width
                            spacing: Theme.space.md

                                Text {
                                    text: root._book ? root._book.priceText : ""
                                    color: Theme.color.primary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeDisplay
                                    font.weight: Theme.font.weightBold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    visible: root._book && root._book.hasDiscount
                                    text: root._book ? root._book.basePriceText : ""
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBodyLarge
                                    font.strikeout: true
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Rectangle {
                                    visible: root._book && root._book.hasDiscount
                                    width: _discText.implicitWidth + 16; height: 26; radius: 13
                                    color: Theme.color.errorSoft
                                    Text {
                                        id: _discText
                                        anchors.centerIn: parent
                                        text: (root._book ? root._book.discountPercent : 0) + "% off"
                                        color: Theme.color.error
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }

                            // Availability
                            Row {
                                spacing: Theme.space.sm
                                AppIcon { name: "check_circle"; size: 16; color: Theme.color.success; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: "In stock · Instant download"
                                    color: Theme.color.success
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }
                            }

                            // Primary CTAs (sticky duplicates)
                            PrimaryButton {
                                width: parent.width
                                text: root._book && root._book.purchased ? "Open in reader" : "Buy now"
                                iconName: root._book && root._book.purchased ? "menu_book" : "shopping_bag"
                                iconPosition: "trailing"
                                enabled: !root._isBusy
                                loading: root._isBusy
                                onClicked: {
                                    if (root._book && root._book.purchased) root.openReaderRequested(root._book.id)
                                    else root.checkoutWithBookRequested(root._book.id)
                                }
                            }

                            SecondaryButton {
                                width: parent.width
                                text: root.viewModel && root.viewModel.inCart ? "Already in cart" : "Add to cart"
                                iconName: "add_shopping_cart"
                                iconPosition: "leading"
                                enabled: !(root.viewModel && root.viewModel.inCart) && !(root._book && root._book.purchased)
                                onClicked: if (root.viewModel) root.viewModel.addToCart()
                            }

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm

                                    Rectangle {
                                        width: (parent.width - Theme.space.sm) / 2
                                        height: 44
                                        radius: Theme.radius.md
                                        color: Theme.color.cardBackground
                                        border.color: Theme.color.border
                                        border.width: 1

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 6
                                            AppIcon {
                                                name: root.viewModel && root.viewModel.inWishlist ? "favorite" : "favorite_border"
                                                size: 18
                                                color: root.viewModel && root.viewModel.inWishlist ? Theme.color.error : Theme.color.textSecondary
                                            }
                                            Text {
                                                text: root.viewModel && root.viewModel.inWishlist ? "Saved" : "Wishlist"
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                font.weight: Theme.font.weightMedium
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: if (root.viewModel) root.viewModel.toggleWishlist()
                                        }
                                    }

                                    Rectangle {
                                        width: (parent.width - Theme.space.sm) / 2
                                        height: 44
                                        radius: Theme.radius.md
                                        color: Theme.color.cardBackground
                                        border.color: Theme.color.border
                                        border.width: 1

                                        Row {
                                            anchors.centerIn: parent
                                            spacing: 6
                                            AppIcon { name: "share"; size: 18; color: Theme.color.textSecondary }
                                            Text {
                                                text: "Share"
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeBody
                                                font.weight: Theme.font.weightMedium
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                // Share — in a real app this would open a share sheet.
                                                // For now, copy the book title to the clipboard via toast.
                                                if (root._book) {
                                                    root.shareRequested(root._book.title)
                                                }
                                            }
                                        }
                                    }
                                }

                                Divider { width: parent.width; orientation: "horizontal" }

                                // Reading progress (if purchased)
                                Column {
                                    width: parent.width
                                    spacing: Theme.space.xs
                                    visible: root.viewModel && root.viewModel.hasReadingProgress

                                    Row {
                                        width: parent.width
                                        Text {
                                            text: "Reading progress"
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightSemibold
                                        }
                                        Item { Layout.fillWidth: true; width: 1; height: 1 }
                                        Text {
                                            text: Math.abs(root.viewModel ? root.viewModel.readingProgress * 100 : 0).toFixed(0) + "%"
                                            color: Theme.color.accent
                                            font.family: Theme.font.familyMono
                                            font.pixelSize: Theme.font.sizeCaption
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                    ProgressBar {
                                        width: parent.width
                                        height: 6
                                        value: root.viewModel ? root.viewModel.readingProgress : 0
                                    }
                                }

                                // Stats
                                Row {
                                    width: parent.width
                                    spacing: Theme.space.lg

                                        Column {
                                            width: (parent.width - Theme.space.lg) / 2
                                            spacing: 0
                                            Text {
                                                text: root._book ? root._book.ratingCount : 0
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeTitle
                                                font.weight: Theme.font.weightBold
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: "Ratings"
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }
                                        Column {
                                            width: (parent.width - Theme.space.lg) / 2
                                            spacing: 0
                                            Text {
                                                text: root._book ? root._book.totalSales : 0
                                                color: Theme.color.textPrimary
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeTitle
                                                font.weight: Theme.font.weightBold
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Text {
                                                text: "Sold"
                                                color: Theme.color.textMuted
                                                font.family: Theme.font.family
                                                font.pixelSize: Theme.font.sizeCaption
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
