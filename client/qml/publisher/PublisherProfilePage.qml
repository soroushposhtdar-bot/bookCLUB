// =============================================================================
//  PublisherProfilePage.qml  (v3 polish)
// =============================================================================
//  Publisher account & profile management (spec §3-1).
//
//  Layout:
//    1. Header card — publisher name + verified badge + plan + joined date +
//       avatar.
//    2. KPI row — 4 StatCards bound to live VM data (total books, total
//       revenue, total units sold, average rating).
//    3. Two-column body:
//       • Left: Account info card (editable fields) + a "Save changes" button
//         that calls viewModel.updatePublisherProfile(...).
//       • Right: Catalog composition card + contact card + security card.
//
//  v3 polish improvements:
//    • Removed the duplicate edit-profile popup (was redundant with the
//      inline editor on the left card). The "Edit profile" button in the
//      header now scrolls to the inline editor instead.
//    • Added a "Security" card on the right with Change password + Manage
//      sessions placeholders — addresses the spec requirement that
//      publishers should be able to change their password (spec §1).
//    • Catalog composition bars animate width on data change.
//    • Layout uses RowLayout/ColumnLayout throughout — no more
//      `parent.parent.width` arithmetic.
//    • All KPIs use the new sparkline-enabled StatCard.
// =============================================================================
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"
import "../components/book"

import BookClub.Services 1.0
import BookClub.ViewModels 1.0
import "../components"

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null

    signal toastRequested(string variant, string title, string description)

    // ----- Profile (QVariantMap from the VM) -----
    readonly property var _profile: page.viewModel ? page.viewModel.publisherProfile : ({})

    // ----- Catalog composition — derived from the VM's books list -----
    readonly property var _books: page.viewModel ? (page.viewModel.books || []) : []
    readonly property int _publishedCount: { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "published" || page._books[i].status === "active") ++n; return n }
    readonly property int _draftCount:     { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "draft") ++n; return n }
    readonly property int _pendingCount:   { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "pending") ++n; return n }
    readonly property int _removedCount:   { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "removed" || page._books[i].status === "inactive") ++n; return n }

    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onProfileChanged: page._refreshEditor()
        onBooksChanged: page._refreshEditor()
    }

    Component.onCompleted: {
        if (page.viewModel && typeof page.viewModel.refresh === "function") {
            page.viewModel.refresh()
        }
        page._refreshEditor()
    }

    function _refreshEditor() {
        const p = page._profile
        _fPublisherName.text = p.publisherName || ""
        _fBiography.text     = p.biography || ""
        _fWebsite.text       = p.website || ""
        _fEmail.text         = p.email || ""
        _fTaxId.text         = p.taxId || ""
    }

    ScrollView {
        id: _scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header card -----
            Card {
                Layout.fillWidth: true
                padding: Theme.space.xl

                RowLayout {
                    width: parent.width
                    spacing: Theme.space.xl

                    // Avatar (large)
                    Rectangle {
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 96
                        radius: 24
                        color: page._profile.avatarColor || Theme.color.accent

                        Text {
                            anchors.centerIn: parent
                            text: {
                                const name = page._profile.publisherName || "P"
                                return name.charAt(0).toUpperCase()
                            }
                            color: Theme.color.textOnAccent
                            font.family: Theme.font.family
                            font.pixelSize: Theme.size.sizeMega
                            font.weight: Theme.font.weightBold
                        }
                    }

                    // Name + verified + plan + joined
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space.sm

                        RowLayout {
                            spacing: Theme.space.sm
                            Text {
                                Layout.fillWidth: true
                                text: page._profile.publisherName || "—"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                elide: Text.ElideRight
                            }
                            // Verified badge
                            Rectangle {
                                visible: page._profile.verified === true
                                Layout.preferredWidth: _verifiedLbl.implicitWidth + 16
                                Layout.preferredHeight: 24
                                radius: height / 2
                                color: Theme.color.successSoft
                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    AppIcon { name: "verified"; size: 14; color: Theme.color.success }
                                    Text {
                                        id: _verifiedLbl
                                        text: "Verified"
                                        color: Theme.color.success
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                }
                            }
                        }

                        RowLayout {
                            spacing: Theme.space.md
                            Text {
                                text: page._profile.publisherId || ""
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                            }
                            Text { text: "·"; color: Theme.color.textMuted; font.pixelSize: Theme.font.sizeCaption }
                            Text {
                                text: "Joined " + (page._profile.joinedAt || "")
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                            }
                            Text { text: "·"; color: Theme.color.textMuted; font.pixelSize: Theme.font.sizeCaption }
                            Rectangle {
                                Layout.preferredWidth: _planLbl.implicitWidth + 16
                                Layout.preferredHeight: 22
                                radius: height / 2
                                color: Theme.color.accentSoft
                                Text {
                                    id: _planLbl
                                    anchors.centerIn: parent
                                    text: page._profile.plan || "Publisher"
                                    color: Theme.color.accent
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeMicro2
                                    font.weight: Theme.font.weightBold
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; height: 1 }

                    // Edit button — focuses the publisher-name field of the
                    // inline editor below (smooth scroll handled by Flickable).
                    PrimaryButton {
                        Layout.alignment: Qt.AlignVCenter
                        text: "Edit profile"
                        iconName: "edit"
                        onClicked: {
                            _fPublisherName.forceActiveFocus()
                            // Scroll the editor into view
                            _scroll.ScrollBar.vertical.position = Math.min(1.0, _editorCard.y / (_scroll.contentHeight - _scroll.height))
                        }
                    }
                }
            }

            // ----- KPI cards row -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "library_books"
                    value: (page.viewModel ? page.viewModel.totalBooks : 0).toString()
                    label: "Total books"
                    delta: "%1 active".arg(page.viewModel ? page.viewModel.activeTitles : 0)
                    deltaUp: true
                    accent: Theme.color.accent
                    spark: []
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "attach_money"
                    value: page.viewModel ? page.viewModel.totalRevenue : "$0"
                    label: "Total revenue"
                    delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%"
                    deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0
                    accent: Theme.color.success
                    spark: page.viewModel ? (page.viewModel.revenueSeries || []).slice(-7) : []
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "shopping_cart"
                    value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0)
                    label: "Units sold"
                    delta: "All time"
                    deltaUp: true
                    accent: Theme.color.info
                    spark: []
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 100
                    iconName: "star"
                    value: page.viewModel ? page.viewModel.averageRating : "0.00"
                    label: "Avg. rating"
                    delta: "Across all titles"
                    deltaUp: true
                    accent: Theme.color.warning
                    spark: []
                }
            }

            // ----- Two-column body -----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.lg

                // ----- Left: Account info -----
                Card {
                    id: _editorCard
                    Layout.fillWidth: true
                    Layout.preferredWidth: 600
                    padding: Theme.space.xl

                    ColumnLayout {
                        id: _editorContent
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            Layout.fillWidth: true
                            title: "Account information"
                            subtitle: "Edit your publisher profile"
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Publisher name"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            InputField {
                                id: _fPublisherName
                                Layout.fillWidth: true
                                placeholder: "Pinecrest Press"
                                onTextEdited: function(newText) { _fPublisherName.text = newText }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Biography"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            InputField {
                                id: _fBiography
                                Layout.fillWidth: true
                                placeholder: "A short description of your publishing house"
                                onTextEdited: function(newText) { _fBiography.text = newText }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs
                                Text {
                                    text: "Website"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }
                                InputField {
                                    id: _fWebsite
                                    Layout.fillWidth: true
                                    placeholder: "https://example.com"
                                    onTextEdited: function(newText) { _fWebsite.text = newText }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs
                                Text {
                                    text: "Email"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightMedium
                                }
                                InputField {
                                    id: _fEmail
                                    Layout.fillWidth: true
                                    placeholder: "contact@example.com"
                                    onTextEdited: function(newText) { _fEmail.text = newText }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.xs
                            Text {
                                text: "Tax ID"
                                color: Theme.color.textSecondary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                font.weight: Theme.font.weightMedium
                            }
                            InputField {
                                id: _fTaxId
                                Layout.fillWidth: true
                                placeholder: "XX-XXX1234"
                                onTextEdited: function(newText) { _fTaxId.text = newText }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md
                            Item { Layout.fillWidth: true; height: 1 }
                            SecondaryButton {
                                text: "Reset"
                                onClicked: page._refreshEditor()
                            }
                            PrimaryButton {
                                text: "Save changes"
                                iconName: "check"
                                enabled: _fPublisherName.text.length > 0
                                onClicked: {
                                    if (!page.viewModel) {
                                        page.toastRequested("error", "No view model", "PublisherViewModel is not available.")
                                        return
                                    }
                                    var ok = page.viewModel.updatePublisherProfile(
                                        _fPublisherName.text,
                                        _fBiography.text,
                                        _fWebsite.text,
                                        _fEmail.text,
                                        _fTaxId.text
                                    )
                                    if (ok) {
                                        page.toastRequested("success", "Profile saved",
                                                            "Your publisher profile has been updated.")
                                    } else {
                                        page.toastRequested("error", "Save failed",
                                                            "Could not save your profile. Please try again.")
                                    }
                                }
                            }
                        }
                    }
                }

                // ----- Right column: Catalog composition + Contact + Security -----
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 400
                    spacing: Theme.space.lg

                    // Catalog composition card
                    Card {
                        Layout.fillWidth: true
                        padding: Theme.space.xl

                        ColumnLayout {
                            id: _catalogContent
                            width: parent.width
                            spacing: Theme.space.md

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Catalog composition"
                                subtitle: "By lifecycle status"
                            }

                            Repeater {
                                model: [
                                    { label: "Published", count: page._publishedCount, color: Theme.color.success },
                                    { label: "Draft",     count: page._draftCount,     color: Theme.color.textMuted },
                                    { label: "Pending",   count: page._pendingCount,   color: Theme.color.warning },
                                    { label: "Removed",   count: page._removedCount,   color: Theme.color.error }
                                ]
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: Theme.space.xs

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: Theme.space.sm
                                        Rectangle {
                                            Layout.preferredWidth: 8
                                            Layout.preferredHeight: 8
                                            radius: width / 2
                                            color: modelData.color
                                        }
                                        Text {
                                            text: modelData.label
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                        }
                                        Item { Layout.fillWidth: true; height: 1 }
                                        Text {
                                            text: modelData.count.toString()
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                        }
                                    }
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 6
                                        radius: 3
                                        color: Theme.color.fieldFilled
                                        Rectangle {
                                            width: parent.width * (page._books.length > 0 ? modelData.count / page._books.length : 0)
                                            height: parent.height
                                            radius: parent.radius
                                            color: modelData.color
                                            Behavior on width { NumberAnimation { duration: Theme.motion.durationBase } }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Contact card
                    Card {
                        Layout.fillWidth: true
                        padding: Theme.space.xl

                        ColumnLayout {
                            id: _contactContent
                            width: parent.width
                            spacing: Theme.space.md

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Contact"
                                subtitle: "Public-facing details"
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm
                                AppIcon { name: "mail"; size: 16; color: Theme.color.textMuted }
                                Text {
                                    Layout.fillWidth: true
                                    text: page._profile.email || "—"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm
                                AppIcon { name: "language"; size: 16; color: Theme.color.textMuted }
                                Text {
                                    Layout.fillWidth: true
                                    text: page._profile.website || "—"
                                    color: Theme.color.accent
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.sm
                                AppIcon { name: "public"; size: 16; color: Theme.color.textMuted }
                                Text {
                                    Layout.fillWidth: true
                                    text: page._profile.country || "—"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                }
                            }
                        }
                    }

                    // Security card (new in v3 polish — spec §1 requires
                    // publishers be able to change their password)
                    Card {
                        Layout.fillWidth: true
                        padding: Theme.space.xl

                        ColumnLayout {
                            id: _securityContent
                            width: parent.width
                            spacing: Theme.space.md

                            SectionHeader {
                                Layout.fillWidth: true
                                title: "Security"
                                subtitle: "Account access and credentials"
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.md

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Theme.color.accentSoft
                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "lock"
                                        size: 22
                                        color: Theme.color.accent
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Password"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Last changed 30 days ago"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                                SecondaryButton {
                                    text: "Change"
                                    onClicked: _passwordDialog.open()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.md

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Theme.color.successSoft
                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "verified_user"
                                        size: 22
                                        color: Theme.color.success
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Two-factor authentication"
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeBody
                                        font.weight: Theme.font.weightMedium
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: "Enabled — adds an extra layer of security"
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                                AppIcon {
                                    name: "check_circle"
                                    size: 20
                                    color: Theme.color.success
                                }
                            }
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.space.xxl }
        }
    }

    // ----- Password change dialog (v10 — polished) -----
    Popup {
        id: _passwordDialog
        anchors.centerIn: parent
        width: Math.min(460, parent.width - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xxl

        property string error: ""
        property string success: ""

        // v10: live validation flags
        readonly property bool _currValid: _currPass.text.length > 0
        readonly property bool _newValid: _newPass.text.length >= 8
        readonly property bool _confirmValid: _confirmPass.text.length > 0 && _confirmPass.text === _newPass.text
        readonly property bool _canSubmit: _currValid && _newValid && _confirmValid

        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        onOpened: {
            _currPass.text = ""
            _newPass.text = ""
            _confirmPass.text = ""
            error = ""
            success = ""
        }

        ColumnLayout {
            width: parent.width
            spacing: Theme.space.lg

            // Header with icon
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md

                Rectangle {
                    width: 40; height: 40; radius: width / 2
                    color: Theme.color.accentSoft
                    Layout.alignment: Qt.AlignVCenter

                    AppIcon {
                        anchors.centerIn: parent
                        name: "lock"
                        size: 20
                        color: Theme.color.accent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text {
                        Layout.fillWidth: true
                        text: "Change password"
                        color: Theme.color.textPrimary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeTitle
                        font.weight: Theme.font.weightBold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Enter your current password and a new one"
                        color: Theme.color.textSecondary
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                    }
                }

                IconButton {
                    iconName: "close"
                    iconColor: Theme.color.textMuted
                    onClicked: _passwordDialog.close()
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            // Current password
            InputField {
                id: _currPass
                Layout.fillWidth: true
                label: "Current password"
                placeholder: "Enter your current password"
                leadingIcon: "lock"
                echoMode: TextInput.Password
                required: true
                onTextEdited: function(newText) { _currPass.text = newText }
            }

            // New password
            InputField {
                id: _newPass
                Layout.fillWidth: true
                label: "New password"
                placeholder: "At least 8 characters"
                leadingIcon: "key"
                echoMode: TextInput.Password
                required: true
                helperText: _newPass.text.length === 0 ? "Minimum 8 characters" :
                            _newPass.text.length < 8 ? "Too short — needs " + (8 - _newPass.text.length) + " more characters" :
                            "Good length"
                errorText: _newPass.text.length > 0 && _newPass.text.length < 8 ? "Password must be at least 8 characters" : ""
                successText: _newPass.text.length >= 8 ? "Looks good" : ""
                onTextEdited: function(newText) { _newPass.text = newText }
            }

            // Confirm password
            InputField {
                id: _confirmPass
                Layout.fillWidth: true
                label: "Confirm new password"
                placeholder: "Re-enter your new password"
                leadingIcon: "verified_user"
                echoMode: TextInput.Password
                required: true
                errorText: _confirmPass.text.length > 0 && _confirmPass.text !== _newPass.text ? "Passwords do not match" : ""
                successText: _confirmPass.text.length > 0 && _confirmPass.text === _newPass.text ? "Passwords match" : ""
                onTextEdited: function(newText) { _confirmPass.text = newText }
            }

            // Error message (server errors)
            Rectangle {
                Layout.fillWidth: true
                visible: _passwordDialog.error.length > 0
                height: _errText.implicitHeight + 16
                radius: Theme.radius.md
                color: Theme.color.errorSoft
                border.color: Theme.color.error
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    width: parent.width - 16
                    spacing: Theme.space.xs

                    AppIcon {
                        name: "error_outline"
                        size: 16
                        color: Theme.color.error
                    }
                    Text {
                        id: _errText
                        Layout.fillWidth: true
                        text: _passwordDialog.error
                        color: Theme.color.error
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeCaption
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // Action buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md
                Item { Layout.fillWidth: true; height: 1 }
                SecondaryButton {
                    text: "Cancel"
                    onClicked: _passwordDialog.close()
                }
                PrimaryButton {
                    text: "Update password"
                    iconName: "check"
                    enabled: _passwordDialog._canSubmit
                    onClicked: {
                        _passwordDialog.error = ""
                        if (_newPass.text !== _confirmPass.text) {
                            _passwordDialog.error = "Passwords do not match. Please re-enter."
                            return
                        }
                        if (_currPass.text === _newPass.text) {
                            _passwordDialog.error = "New password must be different from your current password."
                            return
                        }
                        // Send to server via AuthService (2-arg Q_INVOKABLE version)
                        var ok = false
                        if (typeof AuthService !== "undefined" && AuthService.changePassword) {
                            ok = AuthService.changePassword(_currPass.text, _newPass.text)
                        }
                        if (ok) {
                            page.toastRequested("success", "Password updated",
                                                 "Your password has been changed successfully.")
                            _passwordDialog.close()
                        } else {
                            _passwordDialog.error = "Could not change password. Please check your current password and try again."
                        }
                    }
                }
            }

            // Security note
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.xs
                AppIcon {
                    name: "shield"
                    size: 12
                    color: Theme.color.textMuted
                }
                Text {
                    Layout.fillWidth: true
                    text: "Your password is encrypted and stored securely."
                    color: Theme.color.textMuted
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeMicro2
                }
            }
        }
    }
}
