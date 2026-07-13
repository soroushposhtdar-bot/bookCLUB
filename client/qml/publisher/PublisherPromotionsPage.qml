// =============================================================================
//  PublisherPromotionsPage.qml
// =============================================================================
//  Discount / promo code management for the publisher role. Create new
//  promotions, list active/past promotions, and toggle their state.
//
//  Data source: page.viewModel (PublisherViewModel). The VM exposes
//  `promotions` (QVariantList of { code, description, scope, discount,
//  status, uses, cap, startDate, endDate, period }) plus
//  `addPromotion(...)` and `removePromotion(code)`.
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
import "../components/selection"
import BookClub.Services 1.0
import BookClub.ViewModels 1.0

Item {
    id: page
    anchors.fill: parent

    property var viewModel: null   // PublisherViewModel

    signal toastRequested(string variant, string title, string description)

    // ----- Promo list (QVariantList from the VM) -----
    readonly property var _promos: page.viewModel ? page.viewModel.promotions : []

    // ----- Derived KPIs (recomputed whenever the promo list changes) -----
    readonly property int _promoCount: page._promos ? page._promos.length : 0
    readonly property int _totalUses: {
        if (!page._promos) return 0
        let sum = 0
        for (let i = 0; i < page._promos.length; ++i) sum += (page._promos[i].uses || 0)
        return sum
    }
    readonly property int _avgDiscount: {
        if (!page._promos || page._promos.length === 0) return 0
        let sum = 0
        for (let i = 0; i < page._promos.length; ++i) sum += (page._promos[i].discount || 0)
        return Math.round(sum / page._promos.length)
    }

    function _statusColor(s) {
        return { active: Theme.color.success, scheduled: Theme.color.info, expired: Theme.color.textMuted }[s] || Theme.color.textMuted
    }
    function _statusLabel(s) {
        return { active: "Active", scheduled: "Scheduled", expired: "Expired" }[s] || s
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // ----- KPI cards -----
            Row {
                width: parent.width
                spacing: Theme.space.lg

                StatCard { width: (parent.width - 2 * Theme.space.lg) / 3; iconName: "local_offer";    value: page._promoCount.toString();                          label: "Promotions";         delta: "Active promos"; deltaUp: true; accent: Theme.color.accent  }
                StatCard { width: (parent.width - 2 * Theme.space.lg) / 3; iconName: "percent";        value: "%1%".arg(page._avgDiscount);                          label: "Avg. discount";      delta: "Across all promos"; deltaUp: true; accent: Theme.color.warning }
                StatCard { width: (parent.width - 2 * Theme.space.lg) / 3; iconName: "shopping_cart"; value: page._totalUses.toLocaleString(Qt.locale(), "f", 0);   label: "Redemptions (30d)";  delta: "Total uses"; deltaUp: true; accent: Theme.color.success }
            }

            // ----- Create new promotion -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader { width: parent.width; title: "Create a promotion"; subtitle: "Discounts apply at checkout" }

                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        Column {
                            width: parent.width * 0.5 - Theme.space.lg / 2
                            spacing: Theme.space.sm

                            Text { text: "Promo code"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _code; width: parent.width; placeholder: "SUMMER25" }

                            Text { text: "Description"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                            InputField { id: _desc; width: parent.width; placeholder: "Summer reading — 25% off" }
                        }

                        Column {
                            width: parent.width * 0.5 - Theme.space.lg / 2
                            spacing: Theme.space.sm

                            Text { text: "Discount %"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField { id: _pct; width: parent.width; placeholder: "25" }

                            Text { text: "Usage cap"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium; topPadding: Theme.space.sm }
                            InputField { id: _cap; width: parent.width; placeholder: "1000" }
                        }
                    }

                    // ----- Date pickers row: Start date + End date -----
                    // The mock build doesn't ship a real calendar control, so we
                    // use InputField with a YYYY-MM-DD placeholder + the date
                    // input method hint. The values are passed straight through
                    // to viewModel.addPromotion(code, desc, pct, cap, start, end).
                    Row {
                        width: parent.width
                        spacing: Theme.space.lg

                        Column {
                            width: parent.width * 0.5 - Theme.space.lg / 2
                            spacing: Theme.space.sm

                            Text { text: "Start date"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField {
                                id: _startDate
                                width: parent.width
                                placeholder: "2026-07-01"
                                inputMethodHints: Qt.ImhDateInput
                            }
                        }

                        Column {
                            width: parent.width * 0.5 - Theme.space.lg / 2
                            spacing: Theme.space.sm

                            Text { text: "End date"; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightMedium }
                            InputField {
                                id: _endDate
                                width: parent.width
                                placeholder: "2026-12-31"
                                inputMethodHints: Qt.ImhDateInput
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: Theme.space.md

                        Item { width: 1; Layout.fillWidth: true; height: 1 }
                        SecondaryButton { text: "Reset"; onClicked: { _code.text = ""; _desc.text = ""; _pct.text = ""; _cap.text = ""; _startDate.text = ""; _endDate.text = "" } }
                        PrimaryButton {
                            text: "Create promotion"
                            iconName: "add"
                            onClicked: {
                                if (_code.text.length === 0 || _pct.text.length === 0) {
                                    page.toastRequested("error", "Missing fields", "Promo code and discount % are required.")
                                    return
                                }
                                if (page.viewModel) {
                                    page.viewModel.addPromotion(
                                        _code.text.toUpperCase(),
                                        _desc.text || _code.text,
                                        parseInt(_pct.text) || 0,
                                        parseInt(_cap.text) || 0,
                                        _startDate.text.trim(),
                                        _endDate.text.trim()
                                    )
                                    page.toastRequested("success", "Promotion created", "Promo code '" + _code.text.toUpperCase() + "' is now active.")
                                } else {
                                    page.toastRequested("error", "No view model", "PublisherViewModel is not available.")
                                }
                                _code.text = ""; _desc.text = ""; _pct.text = ""; _cap.text = ""; _startDate.text = ""; _endDate.text = ""
                            }
                        }
                    }
                }
            }

            // ----- Existing promotions table -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                Column {
                    anchors.fill: parent
                    spacing: Theme.space.md

                    SectionHeader { width: parent.width; title: "Your promotions"; subtitle: "%1 total".arg(page._promoCount) }

                    // Header
                    Row {
                        width: parent.width
                        height: 32
                        spacing: 0
                        Repeater {
                            model: [
                                { label: "Code",        w: 0.14 },
                                { label: "Description", w: 0.24 },
                                { label: "Scope",       w: 0.11 },
                                { label: "Discount",    w: 0.09 },
                                { label: "Uses / Cap",  w: 0.11 },
                                { label: "Period",      w: 0.18 },
                                { label: "Status",      w: 0.08 },
                                { label: "",            w: 0.05 }
                            ]
                            Text {
                                width: parent.parent.width * modelData.w
                                text: modelData.label
                                color: Theme.color.textMuted
                                font.family: Theme.font.family; font.pixelSize: Theme.font.sizeCaption; font.weight: Theme.font.weightBold
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: Theme.space.sm
                            }
                        }
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.color.divider }

                    ListView {
                        width: parent.width
                        height: contentHeight
                        clip: true
                        interactive: false
                        model: page._promos
                        spacing: 0

                        delegate: Column {
                            width: parent.width
                            Row {
                                width: parent.width
                                height: 56
                                spacing: 0

                                Text { width: parent.parent.width * 0.14; height: parent.height; text: modelData.code; color: Theme.color.textPrimary; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightBold; verticalAlignment: Text.AlignVCenter; leftPadding: Theme.space.sm }
                                Text { width: parent.parent.width * 0.24; height: parent.height; text: modelData.description; color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; leftPadding: Theme.space.sm }
                                Text { width: parent.parent.width * 0.11; height: parent.height; text: modelData.scope; color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; verticalAlignment: Text.AlignVCenter; leftPadding: Theme.space.sm }
                                Text { width: parent.parent.width * 0.09; height: parent.height; text: "%1%".arg(modelData.discount); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; font.weight: Theme.font.weightMedium; verticalAlignment: Text.AlignVCenter; leftPadding: Theme.space.sm }
                                Text { width: parent.parent.width * 0.11; height: parent.height; text: "%1 / %2".arg(modelData.uses).arg(modelData.cap); color: Theme.color.textSecondary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody; verticalAlignment: Text.AlignVCenter; leftPadding: Theme.space.sm }
                                Text { width: parent.parent.width * 0.18; height: parent.height; text: modelData.period || "—"; color: Theme.color.textSecondary; font.family: Theme.font.familyMono; font.pixelSize: Theme.font.sizeCaption; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; leftPadding: Theme.space.sm }
                                Item {
                                    width: parent.parent.width * 0.08
                                    height: parent.height
                                    Row {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.space.sm
                                        spacing: 6
                                        Rectangle { width: 6; height: 6; radius: 3; color: page._statusColor(modelData.status); anchors.verticalCenter: parent.verticalCenter }
                                        Text { text: page._statusLabel(modelData.status); color: Theme.color.textPrimary; font.family: Theme.font.family; font.pixelSize: Theme.font.sizeBody }
                                    }
                                }
                                Item {
                                    width: parent.parent.width * 0.05
                                    height: parent.height
                                    IconButton {
                                        iconName: "delete"
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: Theme.space.sm
                                        onClicked: {
                                            if (page.viewModel) page.viewModel.removePromotion(modelData.code)
                                            page.toastRequested("info", "Removed", "Promo code '" + modelData.code + "' has been removed.")
                                        }
                                    }
                                }
                            }
                            Rectangle { width: parent.width; height: 1; color: Theme.color.divider }
                        }
                    }

                    EmptyState {
                        width: parent.width
                        height: 200
                        visible: page._promoCount === 0
                        iconName: "local_offer"
                        title: "No promotions yet"
                        description: "Create your first promotion above to start driving sales."
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
