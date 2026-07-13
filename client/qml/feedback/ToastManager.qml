// =============================================================================
//  ToastManager.qml
// =============================================================================
//  Top-level singleton-like host for toast notifications. Drop one instance
//  anywhere high in the QML tree (App.qml) and call ToastManager.show(...) from
//  anywhere in the app.
//
//  Public API:
//      show(variant, title, description, actionLabel, duration) → id
//      dismiss(id)
//      dismissAll()
//
//  Position: top-right by default. Stacks vertically downward.
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    // Stack of toast objects (each is a Toast{} instance)
    property var _toasts: []

    // Anchor area — top-right
    anchors.top: parent ? parent.top : undefined
    anchors.right: parent ? parent.right : undefined
    anchors.topMargin: Theme.space.xxl
    anchors.rightMargin: Theme.space.xxl
    width: 360
    height: parent ? parent.height : 0
    z: Theme.z.toast

    // ----- Column to lay out toasts -----
    Column {
        id: _stack
        anchors.top: parent.top
        anchors.right: parent.right
        width: parent.width
        spacing: Theme.space.md
    }

    // ----- Public API -----

    // Show a toast. Returns an opaque id usable to dismiss it.
    function show(variant, title, description, actionLabel, duration) {
        var params = {
            "variant":     variant     || "info",
            "title":       title       || "",
            "description": description || "",
            "actionLabel": actionLabel || "",
            "duration":    duration === undefined ? 4000 : duration
        }

        var comp = Qt.createComponent("Toast.qml")
        if (comp.status !== Component.Ready) {
            console.warn("Toast.qml failed to load:", comp.errorString())
            return null
        }

        var t = comp.createObject(_stack, params)
        if (!t) return null

        // Wire dismissed → remove from stack
        t.dismissed.connect(function() { _remove(t) })

        _toasts.push(t)
        return t
    }

    function info(title, description, duration)         { return show("info",    title, description, "", duration) }
    function success(title, description, duration)      { return show("success", title, description, "", duration) }
    function warning(title, description, duration)      { return show("warning", title, description, "", duration) }
    function error(title, description, duration)        { return show("error",   title, description, "", duration) }

    function dismiss(t) { _remove(t) }
    function dismissAll() {
        for (var i = 0; i < _toasts.length; i++) {
            if (_toasts[i]) _toasts[i].destroy()
        }
        _toasts = []
    }

    function _remove(t) {
        var idx = _toasts.indexOf(t)
        if (idx >= 0) _toasts.splice(idx, 1)
        if (t) t.destroy()
    }
}
