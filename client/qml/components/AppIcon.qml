// =============================================================================
//  AppIcon.qml
// =============================================================================
//  Material Symbols Outlined icon glyph.
//
//  Usage:
//      AppIcon { name: "lock"; size: 20; color: Theme.color.textMuted }
//
//  The Material Symbols font is expected to be bundled under
//  client/resources/fonts/MaterialSymbolsOutlined-Regular.ttf and registered
//  via QFontDatabase in main.cpp (see fonts.qrc).
//
//  All glyph names below map to Material Symbols codepoints (Outlined axis).
//  See https://fonts.google.com/icons for the canonical reference.
// =============================================================================
import QtQuick 2.15
import "../theme"

Item {
    id: root

    // ----- Public API -----
    // Canonical Material Symbols name — see lookup table below.
    property string name: ""

    // Visual size of the glyph in pixels (square aspect).
    property int size: Theme.size.iconMd

    // Glyph color — defaults to current text color.
    property color color: Theme.color.textSecondary

    // Optical weight (100..700). 300 = light, 400 = regular, 500 = medium.
    property int weight: Font.Normal

    // Filled variant toggle (Material Symbols "Fill" axis, 0..1).
    property bool filled: false

    implicitWidth: size
    implicitHeight: size

    // ----- Glyph rendering -----
    Text {
        id: glyph
        anchors.centerIn: parent
        width: root.size
        height: root.size
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: Theme.font.familyIcon
        font.pixelSize: root.size
        font.weight: root.weight
        color: root.color
        text: root.name.length > 0 ? _iconTable[root.name] || "" : ""
    }

    // ----- Icon codepoint lookup table (Material Symbols Outlined) -----
    // Source: https://github.com/google/material-design-icons
    readonly property var _iconTable: ({
        // ---- Auth / identity ----
        "person":              "\uE7FD",
        "person_outline":      "\uE7FF",
        "badge":               "\uEA67",
        "lock":                "\uE897",
        "lock_outline":        "\uE898",
        "lock_open":           "\uE899",
        "key":                 "\uE73C",
        "shield":              "\uE9F0",
        "verified_user":       "\uE8E8",
        "verified":            "\uEF76",
        "login":               "\uEA77",
        "logout":              "\uE9BA",
        "how_to_reg":          "\uE77D",

        // ---- Visibility ----
        "visibility":          "\uE8F4",
        "visibility_off":      "\uE8F5",

        // ---- Navigation ----
        "arrow_back":          "\uE5CB",
        "arrow_forward":       "\uE5CC",
        "arrow_right_alt":     "\uE5DB",
        "chevron_right":       "\uE5CC",
        "chevron_left":        "\uE5CB",
        "close":               "\uE5CD",
        "check":               "\uE5CA",
        "check_circle":        "\uE86C",
        "cancel":              "\uE5C9",
        "radio_button_unchecked": "\uE836",

        // ---- Feedback ----
        "error":               "\uE000",
        "error_outline":       "\uE001",
        "warning":             "\uE002",
        "warning_amber":       "\uE8CE",
        "info":                "\uE88F",
        "info_outline":        "\uE88E",
        "task_alt":            "\uE735",

        // ---- Progress ----
        "refresh":             "\uE5D5",
        "autorenew":           "\uE862",
        "hourglass_empty":     "\uE88B",
        "progress_activity":   "\uE876",

        // ---- Search ----
        "search":              "\uE8B6",
        "search_off":          "\uEA76",
        "filter_list":         "\uE3E3",

        // ---- Misc UI ----
        "mail":                "\uE158",
        "phone":               "\uE0CD",
        "calendar_today":      "\uE935",
        "menu_book":           "\uEA19",
        "auto_stories":        "\uE80C",
        "book":                "\uE865",
        "bookmark":            "\uE866",
        "favorite":            "\uE87D",
        "favorite_border":     "\uE87E",
        "star":                "\uE838",
        "star_outline":        "\uE83A",

        // ---- BookClub brand ----
        "menu":                "\uE5D2",
        "settings":            "\uE8B8",
        "dark_mode":           "\uE51C",
        "light_mode":          "\uE518",
        "language":            "\uE894",
        "palette":             "\uE40A",

        // ---- Toggle / selection ----
        "expand_more":         "\uE5CF",
        "expand_less":         "\uE5CE",
        "unfold_more":         "\uE88D",
        "drag_handle":         "\uE25D",

        // ---- Security questions / OTP ----
        "quiz":                "\uEA66",
        "question_mark":       "\uEA09",
        "password":            "\uF903",
        "pin":                 "\uE90E",
        "tag":                 "\uE89F"
    })
}
