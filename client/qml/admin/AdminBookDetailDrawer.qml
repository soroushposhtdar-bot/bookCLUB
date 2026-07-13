// =============================================================================
//  AdminBookDetailDrawer.qml
// =============================================================================
//  Slide-in drawer for the admin role showing full details of a single book.
//  Calls viewModel.bookDetails(bookId) to get all book fields + reviews.
//  Admin can delete the book or edit its metadata from this drawer.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"
import "../components/book"

import BookClub.Services 1.0

Item {
    id: drawer

    property var viewModel: null

    property string bookId: ""
    property var _detail: ({})

    signal toastRequested(string variant, string title, string description)
    signal closed()

    visible: false
    width: 480

    function _statusLabel(s) {
        return { "published": "Published", "draft": "Draft", "pending": "Pending review", "removed": "Removed" }[s] || s
    }
    function _statusColor(s) {
        return { "published": Theme.color.success, "draft": Theme.color.textMuted, "pending": Theme.color.warning, "removed": Theme.color.error }[s] || Theme.color.textMuted
    }

    function openForBook(id) {
        drawer.bookId = id
        drawer._reload()
        drawer.visible = true
        _slideIn.from = drawer.width
        _slideIn.start()
    }

    function _reload() {
        if (!drawer.viewModel || drawer.bookId.length === 0) {
            drawer._detail = {}
            return
        }
        const d = drawer.viewModel.bookDetails(drawer.bookId)
        drawer._detail = d || {}
    }

    function close() {
        _slideOut.from = 0
        _slideOut.to = drawer.width
        _slideOut.start()
        _hideTimer.start()
    }

    Timer {
        id: _hideTimer
        // Drive from Theme.motion.durationBase so the hide fires exactly
        // when the slide-out animation finishes (previously hardcoded to
        // 260ms, which could drift if durationBase changed).
        interval: Theme.motion.durationBase
        repeat: false
        onTriggered: {
            drawer.visible = false
            // Emit the closed() signal so parents can react. Previously
            // this signal was declared but never emitted.
            drawer.closed()
        }
    }

    Connections {
        target: drawer.viewModel
        ignoreUnknownSignals: true
        onBooksChanged: drawer._reload()
    }

    NumberAnimation {
        id: _slideIn
        target: _content
        property: "x"
        from: drawer.width
        to: 0
        duration: Theme.motion.durationSlow
        easing.type: Easing.OutCubic
    }
    NumberAnimation {
        id: _slideOut
        target: _content
        property: "x"
        from: 0
        to: drawer.width
        duration: Theme.motion.durationBase
        easing.type: Easing.InCubic
    }

    Item {
        id: _scrim
        anchors.fill: parent
        visible: drawer.visible
        Rectangle {
            anchors.fill: parent
            color: Theme.color.overlayScrim
            MouseArea { anchors.fill: parent; onClicked: drawer.close() }
        }
    }

    Rectangle {
        id: _content
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: drawer.width
        color: Theme.color.cardBackground

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.color.divider
        }

        Column {
            anchors.fill: parent
            spacing: 0

            // Header
            Rectangle {
                width: parent.width
                height: 72
                color: "transparent"
                Rectangle {
                    anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                    height: 1; color: Theme.color.divider
                }
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.space.lg; anchors.rightMargin: Theme.space.lg
                    spacing: Theme.space.md
                    Text {
                        text: "Book details"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    IconButton {
                        iconName: "close"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: drawer.close()
                    }
                }
            }

            // Body
            ScrollView {
                width: parent.width
                height: parent.height - 72 - 88
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: Theme.space.lg

                    // Cover + title + author + status
                    Item {
                        width: parent.width
                        height: 140
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.space.xl; anchors.rightMargin: Theme.space.xl
                            spacing: Theme.space.lg

                            BookCover {
                                width: 80; height: 120
                                book: drawer._detail
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Column {
                                spacing: 6
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    width: parent.width - 80 - Theme.space.lg
                                    text: drawer._detail.title || "(no title)"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold
                                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                                }
                                Text {
                                    text: "by " + (drawer._detail.authorName || "—")
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody
                                }
                                Text {
                                    text: drawer._detail.publisherName || "—"
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption
                                }
                                Row {
                                    spacing: Theme.space.xs
                                    Rectangle { width: 8; height: 8; radius: 4; color: drawer._statusColor(drawer._detail.status || "published"); anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        text: drawer._statusLabel(drawer._detail.status || "published")
                                        color: drawer._statusColor(drawer._detail.status || "published")
                                        font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }

                    // Stats grid
                    Row {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.space.md
                        Repeater {
                            model: [
                                { label: "Price", value: drawer._detail.priceText || "—", color: Theme.color.accent },
                                { label: "Sales", value: (drawer._detail.totalSales || 0).toString(), color: Theme.color.success },
                                { label: "Rating", value: (drawer._detail.averageRating || 0).toFixed(1), color: Theme.color.warning },
                                { label: "Reviews", value: (drawer._detail.ratingCount || 0).toString(), color: Theme.color.info }
                            ]
                            Rectangle {
                                width: (parent.width - 3 * Theme.space.md) / 4; height: 80; radius: Theme.radius.md
                                color: Theme.color.fieldFilled; border.color: Theme.color.divider
                                Column {
                                    anchors.centerIn: parent; spacing: 2
                                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.value; color: modelData.color; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeTitle; font.weight: Theme.font.weightBold }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.label; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                                }
                            }
                        }
                    }

                    // Description
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"; bordered: true; padding: Theme.space.lg
                        Column {
                            anchors.fill: parent; spacing: Theme.space.sm
                            SectionHeader { width: parent.width; title: "Description" }
                            Text { width: parent.width; text: drawer._detail.description || "No description available."; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; wrapMode: Text.WordWrap }
                        }
                    }

                    // Reviews
                    Card {
                        width: parent.width - 2 * Theme.space.xl
                        anchors.horizontalCenter: parent.horizontalCenter
                        elevation: "none"; bordered: true; padding: Theme.space.lg
                        Column {
                            anchors.fill: parent; spacing: Theme.space.sm
                            SectionHeader { width: parent.width; title: "Reviews"; subtitle: "%1 total".arg((drawer._detail.reviews || []).length) }
                            ListView {
                                width: parent.width
                                height: Math.min(360, Math.max(0, (drawer._detail.reviews || []).length) * 80)
                                clip: true; interactive: (drawer._detail.reviews || []).length > 3
                                model: drawer._detail.reviews || []
                                spacing: Theme.space.sm
                                delegate: Rectangle {
                                    width: parent.width; radius: Theme.radius.md; color: Theme.color.fieldFilled; border.color: Theme.color.divider
                                    height: _revCol.implicitHeight + 2 * Theme.space.md
                                    Column {
                                        id: _revCol; anchors.fill: parent; anchors.margins: Theme.space.md; spacing: Theme.space.xs
                                        Row { width: parent.width; spacing: Theme.space.sm; RatingStars { size: 12; rating: modelData.rating }; Item { width: 1; Layout.fillWidth: true; height: 1 }; Text { text: modelData.createdAtText || ""; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption } }
                                        Text { text: "by @" + (modelData.username || "—"); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                                        Text { width: parent.width; text: "\"" + (modelData.comment || "") + "\""; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight }
                                        Row {
                                            spacing: Theme.space.sm
                                            Text { text: "▲ " + (modelData.helpfulCount || 0) + " helpful"; color: Theme.color.textMuted; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption }
                                            SecondaryButton {
                                                text: "Remove"
                                                iconName: "delete"
                                                onClicked: {
                                                    if (drawer.viewModel && typeof drawer.viewModel.deleteReview === "function") {
                                                        drawer.viewModel.deleteReview(modelData.id)
                                                        drawer.toastRequested("warning", "Review removed", "Review by @" + modelData.username + " was deleted.")
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            EmptyState {
                                width: parent.width; height: 120
                                visible: (drawer._detail.reviews || []).length === 0
                                iconName: "rate_review"; title: "No reviews"; description: "This book has no reviews yet."
                            }
                        }
                    }

                    Item { width: 1; height: Theme.space.xl }
                }
            }

            // Footer action bar
            Rectangle {
                width: parent.width; height: 88; color: Theme.color.cardBackground
                Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: 1; color: Theme.color.divider }
                Row {
                    anchors.fill: parent; anchors.leftMargin: Theme.space.xl; anchors.rightMargin: Theme.space.xl; anchors.topMargin: Theme.space.md; anchors.bottomMargin: Theme.space.md
                    spacing: Theme.space.md
                    PrimaryButton {
                        text: drawer._detail.status === "removed" ? "Re-publish" : "Remove book"
                        iconName: drawer._detail.status === "removed" ? "history" : "delete"
                        enabled: drawer.bookId.length > 0
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            if (!drawer.viewModel) return
                            if (drawer._detail.status === "removed") {
                                // Re-publish: set status back to "published".
                                drawer.viewModel.setBookStatus(drawer.bookId, "published")
                                drawer.toastRequested("success", "Re-published", "'" + drawer._detail.title + "' is back in the storefront.")
                            } else {
                                // Soft-delete: set status to "removed".
                                drawer.viewModel.deleteBook(drawer.bookId, "Admin policy violation")
                                drawer.toastRequested("warning", "Removed", "'" + drawer._detail.title + "' has been removed.")
                            }
                        }
                    }
                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                    SecondaryButton {
                        text: "Close"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: drawer.close()
                    }
                }
            }
        }
    }
}
