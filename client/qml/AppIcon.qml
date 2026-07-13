// =============================================================================
//  AppIcon.qml
// =============================================================================
//  Material Symbols Outlined icon glyph.
//
//  Usage:
//      AppIcon { name: "lock"; size: 20; color: Theme.color.textMuted }
//
//  The Material Symbols font is bundled under
//  client/resources/fonts/MaterialSymbolsOutlined-Regular.ttf and registered
//  via QFontDatabase in main.cpp (see fonts.qrc).
//
//  All glyph names below map to Material Symbols Outlined codepoints.
//  See https://fonts.google.com/icons for the canonical reference.
// =============================================================================
import QtQuick 2.15
import "../theme"

Item {
    id: root

    // ----- Public API -----
    property string name: ""
    property int size: Theme.size.iconMd
    property color color: Theme.color.textSecondary
    property int weight: Font.Normal
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
    // Single canonical entry per name (no duplicates).
    readonly property var _iconTable: ({
        // ---- Auth / identity ----
        "person":              "\uE7FD",
        "person_outline":      "\uE7FF",
        "person_filled":       "\uE7FD",
        "person_add":          "\uE854",
        "sticky_note_2":       "\uE8FC",
        "people":              "\uEA21",
        "group":               "\uE7EF",
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
        "arrow_upward":        "\uE5D8",
        "arrow_downward":      "\uE5DB",
        "chevron_right":       "\uE5CC",
        "chevron_left":        "\uE5CB",
        "close":               "\uE5CD",
        "check":               "\uE5CA",
        "check_circle":        "\uE86C",
        "add_circle":          "\uE148",
        "play_arrow":          "\uE037",
        "pause":               "\uE034",
        "block":               "\uE14B",
        "check_box":           "\uE834",
        "cancel":              "\uE5C9",
        "radio_button_unchecked": "\uE836",
        "card_membership":     "\uE8F1",
        "delete_forever":      "\uE92B",
        "event":               "\uE8C9",

        // ---- Feedback ----
        "error":               "\uE000",
        "error_outline":       "\uE001",
        "warning":             "\uE002",
        "warning_amber":       "\uE8CE",
        "info":                "\uE88F",
        "info_outline":        "\uE88E",
        "task_alt":            "\uE735",
        "help":                "\uE8FD",
        "help_outline":        "\uE8FD",
        "feedback":            "\uE87F",
        "description":         "\uE873",

        // ---- Progress ----
        "refresh":             "\uE5D5",
        "autorenew":           "\uE862",
        "hourglass_empty":     "\uE88B",
        "progress_activity":   "\uE876",
        "update":              "\uE923",
        "sync":                "\uE627",
        "sync_problem":        "\uE629",

        // ---- Search ----
        "search":              "\uE8B6",
        "search_off":          "\uEA76",
        "filter_list":         "\uE3E3",
        "filter_alt":          "\uE4EF",
        "filter_list_off":     "\uEB53",
        "sort":                "\uE5D2",
        "tune":                "\uE429",

        // ---- BookClub brand ----
        "menu":                "\uE5D2",
        "menu_open":           "\uE9BD",
        "settings":            "\uE8B8",
        "dark_mode":           "\uE51C",
        "light_mode":          "\uE518",
        "language":            "\uE894",
        "public":              "\uE80B",
        "palette":             "\uE40A",
        "share":               "\uE80D",

        // ---- Toggle / selection ----
        "expand_more":         "\uE5CF",
        "expand_less":         "\uE5CE",
        "unfold_more":         "\uE88D",
        "drag_handle":         "\uE25D",
        "drag_indicator":      "\uE25D",

        // ---- Security questions / OTP ----
        "quiz":                "\uEA66",
        "question_mark":       "\uEA09",
        "password":            "\uF903",
        "pin":                 "\uE90E",
        "tag":                 "\uE89F",

        // ---- Dashboard navigation ----
        "home":                "\uE88A",
        "home_filled":         "\uE88B",
        "explore":             "\uE87A",
        "library_books":       "\uE8F1",
        "bookmarks":           "\uE866",
        "shopping_cart":       "\uE8CC",
        "shopping_bag":        "\uE8CC",
        "notifications":       "\uE7F4",
        "account_circle":      "\uE853",

        // ---- Book / reading ----
        "auto_stories":        "\uE80C",
        "menu_book":           "\uEA19",
        "book":                "\uE865",
        "bookmark":            "\uE866",
        "bookmark_border":     "\uE867",
        "favorite":            "\uE87D",
        "favorite_border":     "\uE87E",
        "star":                "\uE838",
        "star_half":           "\uE839",
        "star_outline":        "\uE83A",
        "rate_review":         "\uE860",
        "reviews":             "\uE860",
        "thumb_up":            "\uE8DC",
        "thumb_up_outlined":   "\uE8DB",
        "thumb_down":          "\uE8DB",
        "thumb_down_outlined": "\uE8DA",
        "reply":               "\uE15E",
        "flag":                "\uE153",
        "report":              "\uE160",

        // ---- Cart / commerce ----
        "add_shopping_cart":   "\uE854",
        "remove_shopping_cart":"\uE8EF",
        "delete":              "\uE872",
        "delete_outline":      "\uE92B",
        "remove":              "\uE15B",
        "add":                 "\uE145",
        "checkout":            "\uE876",
        "payments":            "\uEF67",
        "sell":                "\uE942",
        "sell_outlined":       "\uE942",
        "local_offer":         "\uE54E",
        "percent":             "\uEB58",
        "attach_money":        "\uE227",
        "savings":             "\uE2EB",
        "trending_up":         "\uE8E5",
        "trending_down":       "\uE8E3",
        "whatshot":            "\uE80E",
        "new_releases":        "\uE031",

        // ---- Reader controls ----
        "first_page":          "\uE5DC",
        "last_page":           "\uE5DD",
        "zoom_in":             "\uE8FF",
        "zoom_out":            "\uE900",
        "fullscreen":          "\uE5D0",
        "fullscreen_exit":     "\uE5D1",
        "fit_screen":          "\uE5CE",
        "read_more":           "\uEF6D",
        "format_align_justify":"\uE264",
        "contrast":            "\uEAB1",

        // ---- Shelves / organization ----
        "shelves":             "\uEFDB",
        "create_new_folder":   "\uE2CC",
        "folder":              "\uE2C7",
        "folder_open":         "\uE2C8",
        "edit":                "\uE254",
        "edit_note":           "\uE66E",
        "more_horiz":          "\uE5D3",
        "more_vert":           "\uE5D4",

        // ---- Connectivity / system ----
        "wifi":                "\uE63E",
        "wifi_off":            "\uE648",
        "cloud":               "\uE2BD",
        "cloud_off":           "\uE2C0",
        "storage":             "\uE1DB",
        "database":            "\uE1DB",
        "dns":                 "\uE8E9",
        "server":              "\uE8E9",
        "terminal":            "\uE8C0",
        "code":                "\uE86F",
        "engineering":         "\uE943",
        "monitor_heart":       "\uE61F",
        "memory":              "\uE322",
        "speed":               "\uE9E4",
        "animation":           "\uE71C",
        "history":             "\uE88B",
        "today":               "\uE8DF",
        "schedule":            "\uE8B5",
        "calendar_today":      "\uE935",
        "security":            "\uE8B8",

        // ---- Misc UI ----
        "mail":                "\uE158",
        "phone":               "\uE0CD",
        "campaign":            "\uEF49",
        "send":                "\uE163",
        "archive":             "\uE149",
        "unarchive":           "\uE172",
        "inbox":               "\uE156",
        "mark_email_read":     "\uE876",
        "mark_email_unread":   "\uE859",
        "mark_chat_read":      "\uE876",
        "mark_chat_unread":    "\uE859",
        "content_copy":        "\uE14D",
        "filter_none":         "\uE3E4",
        "done_all":            "\uE930",
        "download":            "\uE2BC",
        "download_done":       "\uE2C1",
        "upload":              "\uE2C3",
        "view_module":         "\uE8B2",
        "view_list":           "\uE8EF",
        "grid_view":           "\uE8B0",

        // ---- Publisher / admin / role UI ----
        "dashboard":           "\uE871",
        "analytics":           "\uE619",
        "insights":            "\uE619",
        "supervisor_account":  "\uE8EC",
        "business":            "\uE272",
        "domain":              "\uE7FF",
        "work":                "\uE943",
        "gavel":               "\uE90E",
        "policy":              "\uEA17",
        "moderation":          "\uE90E",
        "manage_accounts":     "\uE8F1",
        "admin_panel_settings":"\uE8F2",
        "groups":              "\uE234",
        "school":              "\uE80C",
        "category":            "\uE574",
        "label":               "\uE89F",
        "inventory":           "\uE179",
        "production_quantity_limits": "\uE1D1",
        "request_quote":       "\uE1B6",
        "leaderboard":         "\uE90C",
        "stacked_bar_chart":   "\uE8C2",
        "pie_chart":           "\uE6C4",
        "bar_chart":           "\uE26B",
        "show_chart":          "\uE6E1"
    })
}
