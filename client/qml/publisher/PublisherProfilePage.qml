// =============================================================================
//  PublisherProfilePage.qml
// =============================================================================
//  Publisher account & profile management (spec §3-1: personal information &
//  account management for the publisher role).
//
//  Layout:
//    1. Header card — publisher name + verified badge + plan + joined date +
//       avatar (with edit button to open the profile editor dialog).
//    2. KPI row — 4 StatCards bound to live VM data (total books, total
//       revenue, total units sold, average rating).
//    3. Two-column body:
//       • Left: Account info card (editable fields) + a "Save changes" button
//         that calls viewModel.updatePublisherProfile(...).
//       • Right: Publisher stats card (catalog composition — published/draft/
//         pending/removed counts) + verified-contact card.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `publisherProfile` (QVariantMap) + `updatePublisherProfile(...)`. Profile
//  changes emit profileChanged so the header + KPI cards refresh immediately.
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

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)

    // ----- Profile (QVariantMap from the VM) -----
    readonly property var _profile: page.viewModel ? page.viewModel.publisherProfile : ({})

    // ----- Catalog composition — derived from the VM's books list -----
    readonly property var _books: page.viewModel ? (page.viewModel.books || []) : []
    readonly property int _publishedCount: { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "published") ++n; return n }
    readonly property int _draftCount:     { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "draft") ++n; return n }
    readonly property int _pendingCount:   { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "pending") ++n; return n }
    readonly property int _removedCount:   { let n = 0; for (let i = 0; i < page._books.length; ++i) if (page._books[i].status === "removed") ++n; return n }

    // ----- Re-pull the profile when the VM signals profileChanged -----
    Connections {
        target: page.viewModel
        ignoreUnknownSignals: true
        onProfileChanged: page._refreshEditor()
        // Catalog composition (published/draft/pending/removed counts) derives
        // from viewModel.books, so we need to refresh when books change too.
        // Previously this page had no onBooksChanged handler, so the bars
        // stayed stale after catalog edits from other pages.
        onBooksChanged: {
            // The bindings on _publishedCount etc. re-evaluate automatically,
            // but we explicitly trigger a refresh of the editor in case the
            // profile's catalog stats (totalBooks/activeTitles) also changed.
            page._refreshEditor()
        }
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
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- Header card -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 0
                    anchors.rightMargin: 0
                    spacing: Theme.space.xl

                    // Avatar (large)
                    Rectangle {
                        width: 96; height: 96; radius: 24
                        color: page._profile.avatarColor || Theme.color.accent
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            anchors.centerIn: parent
                            text: {
                                const name = page._profile.publisherName || "P"
                                return name.charAt(0).toUpperCase()
                            }
                            color: Theme.color.textOnAccent
                            font.family: Theme.font.family
                            font.pixelSize: Theme.font.sizeMega
                            font.weight: Theme.font.weightBold
                        }
                    }

                    // Name + verified + plan + joined
                    Column {
                        spacing: Theme.space.sm
                        anchors.verticalCenter: parent.verticalCenter

                        Row {
                            spacing: Theme.space.sm
                            Text {
                                text: page._profile.publisherName || "—"
                                color: Theme.color.textPrimary
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeTitle
                                font.weight: Theme.font.weightBold
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            // Verified badge
                            Rectangle {
                                visible: page._profile.verified === true
                                width: _verifiedLbl.implicitWidth + 16
                                height: 24
                                radius: 12
                                color: Theme.color.successSoft
                                anchors.verticalCenter: parent.verticalCenter
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    AppIcon { name: "verified"; size: 14; color: Theme.color.success; anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        id: _verifiedLbl
                                        text: "Verified"
                                        color: Theme.color.success
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }

                        Row {
                            spacing: Theme.space.md
                            Text {
                                text: page._profile.publisherId || ""
                                color: Theme.color.textMuted
                                font.family: Theme.font.familyMono
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "·"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "Joined " + (page._profile.joinedAt || "")
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: "·"
                                color: Theme.color.textMuted
                                font.family: Theme.font.family
                                font.pixelSize: Theme.font.sizeCaption
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Rectangle {
                                width: _planLbl.implicitWidth + 16
                                height: 22
                                radius: 11
                                color: Theme.color.accentSoft
                                anchors.verticalCenter: parent.verticalCenter
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

                    Item { width: 1; Layout.fillWidth: true; height: 1 }

                    // Edit button
                    PrimaryButton {
                        text: "Edit profile"
                        iconName: "edit"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: _editDialog.open()
                    }
                }
            }

            // ----- KPI cards row -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "library_books"
                    value: (page.viewModel ? page.viewModel.totalBooks : 0).toString()
                    label: "Total books"
                    delta: "%1 active".arg(page.viewModel ? page.viewModel.activeTitles : 0)
                    deltaUp: true
                    accent: Theme.color.accent
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "attach_money"
                    value: page.viewModel ? page.viewModel.totalRevenue : "$0"
                    label: "Total revenue"
                    delta: page.viewModel ? page.viewModel.revenueTrend : "+0.0%"
                    deltaUp: (page.viewModel ? page.viewModel.revenueTrend : "+0.0%").indexOf("+") === 0
                    accent: Theme.color.success
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "shopping_cart"
                    value: (page.viewModel ? page.viewModel.totalUnitsSold : 0).toLocaleString(Qt.locale(), "f", 0)
                    label: "Units sold"
                    delta: "All time"
                    deltaUp: true
                    accent: Theme.color.info
                }
                StatCard {
                    width: (parent.width - 3 * Theme.space.lg) / 4
                    iconName: "star"
                    value: page.viewModel ? page.viewModel.averageRating : "0.00"
                    label: "Avg. rating"
                    delta: "Across all titles"
                    deltaUp: true
                    accent: Theme.color.warning
                }
            }

            // ----- Two-column body -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                // ----- Left: Account info -----
                Card {
                    width: parent.width * 0.60 - Theme.space.lg / 2
                    padding: Theme.space.xl

                    Column {
                        anchors.fill: parent
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Account information"
                            subtitle: "Edit your publisher profile"
                        }

                        // Editable fields
                        Text { text: "Publisher name"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                        InputField { id: _fPublisherName; width: parent.width; placeholder: "Pinecrest Press" }

                        Text { text: "Biography"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                        InputField { id: _fBiography; width: parent.width; placeholder: "A short description of your publishing house" }

                        Row {
                            width: parent.width
                            spacing: Theme.space.md
                            Column {
                                width: (parent.width - Theme.space.md) / 2
                                spacing: Theme.space.sm
                                Text { text: "Website"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                                InputField { id: _fWebsite; width: parent.width; placeholder: "https://example.com" }
                            }
                            Column {
                                width: (parent.width - Theme.space.md) / 2
                                spacing: Theme.space.sm
                                Text { text: "Email"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                                InputField { id: _fEmail; width: parent.width; placeholder: "contact@example.com" }
                            }
                        }

                        Text { text: "Tax ID"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                        InputField { id: _fTaxId; width: parent.width; placeholder: "XX-XXX1234" }

                        Row {
                            width: parent.width
                            spacing: Theme.space.md
                            Item { width: 1; Layout.fillWidth: true; height: 1 }
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
                                    page.viewModel.updatePublisherProfile(
                                        _fPublisherName.text,
                                        _fBiography.text,
                                        _fWebsite.text,
                                        _fEmail.text,
                                        _fTaxId.text
                                    )
                                    page.toastRequested("success", "Profile saved",
                                                        "Your publisher profile has been updated.")
                                }
                            }
                        }
                    }
                }

                // ----- Right: Catalog composition + contact -----
                Column {
                    width: parent.width * 0.40 - Theme.space.lg / 2
                    spacing: Theme.space.lg

                    // Catalog composition card
                    Card {
                        width: parent.width
                        padding: Theme.space.xl

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Catalog composition"
                                subtitle: "By lifecycle status"
                            }

                            // 4 status rows with bars
                            Repeater {
                                model: [
                                    { label: "Published", count: page._publishedCount, color: Theme.color.success },
                                    { label: "Draft",     count: page._draftCount,     color: Theme.color.textMuted },
                                    { label: "Pending",   count: page._pendingCount,   color: Theme.color.warning },
                                    { label: "Removed",   count: page._removedCount,   color: Theme.color.error }
                                ]
                                Column {
                                    width: parent.width
                                    spacing: Theme.space.xs

                                    Row {
                                        width: parent.width
                                        spacing: Theme.space.sm
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: modelData.color
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            text: modelData.label
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightMedium
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                                        Text {
                                            text: modelData.count.toString()
                                            color: Theme.color.textPrimary
                                            font.family: Theme.font.family
                                            font.pixelSize: Theme.font.sizeBody
                                            font.weight: Theme.font.weightBold
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                    Rectangle {
                                        width: parent.width
                                        height: 6
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
                        width: parent.width
                        padding: Theme.space.xl

                        Column {
                            anchors.fill: parent
                            spacing: Theme.space.md

                            SectionHeader {
                                width: parent.width
                                title: "Contact"
                                subtitle: "Public-facing details"
                            }

                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "mail"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: page._profile.email || "—"
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "language"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: page._profile.website || "—"
                                    color: Theme.color.accent
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    elide: Text.ElideRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            Row {
                                width: parent.width
                                spacing: Theme.space.sm
                                AppIcon { name: "public"; size: 16; color: Theme.color.textMuted; anchors.verticalCenter: parent.verticalCenter }
                                Text {
                                    text: page._profile.country || "—"
                                    color: Theme.color.textSecondary
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeBody
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }

            // ----- Bottom footer spacer (prevents the last card from sitting
            //       flush against the scroll viewport edge) -----
            Item { width: 1; height: Theme.space.xxl }
        }
    }

    // ----- Edit-profile dialog (alt entry point — opens the same fields
    //       in a popup as the inline editor on the left card) -----
    Popup {
        id: _editDialog
        anchors.centerIn: parent
        width: Math.min(540, parent.width - 64)
        height: Math.min(600, parent.height - 64)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: Theme.space.xl

        background: Card { elevation: "xl"; bordered: false; radius: Theme.radius.lg }

        Column {
            anchors.fill: parent
            spacing: Theme.space.md

            SectionHeader {
                width: parent.width
                title: "Edit publisher profile"
                subtitle: "Changes are saved to your publisher record"
            }

            ScrollView {
                width: parent.width
                height: parent.height - 130
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    Text { text: "Publisher name"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                    InputField { id: _dName; width: parent.width; placeholder: "Pinecrest Press" }

                    Text { text: "Biography"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _dBio; width: parent.width; placeholder: "A short description" }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md
                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Website"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _dWeb; width: parent.width; placeholder: "https://example.com" }
                        }
                        Column {
                            width: (parent.width - Theme.space.md) / 2
                            spacing: Theme.space.sm
                            Text { text: "Email"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _dEmail; width: parent.width; placeholder: "contact@example.com" }
                        }
                    }

                    Text { text: "Tax ID"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                    InputField { id: _dTax; width: parent.width; placeholder: "XX-XXX1234" }
                }
            }

            Row {
                width: parent.width
                spacing: Theme.space.md
                Item { width: 1; Layout.fillWidth: true; height: 1 }
                SecondaryButton { text: "Cancel"; onClicked: _editDialog.close() }
                PrimaryButton {
                    text: "Save"
                    iconName: "check"
                    enabled: _dName.text.length > 0
                    onClicked: {
                        if (page.viewModel) {
                            page.viewModel.updatePublisherProfile(_dName.text, _dBio.text, _dWeb.text, _dEmail.text, _dTax.text)
                            page.toastRequested("success", "Profile saved", "Your publisher profile has been updated.")
                        }
                        _editDialog.close()
                    }
                }
            }
        }

        onAboutToShow: {
            // Pre-fill the dialog fields from the current profile.
            const p = page._profile
            _dName.text  = p.publisherName || ""
            _dBio.text   = p.biography || ""
            _dWeb.text   = p.website || ""
            _dEmail.text = p.email || ""
            _dTax.text   = p.taxId || ""
        }
    }
}
