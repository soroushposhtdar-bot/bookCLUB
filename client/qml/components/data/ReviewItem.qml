// =============================================================================
//  ReviewItem.qml
// =============================================================================
//  Reusable single-review card with the full action surface:
//      • Avatar (initial), display name, role badges (You / Verified purchase /
//        Author / Publisher / Pinned), star rating, relative time.
//      • Comment text — clamped to 3 lines by default with a "Read more" toggle
//        that expands the full comment inline (animated height).
//      • Action row — Helpful (count, highlighted if currentUserHelpful),
//        Not-helpful (count, highlighted if currentUserNotHelpful), Reply,
//        and a "More" overflow button that opens a ContextMenu with
//        Report / Pin / Edit / Delete.
//      • Hover state — subtle accent-soft tint over the entire card.
//      • Right-click anywhere on the card surfaces the same ContextMenu.
//      • Animated expand/collapse of the replies section — a small chip
//        "N replies" toggles the placeholder area (the actual reply list is
//        populated by the parent via the `replyClicked` signal).
//
//  Public API:
//      review : ReviewDto* (id, displayName, initial, rating, comment,
//                            helpfulCount, notHelpfulCount,
//                            currentUserHelpful, currentUserNotHelpful,
//                            byCurrentUser, verifiedPurchase, byAuthor,
//                            byPublisher, pinned, flagged, relativeTime,
//                            replyCount)
//      highlighted : bool — render with accent left border (e.g. pinned review)
//      compact     : bool — tighter padding for nested reply contexts
//
//  Signals:
//      helpfulClicked(string id)
//      notHelpfulClicked(string id)
//      replyClicked(string id)
//      reportClicked(string id)
//      pinClicked(string id)
//      editClicked(string id)
//      deleteClicked(string id)
// =============================================================================
import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"
import "../"
import "../buttons"
import "../book"
import "../feedback"
import "../effects"
import "../surfaces"

Item {
    id: root

    property var review: null
    property bool highlighted: false
    property bool compact: false

    signal helpfulClicked(string id)
    signal notHelpfulClicked(string id)
    signal replyClicked(string id)
    signal reportClicked(string id)
    signal pinClicked(string id)
    signal editClicked(string id)
    signal deleteClicked(string id)

    implicitWidth: parent ? parent.width : 600
    implicitHeight: _bg.height

    // ----- Helpers -----
    readonly property bool _byCurrentUser: root.review && root.review.byCurrentUser
    readonly property bool _canPin: root.review && !(root.review.byAuthor || root.review.byPublisher)
    readonly property int _commentLineLimit: 3
    readonly property string _reviewId: root.review ? root.review.id : ""

    // -------------------------------------------------------------------------
    //  Card surface
    // -------------------------------------------------------------------------
    Rectangle {
        id: _bg
        anchors.left: parent.left
        anchors.right: parent.right
        height: _col.implicitHeight + 2 * (root.compact ? Theme.space.md : Theme.space.lg)
        radius: Theme.radius.lg

        // Base color — subtle tint on hover, even softer when highlighted.
        color: _hoverHandler.hovered ? Theme.color.accentSoft : Theme.color.cardBackground
        border.color: root.highlighted ? Theme.color.accent
                 : _hoverHandler.hovered ? Theme.color.borderStrong
                 : Theme.color.border
        border.width: 1

        Behavior on color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: Theme.motion.durationFast; easing.type: Easing.OutCubic } }

        // Pinned accent stripe down the left edge.
        Rectangle {
            visible: root.highlighted
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 0
            width: 3
            color: Theme.color.accent
            radius: 0
        }

        // Hover lift shadow.
        layer.enabled: _hoverHandler.hovered
        layer.effect: DropShadowBase { colorSpec: Theme.shadow.sm }

        HoverHandler {
            id: _hoverHandler
            cursorShape: Qt.ArrowCursor
        }

        // Right-click anywhere → context menu.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: {
                if (mouse.button === Qt.RightButton) {
                    _ctxMenu.x = mouseX
                    _ctxMenu.y = mouseY
                    _ctxMenu.open()
                }
            }
            z: -1
        }
    }

    // -------------------------------------------------------------------------
    //  Content column
    // -------------------------------------------------------------------------
    Column {
        id: _col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: root.compact ? Theme.space.md : Theme.space.lg
        anchors.rightMargin: root.compact ? Theme.space.md : Theme.space.lg
        anchors.topMargin: root.compact ? Theme.space.md : Theme.space.lg
        spacing: Theme.space.md

        // ----- Header row: avatar + name + badges + rating + time -----
        Row {
            width: parent.width
            spacing: Theme.space.md

            // Avatar circle (initial)
            Item {
                width: 40
                height: 40
                anchors.verticalCenter: parent.verticalCenter

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: root._byCurrentUser ? Theme.color.accent
                         : (root.review && root.review.byAuthor) ? Theme.color.warning
                         : (root.review && root.review.byPublisher) ? Theme.color.success
                         : Theme.color.primary
                }

                Text {
                    anchors.centerIn: parent
                    text: root.review ? root.review.initial : "?"
                    color: Theme.color.textOnPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBodyLarge
                    font.weight: Theme.font.weightBold
                }
            }

            // Name + badges + time column
            Column {
                width: parent.width - 40 - Theme.space.md
                spacing: 2
                anchors.verticalCenter: parent.verticalCenter

                Row {
                    width: parent.width
                    spacing: Theme.space.sm

                    Text {
                        text: root.review ? root.review.displayName : ""
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeBodyLarge
                        font.weight: Theme.font.weightSemibold
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    // ----- Role / status badges (Repeater over active set) -----
                    Repeater {
                        // Build the active badge list reactively
                        model: {
                            var badges = []
                            if (root._byCurrentUser)
                                badges.push({ key: "you",     label: "You",                icon: "person",    color: Theme.color.accent,  soft: Theme.color.accentSoft })
                            if (root.review && root.review.verifiedPurchase)
                                badges.push({ key: "v",       label: "Verified purchase",  icon: "verified",  color: Theme.color.success, soft: Theme.color.successSoft })
                            if (root.review && root.review.byAuthor)
                                badges.push({ key: "a",       label: "Author",             icon: "edit_note", color: Theme.color.warning, soft: Theme.color.warningSoft })
                            if (root.review && root.review.byPublisher)
                                badges.push({ key: "p",       label: "Publisher",          icon: "campaign", color: Theme.color.success, soft: Theme.color.successSoft })
                            if (root.review && root.review.pinned)
                                badges.push({ key: "pi",      label: "Pinned",             icon: "pin",       color: Theme.color.accent,  soft: Theme.color.accentSoft })
                            return badges
                        }
                        delegate: Item {
                            width: _badgeRow.implicitWidth + 2 * Theme.space.sm
                            height: 22
                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radius.pill
                                color: modelData.soft
                            }
                            Row {
                                id: _badgeRow
                                anchors.centerIn: parent
                                spacing: 4
                                AppIcon {
                                    name: modelData.icon
                                    size: 12
                                    color: modelData.color
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                Text {
                                    text: modelData.label
                                    color: modelData.color
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightSemibold
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }

                    // Filler
                    Item { width: 1; height: 1; Layout.fillWidth: true }

                    // Relative time
                    Text {
                        text: root.review ? root.review.relativeTime : ""
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        font.weight: Theme.font.weightRegular
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Star rating row
                RatingStars {
                    rating: root.review ? root.review.rating : 0
                    showNumber: false
                    size: 14
                }
            }
        }

        // ----- Comment text (expandable) -----
        Text {
            id: _comment
            width: parent.width
            text: root.review ? root.review.comment : ""
            color: Theme.color.textPrimary
            font.family: Theme.font.family
            font.pixelSize: Theme.font.sizeBody
            font.weight: Theme.font.weightRegular
            wrapMode: Text.WordWrap
            lineHeight: Theme.lineHeight.normal
            // Toggle between clamped and full.
            maximumLineCount: _expanded ? 99999 : root._commentLineLimit
            elide: _expanded ? Text.ElideNone : Text.ElideRight

            // "Read more" / "Show less" link sits below when the comment
            // would otherwise be truncated.
            property bool _expanded: false
        }

        // Read more / show less toggle — only shown if the comment actually
        // exceeds the clamped height.
        TextButton {
            text: _comment._expanded ? "Show less" : "Read more"
            iconName: _comment._expanded ? "expand_less" : "read_more"
            visible: _comment.truncated || _comment._expanded
            color: Theme.color.accent
            hoverColor: Theme.color.accentHover
            onClicked: _comment._expanded = !_comment._expanded
        }

        // ----- Action row -----
        Row {
            width: parent.width
            spacing: Theme.space.sm

            // Helpful
            ReviewReactionButton {
                iconName: "thumb_up_outlined"
                label: "Helpful"
                count: root.review ? root.review.helpfulCount : 0
                active: root.review && root.review.currentUserHelpful
                accentColor: Theme.color.accent
                softColor: Theme.color.accentSoft
                onClicked: root.helpfulClicked(root._reviewId)
            }

            // Not helpful
            ReviewReactionButton {
                iconName: "thumb_down_outlined"
                label: ""
                count: root.review ? root.review.notHelpfulCount : 0
                active: root.review && root.review.currentUserNotHelpful
                accentColor: Theme.color.error
                softColor: Theme.color.errorSoft
                onClicked: root.notHelpfulClicked(root._reviewId)
            }

            // Reply
            ReviewReactionButton {
                iconName: "reply"
                label: "Reply"
                count: -1
                active: false
                accentColor: Theme.color.accent
                softColor: Theme.color.accentSoft
                onClicked: root.replyClicked(root._reviewId)
            }

            Item { width: 1; height: 1; Layout.fillWidth: true }

            // Replies chip — toggles the replies section
            ReviewReactionButton {
                id: _repliesChip
                visible: root.review && root.review.replyCount > 0
                iconName: _repliesSection.visible ? "expand_less" : "expand_more"
                label: "%1 %2".arg(root.review ? root.review.replyCount : 0)
                                .arg((root.review && root.review.replyCount === 1) ? "reply" : "replies")
                count: -1
                active: _repliesSection.visible
                accentColor: Theme.color.accent
                softColor: Theme.color.accentSoft
                onClicked: _repliesSection.visible = !_repliesSection.visible
            }

            // Overflow "More" button
            IconButton {
                iconName: "more_vert"
                iconColor: Theme.color.textSecondary
                hoverIconColor: Theme.color.textPrimary
                anchors.verticalCenter: parent.verticalCenter
                onClicked: _ctxMenu.openAt(0, height)
            }
        }

        // ----- Animated replies section -----
        // Visible only when toggled by the replies chip. The parent is
        // responsible for populating this with actual reply items; this
        // area just renders the placeholder container so the collapse
        // animation is smooth.
        Column {
            id: _repliesSection
            width: parent.width
            visible: false
            spacing: Theme.space.sm
            leftPadding: 40 + Theme.space.md   // indent to convey hierarchy
            rightPadding: 0

            // Connector line down the left edge.
            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 20
                width: 2
                height: _repliesSection.implicitHeight
                color: Theme.color.divider
            }

            // Placeholder row — animated when expanded.
            Row {
                width: parent.width - _repliesSection.leftPadding - _repliesSection.rightPadding
                spacing: Theme.space.sm

                Text {
                    text: "Replies are loaded on demand."
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeCaption
                    font.weight: Theme.font.weightRegular
                    anchors.verticalCenter: parent.verticalCenter
                }

                TextButton {
                    text: "Load replies"
                    iconName: "expand_more"
                    onClicked: root.replyClicked(root._reviewId)
                }
            }

            // Animated fade-in when the section becomes visible.
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity {
                NumberAnimation { duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
            }
        }
    }

    // -------------------------------------------------------------------------
    //  Context menu (right-click + overflow button share this)
    // -------------------------------------------------------------------------
    ContextMenu {
        id: _ctxMenu
        parent: root
        actions: root._buildMenuActions()
    }

    // Build the context-menu action list reactively based on the current
    // review state (author / current user / pinned, etc.).
    function _buildMenuActions() {
        var list = []
        // Report — available for any review not by the current user
        if (!root._byCurrentUser) {
            list.push({
                text: "Report review",
                iconName: "flag",
                action: function() { root.reportClicked(root._reviewId) }
            })
        }
        // Pin — only curators (and not the author themselves)
        if (root._canPin) {
            list.push({
                text: root.review && root.review.pinned ? "Unpin review" : "Pin review",
                iconName: "pin",
                action: function() { root.pinClicked(root._reviewId) }
            })
        }
        // Separator before destructive / own-actions
        list.push({ separator: true })
        // Edit / Delete — only for the current user
        if (root._byCurrentUser) {
            list.push({
                text: "Edit review",
                iconName: "edit",
                action: function() { root.editClicked(root._reviewId) }
            })
            list.push({
                text: "Delete review",
                iconName: "delete_outline",
                destructive: true,
                action: function() { root.deleteClicked(root._reviewId) }
            })
        }
        return list
    }
}
