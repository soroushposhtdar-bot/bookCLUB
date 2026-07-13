// =============================================================================
//  Theme.qml — Design System Singleton (extended for the User dashboard)
// =============================================================================
//  Central definition of every design token used across the BookClub client.
//
//  Design language:
//      • Minimal, monochrome core (black on white) + a single accent blue
//      • Soft single-layer elevation, large card radius, medium field radius
//      • Humanist sans-serif typography scale
//      • Calm, balanced spacing rhythm (8 px base grid)
//
//  v2 additions (User dashboard module):
//      • Dark-mode palette — switch via the `mode` property (read by every
//        component through the `effectiveColor()` helper).
//      • Dashboard-specific tokens: sidebar width, topbar height, book-card
//        metrics, navigation-rail spacing.
//      • Helper API: c(roleKey) → resolves the correct color for the active
//        mode, so components do not need to branch on `mode` themselves.
// =============================================================================
pragma Singleton

import QtQuick 2.15

QtObject {
    // -------------------------------------------------------------------------
    //  Active color mode — "light" | "dark"
    // -------------------------------------------------------------------------
    //  Flipping this property cascades to every component that reads colors
    //  through `Theme.c("roleKey")`. Components that still read the legacy
    //  `Theme.color.xxx` shortcuts below keep working — those shortcuts are
    //  re-bound to the active mode.
    // -------------------------------------------------------------------------
    property string mode: "light"

    readonly property bool isDark: mode === "dark"

    // -------------------------------------------------------------------------
    //  Light palette (default — matches the original auth module exactly)
    // -------------------------------------------------------------------------
    readonly property var light: ({
        "pageBackground":    "#F4F5F7",
        "cardBackground":    "#FFFFFF",
        "heroGradientTop":   "#FFFFFF",
        "heroGradientBottom":"#ECEEF2",
        "scrim":             "rgba(15, 17, 21, 0.45)",

        "primary":           "#0A0A0B",
        "primaryHover":      "#2A2A2E",
        "primaryPressed":    "#000000",
        "onPrimary":         "#FFFFFF",

        "accent":            "#1A73E8",
        "accentHover":       "#1557B0",
        "accentSoft":        "#E8F0FE",

        "textPrimary":       "#0A0A0B",
        "textSecondary":     "#5F6368",
        "textMuted":         "#9AA0A6",
        "textOnPrimary":     "#FFFFFF",
        "textOnAccent":      "#FFFFFF",
        "textInverse":       "#FFFFFF",

        "border":            "#E2E4E8",
        "borderStrong":      "#C8CBD1",
        "divider":           "#EAEAEC",

        "success":           "#1E8E3E",
        "successSoft":       "#E6F4EA",
        "warning":           "#F29900",
        "warningSoft":       "#FEF0E2",
        "error":             "#D93025",
        "errorSoft":         "#FCE8E6",
        "info":              "#1A73E8",
        "infoSoft":          "#E8F0FE",

        "fieldBackground":   "#FFFFFF",
        "fieldFilled":       "#F7F8FA",
        "fieldDisabled":     "#F1F3F4",

        // Dashboard-only additions
        "sidebarBackground": "#FFFFFF",
        "sidebarItemHover":  "#F1F3F4",
        "sidebarItemActive": "#E8F0FE",
        "sidebarItemActiveFg":"#0A0A0B",
        "topbarBackground":  "#FFFFFF",
        "overlayScrim":      "rgba(15, 17, 21, 0.55)"
    })

    // -------------------------------------------------------------------------
    //  Dark palette — same role names, tuned for OLED-friendly dark surfaces.
    //  The accent blue stays consistent so the brand reads the same in both
    //  modes.
    // -------------------------------------------------------------------------
    readonly property var dark: ({
        "pageBackground":    "#0E0F11",
        "cardBackground":    "#17181B",
        "heroGradientTop":   "#1B1C20",
        "heroGradientBottom":"#0E0F11",
        "scrim":             "rgba(0, 0, 0, 0.65)",

        "primary":           "#F4F5F7",
        "primaryHover":      "#FFFFFF",
        "primaryPressed":    "#E2E4E8",
        "onPrimary":         "#0A0A0B",

        "accent":            "#8AB4F8",
        "accentHover":       "#A8C7FA",
        "accentSoft":        "rgba(138, 180, 248, 0.14)",

        "textPrimary":       "#F4F5F7",
        "textSecondary":     "#B6BAC2",
        "textMuted":         "#7A7F88",
        "textOnPrimary":     "#0A0A0B",
        "textOnAccent":      "#0A0A0B",
        "textInverse":       "#0A0A0B",

        "border":            "#2A2C31",
        "borderStrong":      "#3C3F46",
        "divider":           "#222327",

        "success":           "#81C995",
        "successSoft":       "rgba(129, 201, 149, 0.14)",
        "warning":           "#FCD663",
        "warningSoft":       "rgba(252, 214, 99, 0.14)",
        "error":             "#F28B82",
        "errorSoft":         "rgba(242, 139, 130, 0.14)",
        "info":              "#8AB4F8",
        "infoSoft":          "rgba(138, 180, 248, 0.14)",

        "fieldBackground":   "#1B1C20",
        "fieldFilled":       "#1F2024",
        "fieldDisabled":     "#1B1C20",

        "sidebarBackground": "#0E0F11",
        "sidebarItemHover":  "#1F2024",
        "sidebarItemActive": "rgba(138, 180, 248, 0.16)",
        "sidebarItemActiveFg":"#F4F5F7",
        "topbarBackground":  "#0E0F11",
        "overlayScrim":      "rgba(0, 0, 0, 0.70)"
    })

    // -------------------------------------------------------------------------
    //  Color access
    // -------------------------------------------------------------------------
    //  • `Theme.color.xxx` — backward-compatible object that re-points to the
    //    active palette. Existing auth components keep working unchanged.
    //  • `Theme.c("roleKey")` — explicit accessor; preferred for new code.
    // -------------------------------------------------------------------------
    readonly property var color: isDark ? dark : light

    function c(key) {
        return isDark ? dark[key] : light[key]
    }

    // -------------------------------------------------------------------------
    //  Typography
    // -------------------------------------------------------------------------
    readonly property var font: ({
        "family":        "Inter, SF Pro Display, SF Pro Text, Segoe UI, Roboto, Helvetica Neue, Arial",
        "familyMono":    "JetBrains Mono, SF Mono, Menlo, Consolas, monospace",
        "familyIcon":    "Material Symbols Outlined",

        // Size scale (1.125 ratio)
        "sizeMicro":     10,
        "sizeMicro2":    11,
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
    //  In dark mode the shadow color is slightly stronger to read against the
    //  dark surface.
    // -------------------------------------------------------------------------
    readonly property var shadow: ({
        "sm": { "color": isDark ? "rgba(0, 0, 0, 0.40)" : "rgba(15, 17, 21, 0.06)", "blur": 8,  "offsetY": 2 },
        "md": { "color": isDark ? "rgba(0, 0, 0, 0.45)" : "rgba(15, 17, 21, 0.08)", "blur": 16, "offsetY": 6 },
        "lg": { "color": isDark ? "rgba(0, 0, 0, 0.55)" : "rgba(15, 17, 21, 0.10)", "blur": 28, "offsetY": 12 },
        "xl": { "color": isDark ? "rgba(0, 0, 0, 0.65)" : "rgba(15, 17, 21, 0.14)", "blur": 44, "offsetY": 20 }
    })

    // -------------------------------------------------------------------------
    //  Sizing — fields, buttons, icons, dashboard chrome
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
        "formMaxWidth":    420,

        // Dashboard chrome
        "sidebarWidth":        248,
        "sidebarCollapsedWidth":72,
        "topbarHeight":        64,
        "contentMaxWidth":    1280,
        "bookCardWidth":      188,
        "bookCardHeight":     296,
        "bookCoverRatio":     1.5,     // height / width
        "avatarSize":         40,
        "avatarSizeSm":       32,
        "navItemHeight":      44
    })

    // -------------------------------------------------------------------------
    //  Motion — extended for production-grade micro-interactions
    // -------------------------------------------------------------------------
    readonly property var motion: ({
        // Duration scale (ms)
        "durationInstant":  80,
        "durationFast":     140,
        "durationBase":     220,
        "durationSlow":     340,
        "durationPage":     420,
        "durationHero":     560,
        // Easing curves (QtQuick Easing.Type names)
        "easeStandard":     "OutCubic",
        "easeEntrance":     "OutQuint",
        "easeExit":         "InQuad",
        "easePage":         "OutExpo",
        "easeSpring":       "OutBack",
        "easeOvershoot":    "OutElastic",
        // Spring params for SmoothedAnimation / SpringAnimation
        "spring":           { "stiffness": 250, "damping": 24, "mass": 0.8 },
        // Ripple
        "rippleDuration":   560,
        "rippleMaxOpacity": 0.18,
        // Stagger delays for list/grid entrance animations
        "staggerStep":      40
    })

    // -------------------------------------------------------------------------
    //  Ripple — touch / click feedback
    // -------------------------------------------------------------------------
    readonly property var ripple: ({
        "color":        isDark ? "#FFFFFF" : "#000000",
        "maxOpacity":   motion.rippleMaxOpacity,
        "duration":     motion.rippleDuration,
        "ease":         "OutCubic"
    })

    // -------------------------------------------------------------------------
    //  Skeleton — shimmer placeholders for loading states
    // -------------------------------------------------------------------------
    readonly property var skeleton: ({
        "base":         isDark ? "#1F2024" : "#ECEEF2",
        "highlight":    isDark ? "#2A2C31" : "#F4F5F7",
        "shimmerDuration": 1400,
        "shimmerWidth": 0.4     // fraction of element width the shimmer band occupies
    })

    // -------------------------------------------------------------------------
    //  Elevation scale — expanded for production depth cues
    // -------------------------------------------------------------------------
    readonly property var elevation: ({
        // Each level: { color, blur, offsetY, opacity }
        "0": { "color": "transparent",                  "blur": 0,  "offsetY": 0,  "opacity": 0    },
        "1": { "color": isDark ? "rgba(0,0,0,0.40)" : "rgba(15,17,21,0.04)", "blur": 4,  "offsetY": 1, "opacity": 1 },
        "2": { "color": isDark ? "rgba(0,0,0,0.42)" : "rgba(15,17,21,0.06)", "blur": 8,  "offsetY": 2, "opacity": 1 },
        "3": { "color": isDark ? "rgba(0,0,0,0.45)" : "rgba(15,17,21,0.08)", "blur": 16, "offsetY": 6, "opacity": 1 },
        "4": { "color": isDark ? "rgba(0,0,0,0.55)" : "rgba(15,17,21,0.10)", "blur": 28, "offsetY": 12,"opacity": 1 },
        "5": { "color": isDark ? "rgba(0,0,0,0.65)" : "rgba(15,17,21,0.14)", "blur": 44, "offsetY": 20,"opacity": 1 }
    })

    // -------------------------------------------------------------------------
    //  Accent palette — switchable accent colors for the user's preference.
    //  `accent` (above) is the active one; `accentPalette` is the chooser grid.
    // -------------------------------------------------------------------------
    readonly property string accentName: "blue"
    readonly property var accentPalette: ([
        { "name": "blue",    "color": "#1A73E8", "soft": "#E8F0FE" },
        { "name": "indigo",  "color": "#3F51B5", "soft": "#E8EAF6" },
        { "name": "purple",  "color": "#7C4DFF", "soft": "#EDE7F6" },
        { "name": "teal",    "color": "#00897B", "soft": "#E0F2F1" },
        { "name": "pink",    "color": "#D81B60", "soft": "#FCE4EC" },
        { "name": "orange",  "color": "#E65100", "soft": "#FFF3E0" },
        { "name": "green",   "color": "#1E8E3E", "soft": "#E6F4EA" },
        { "name": "graphite","color": "#0A0A0B", "soft": "#E2E4E8" }
    ])

    // -------------------------------------------------------------------------
    //  Typography refinements — line-heights + max-line-count presets
    // -------------------------------------------------------------------------
    readonly property var lineHeight: ({
        "tight":    1.15,
        "snug":     1.3,
        "normal":   1.5,
        "relaxed":  1.65,
        "loose":    1.8
    })

    // -------------------------------------------------------------------------
    //  Animation presets — reusable Transition objects for common UI moves
    // -------------------------------------------------------------------------
    readonly property QtObject anim: QtObject {
        // Page push (slide + fade)
        readonly property Transition pagePush: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: 60; to: 0; duration: motion.durationPage; easing.type: Easing.OutExpo }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: motion.durationPage; easing.type: Easing.OutCubic }
            }
        }
        // Card entrance (subtle rise + fade)
        readonly property Transition cardEntrance: Transition {
            ParallelAnimation {
                NumberAnimation { property: "y"; from: 12; to: 0; duration: motion.durationSlow; easing.type: Easing.OutQuint }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: motion.durationBase; easing.type: Easing.OutCubic }
            }
        }
        // Fade-only
        readonly property Transition fade: Transition {
            NumberAnimation { property: "opacity"; duration: motion.durationBase; easing.type: Easing.OutCubic }
        }
        // Scale-in for popups
        readonly property Transition scaleIn: Transition {
            ParallelAnimation {
                NumberAnimation { property: "scale"; from: 0.92; to: 1; duration: motion.durationBase; easing.type: Easing.OutBack }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: motion.durationFast; easing.type: Easing.OutCubic }
            }
        }
    }

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
        "tooltip":   300,
        "contextMenu": 250
    })

    // -------------------------------------------------------------------------
    //  Extended typography — sizeMega for hero price/rating displays
    // -------------------------------------------------------------------------
    readonly property int sizeMega: 56
}
