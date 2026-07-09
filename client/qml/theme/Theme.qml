// pragma Singleton
// =============================================================================
//  Theme.qml — Design System Singleton
// =============================================================================
//  Central definition of every design token used by the Authentication module
//  (and intended as the foundation for the rest of the BookClub client).
//
//  Design language:
//      • Minimal, monochrome core (black on white) + a single accent blue
//      • Soft single-layer elevation, large card radius, medium field radius
//      • Humanist sans-serif typography scale
//      • Calm, balanced spacing rhythm (8 px base grid)
// =============================================================================
pragma Singleton

import QtQuick 2.15

QtObject {
    // -------------------------------------------------------------------------
    //  Color palette
    // -------------------------------------------------------------------------
    readonly property var color: ({
        // Surfaces
        "pageBackground":   "#F4F5F7",
        "cardBackground":   "#FFFFFF",
        "heroGradientTop":  "#FFFFFF",
        "heroGradientBottom":"#ECEEF2",
        "scrim":            "rgba(15, 17, 21, 0.45)",

        // Brand
        "primary":          "#0A0A0B",   // near-black — headings, primary button
        "primaryHover":     "#2A2A2E",
        "primaryPressed":   "#000000",
        "onPrimary":        "#FFFFFF",

        // Accent (focus ring, links)
        "accent":           "#1A73E8",
        "accentHover":      "#1557B0",
        "accentSoft":       "#E8F0FE",

        // Text
        "textPrimary":      "#0A0A0B",
        "textSecondary":    "#5F6368",
        "textMuted":        "#9AA0A6",
        "textOnPrimary":    "#FFFFFF",
        "textOnAccent":     "#FFFFFF",
        "textInverse":      "#FFFFFF",

        // Borders / dividers
        "border":           "#E2E4E8",
        "borderStrong":     "#C8CBD1",
        "divider":          "#EAEAEC",

        // States
        "success":          "#1E8E3E",
        "successSoft":      "#E6F4EA",
        "warning":          "#F29900",
        "warningSoft":      "#FEF0E2",
        "error":            "#D93025",
        "errorSoft":        "#FCE8E6",
        "info":             "#1A73E8",
        "infoSoft":         "#E8F0FE",

        // Field
        "fieldBackground":  "#FFFFFF",
        "fieldFilled":      "#F7F8FA",
        "fieldDisabled":    "#F1F3F4"
    })

    // -------------------------------------------------------------------------
    //  Typography
    // -------------------------------------------------------------------------
    readonly property var font: ({
        "family":        "Inter, SF Pro Display, SF Pro Text, Segoe UI, Roboto, Helvetica Neue, Arial",
        "familyMono":    "JetBrains Mono, SF Mono, Menlo, Consolas, monospace",
        "familyIcon":    "Material Symbols Outlined",

        // Size scale (1.125 ratio)
        "sizeCaption":   12,
        "sizeSmall":     13,
        "sizeBody":      14,
        "sizeBodyLarge": 16,
        "sizeTitle":     18,
        "sizeHeadline":  22,
        "sizeDisplay":   28,
        "sizeHero":      34,

        // Weights
        "weightRegular": 400,
        "weightMedium":  500,
        "weightSemibold":600,
        "weightBold":    700,

        // Letter spacing
        "trackingTight":  -0.2,
        "trackingNormal": 0.0,
        "trackingWide":   0.6,
        "trackingXWide":  1.6
    })

    // -------------------------------------------------------------------------
    //  Spacing scale (8 px base grid)
    // -------------------------------------------------------------------------
    readonly property var space: ({
        "xxs": 2,
        "xs":  4,
        "sm":  8,
        "md":  12,
        "base":16,
        "lg":  20,
        "xl":  24,
        "xxl": 32,
        "xxxl":40,
        "huge":48,
        "mega":64
    })

    // -------------------------------------------------------------------------
    //  Radius scale
    // -------------------------------------------------------------------------
    readonly property var radius: ({
        "none":   0,
        "xs":     4,
        "sm":     6,
        "md":     8,
        "lg":     12,
        "xl":     16,
        "xxl":    24,
        "pill":   999
    })

    // -------------------------------------------------------------------------
    //  Elevation / shadows
    // -------------------------------------------------------------------------
    readonly property var shadow: ({
        // Each entry is a small descriptor consumed by the DropShadow component.
        "sm": { "color": "rgba(15, 17, 21, 0.06)", "blur": 8,  "offsetY": 2 },
        "md": { "color": "rgba(15, 17, 21, 0.08)", "blur": 16, "offsetY": 6 },
        "lg": { "color": "rgba(15, 17, 21, 0.10)", "blur": 28, "offsetY": 12 },
        "xl": { "color": "rgba(15, 17, 21, 0.14)", "blur": 44, "offsetY": 20 }
    })

    // -------------------------------------------------------------------------
    //  Sizing — fields, buttons, icons
    // -------------------------------------------------------------------------
    readonly property var size: ({
        "fieldHeight":     48,
        "buttonHeight":    48,
        "buttonHeightSm":  36,
        "iconSm":          16,
        "iconMd":          20,
        "iconLg":          24,
        "iconXl":          32,
        "logoSize":        56,
        "cardMaxWidth":    980,
        "formMaxWidth":    420
    })

    // -------------------------------------------------------------------------
    //  Motion
    // -------------------------------------------------------------------------
    readonly property var motion: ({
        "durationFast":    140,
        "durationBase":    220,
        "durationSlow":    340,
        "durationPage":    420,
        // Easing curves — uses standard QtQuick format
        "easeStandard":     "OutCubic",
        "easeEntrance":     "OutQuint",
        "easeExit":         "InQuad",
        "easePage":         "OutExpo"
    })

    // -------------------------------------------------------------------------
    //  Z-index layers
    // -------------------------------------------------------------------------
    readonly property var z: ({
        "base":      0,
        "card":      1,
        "sticky":    10,
        "drawer":    50,
        "modal":     100,
        "toast":     200,
        "tooltip":   300
    })
}
