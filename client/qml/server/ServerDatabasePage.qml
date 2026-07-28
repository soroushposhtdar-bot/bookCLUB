// QT6 MIGRATION CHANGES:
//   - imports unversioned (QtQuick / .Controls / .Layouts)
//
// FUNCTIONAL FIXES:
//   - Added defensive null checks in helper functions for missing viewModel data
//   - All KPIs + list views bind directly to viewModel properties, so they
//     re-evaluate automatically when NOTIFY signals fire
//
// LAYOUT FIXES:
//   - FIX 3-A: Added top spacer + switched KPI row to RowLayout
//   - FIX 3-B: Tables + pool side-by-side Row → RowLayout with
//     Layout.fillWidth + Layout.preferredWidth: 50; inner Columns use
//     width-only (NOT anchors.fill); ListView heights are content-driven
//   - FIX 3-C: Slow queries Card inner Column: anchors.fill → width-only;
//     ListView height content-driven; added EmptyState when slowQueries empty

// =============================================================================
//  ServerDatabasePage.qml
// =============================================================================
//  Database health: 4 KPI cards, a two-column row with table sizes + a
//  connection-pool grid (connections rendered as colored squares), and a
//  "Recent slow queries" panel using monospace text for query bodies. Sourced
//  from ServerViewModel.databaseTables, .connectionPool, .slowQueries.
// =============================================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components/data"
import "../components/surfaces"
import "../components/buttons"
import "../components/progress"
import "../components/inputs"
import "../components/navigation"
import "../components/feedback"

import BookClub.Services 1.0

Item {
    id: page

    property var viewModel: null   // ServerViewModel

    signal toastRequested(string variant, string title, string description)

    function _poolColor(state) {
        if (state === "active") return Theme.color.success
        if (state === "idle")   return Theme.color.warning
        return Theme.color.error
    }

    // Server sends { name, rows } — no sizeMb. We compute an estimated
    // size: 1 row ≈ 1 KB. This gives a meaningful display without mock data.
    function _rowsToMb(rows) { return (rows * 0.001) }   // 1 row ≈ 1 KB

    function _totalSizeMb() {
        var tables = page.viewModel ? page.viewModel.databaseTables : []
        var sum = 0
        for (var i = 0; i < tables.length; ++i) {
            var t = tables[i]
            sum += t.sizeMb !== undefined ? t.sizeMb : page._rowsToMb(t.rows || 0)
        }
        return sum
    }

    function _getSizeMb(t) {
        if (!t) return 0
        return t.sizeMb !== undefined ? t.sizeMb : page._rowsToMb(t.rows || 0)
    }

    function _formatSize(sizeMb) {
        if (isNaN(sizeMb) || sizeMb === undefined) return "— MB"
        if (sizeMb >= 1024) return (sizeMb / 1024).toFixed(2) + " GB"
        if (sizeMb < 0.1)   return (sizeMb * 1024).toFixed(0) + " KB"
        return sizeMb.toFixed(2) + " MB"
    }

    function _tablePct(sizeMb) {
        var total = page._totalSizeMb()
        if (total <= 0) return "0.0"
        return (sizeMb / total * 100).toFixed(1)
    }

    function _poolCount(state) {
        var pool = page.viewModel ? page.viewModel.connectionPool : []
        var n = 0
        for (var i = 0; i < pool.length; ++i) {
            if (pool[i].state === state) n++
        }
        return n
    }

    function _avgQueryMs() {
        var queries = page.viewModel ? page.viewModel.slowQueries : []
        if (queries.length === 0) return "—"
        var sum = 0
        for (var i = 0; i < queries.length; ++i) {
            // duration may be a string like "284ms" or a number; coerce to number.
            var d = queries[i].duration
            if (typeof d === "string") d = parseFloat(d)
            if (!isNaN(d)) sum += d
        }
        if (sum === 0) return "—"
        return (sum / queries.length).toFixed(1) + "ms"
    }

    // All KPIs and list views below bind directly to the viewModel's
    // QVariantList properties, so they re-evaluate automatically when those
    // properties emit their Changed signals. No explicit Connections handlers
    // are required here.

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Column {
            width: parent.width
            spacing: Theme.space.xl

            // FIX 3-A — Top breathing room
            Item { width: 1; height: Theme.space.xl }

            // ----- KPI cards (FIX 3-A: RowLayout) -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "storage"
                    value:    page._formatSize(page._totalSizeMb())
                    label:    "DB size"
                    delta:    "%1 tables".arg(page.viewModel ? page.viewModel.databaseTables.length : 0)
                    deltaUp:  true
                    accent:   Theme.color.accent
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "dns"
                    value:    page.viewModel ? page.viewModel.connectionPool.length : 0
                    label:    "Active connections"
                    delta:    "%1 idle / %2 active".arg(page._poolCount("idle")).arg(page._poolCount("active"))
                    deltaUp:  true
                    accent:   Theme.color.info
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "speed"
                    value:    (page.viewModel ? page.viewModel.dbQueryRate : 0) + "/min"
                    label:    "Query rate"
                    delta:    "live"
                    deltaUp:  true
                    accent:   Theme.color.success
                }
                StatCard {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    height: Theme.size.kpiCardHeight
                    iconName: "hourglass_empty"
                    value:    page._avgQueryMs()
                    label:    "Avg query time"
                    delta:    "from slow log"
                    deltaUp:  true
                    accent:   Theme.color.warning
                }
            }

            // ----- Tables + pool (FIX 3-B: RowLayout) -----
            RowLayout {
                width: parent.width
                spacing: Theme.space.lg

                // Table sizes
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50
                    padding: Theme.space.xl

                    // FIX 3-B — width-only (NOT anchors.fill)
                    Column {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Table sizes"
                            subtitle: "Top %1 tables by storage".arg(page.viewModel ? page.viewModel.databaseTables.length : 0)
                        }

                        Row {
                            width: parent.width
                            Repeater {
                                model: [
                                    { w: 0.30, label: "Table" },
                                    { w: 0.25, label: "Rows" },
                                    { w: 0.25, label: "Size" },
                                    { w: 0.20, label: "% of total" }
                                ]
                                Text {
                                    width: parent.parent.width * modelData.w
                                    text: modelData.label
                                    color: Theme.color.textMuted
                                    font.family: Theme.font.family
                                    font.pixelSize: Theme.font.sizeCaption
                                    font.weight: Theme.font.weightBold
                                }
                            }
                        }
                        Rectangle { width: parent.width; height: 1; color: Theme.color.divider }

                        ListView {
                            width: parent.width
                            // FIX 3-B — content-driven height
                            height: Math.max(
                                (page.viewModel ? page.viewModel.databaseTables.length : 0) === 0 ? 160 : 0,
                                (page.viewModel ? page.viewModel.databaseTables.length : 0) * 44
                            )
                            clip: true
                            interactive: false
                            spacing: 0
                            model: page.viewModel ? page.viewModel.databaseTables : []

                            delegate: Column {
                                width: parent.width
                                height: 44
                                Row {
                                    width: parent.width
                                    height: 44
                                    Text {
                                        width: parent.width * 0.30
                                        text: modelData.name
                                        color: Theme.color.textPrimary
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        width: parent.width * 0.25
                                        text: modelData.rows.toLocaleString(Qt.locale("en_US"))
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        width: parent.width * 0.25
                                        text: page._formatSize(page._getSizeMb(modelData))
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        width: parent.width * 0.20
                                        text: "%1%".arg(page._tablePct(page._getSizeMb(modelData)))
                                        color: Theme.color.accent
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                Rectangle { width: parent.width; height: 1; color: Theme.color.divider }
                            }
                        }
                    }
                }

                // Connection pool
                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 50
                    padding: Theme.space.xl

                    // FIX 3-B — width-only (NOT anchors.fill)
                    Column {
                        width: parent.width
                        spacing: Theme.space.md

                        SectionHeader {
                            width: parent.width
                            title: "Connection pool"
                            subtitle: "%1 of 64 max connections in use".arg(page.viewModel ? page.viewModel.connectionPool.length : 0)
                        }

                        // Grid 8x4
                        Grid {
                            width: parent.width
                            columns: 8
                            spacing: Theme.space.sm

                            Repeater {
                                model: page.viewModel ? page.viewModel.connectionPool : []
                                Rectangle {
                                    width: (parent.width - 7 * Theme.space.sm) / 8
                                    height: 32
                                    radius: Theme.radius.sm
                                    color: page._poolColor(modelData.state)
                                    border.color: Qt.rgba(Theme.color.divider.r, Theme.color.divider.g, Theme.color.divider.b, 0.5)
                                    border.width: 1
                                }
                            }
                        }

                        // Legend
                        Row {
                            width: parent.width
                            spacing: Theme.space.lg
                            Repeater {
                                model: [
                                    { label: "Active", color: Theme.color.success },
                                    { label: "Idle",   color: Theme.color.warning },
                                    { label: "Slow",   color: Theme.color.error   }
                                ]
                                Row {
                                    spacing: Theme.space.xs
                                    Rectangle { width: 8; height: 8; radius: 4; color: modelData.color; anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        text: modelData.label
                                        color: Theme.color.textSecondary
                                        font.family: Theme.font.family
                                        font.pixelSize: Theme.font.sizeCaption
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ----- Slow queries (FIX 3-C) -----
            Card {
                width: parent.width
                padding: Theme.space.xl

                // FIX 3-C — width-only (NOT anchors.fill)
                Column {
                    width: parent.width
                    spacing: Theme.space.md

                    SectionHeader {
                        width: parent.width
                        title: "Recent slow queries (>100ms)"
                        subtitle: "%1 queries that exceeded the latency budget".arg(page.viewModel ? page.viewModel.slowQueries.length : 0)
                    }

                    // FIX 3-C — EmptyState when no slow queries
                    EmptyState {
                        width: parent.width
                        height: (page.viewModel ? page.viewModel.slowQueries.length : 0) === 0 ? 120 : 0
                        visible: (page.viewModel ? page.viewModel.slowQueries.length : 0) === 0
                        iconName: "hourglass_empty"
                        title: "No slow queries"
                        description: "0 queries that exceeded the latency budget."
                    }

                    ListView {
                        width: parent.width
                        // FIX 3-C — content-driven height
                        height: Math.max(
                            (page.viewModel ? page.viewModel.slowQueries.length : 0) === 0 ? 120 : 0,
                            (page.viewModel ? page.viewModel.slowQueries.length : 0) * 72
                        )
                        clip: true
                        interactive: false
                        spacing: Theme.space.sm
                        model: page.viewModel ? page.viewModel.slowQueries : []
                        visible: (page.viewModel ? page.viewModel.slowQueries.length : 0) > 0

                        delegate: Row {
                            width: parent.width
                            spacing: Theme.space.md

                            Rectangle {
                                width: 4; height: 40
                                color: Theme.color.error
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Column {
                                width: parent.width - 4 - Theme.space.md
                                spacing: 2

                                Text {
                                    width: parent.width
                                    text: modelData.query
                                    color: Theme.color.textPrimary
                                    font.family: Theme.font.familyMono
                                    font.pixelSize: Theme.font.sizeCaption
                                    elide: Text.ElideRight
                                }
                                Row {
                                    width: parent.width
                                    spacing: Theme.space.md
                                    Text {
                                        text: modelData.duration
                                        color: Theme.color.error
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                        font.weight: Theme.font.weightBold
                                    }
                                    Text {
                                        text: "table: %1".arg(modelData.table)
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                    Item { width: 1; Layout.fillWidth: true; height: 1 }
                                    Text {
                                        text: modelData.time
                                        color: Theme.color.textMuted
                                        font.family: Theme.font.familyMono
                                        font.pixelSize: Theme.font.sizeCaption
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Item { width: 1; height: Theme.space.xxl }
        }
    }
}
