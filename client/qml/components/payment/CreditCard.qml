// =============================================================================
//  CreditCard.qml — Visual Demo Credit Card Component
// =============================================================================
//  A realistic-looking credit card that updates in real-time as the user types.
//
//  Features:
//      • 3D flip animation using scale.x instead of rotation (avoids mirror bug)
//      • Automatic card-brand detection (Visa / Mastercard / Amex)
//      • Formatted card number display (groups of 4)
//      • Gradient background, chip, contactless icon
//      • Back face: magnetic strip, CVC, signature area
//
//  Public API:
//      cardNumber  : string
//      cardName    : string
//      cardExpiry  : string
//      cardCvc     : string
//      flipped     : bool  (true = show back)
// =============================================================================
import QtQuick 2.15
import "../../theme"

Item {
    id: root

    property string cardNumber: ""
    property string cardName: ""
    property string cardExpiry: ""
    property string cardCvc: ""
    property bool flipped: false

    readonly property real _cardWidth: 260
    readonly property real _cardHeight: _cardWidth / 1.586
    readonly property real _cardRadius: 14

    implicitWidth: _cardWidth
    implicitHeight: _cardHeight

    readonly property string _cleanNumber: cardNumber.replace(/\s+/g, "")
    readonly property string _brand: {
        var n = _cleanNumber
        if (n.length >= 1) {
            var c = n.charAt(0)
            if (c === '4') return "visa"
            if (c === '5') return "mastercard"
            if (c === '3') return "amex"
        }
        return ""
    }

    readonly property string _formattedNumber: {
        var n = _cleanNumber
        var r = ""
        for (var i = 0; i < n.length && i < 16; ++i) {
            if (i > 0 && i % 4 === 0) r += " "
            r += n.charAt(i)
        }
        var rem = 16 - n.length
        if (rem > 0) {
            if (r.length > 0) r += " "
            for (var j = 0; j < rem; ++j) {
                if (j > 0 && j % 4 === 0) r += " "
                r += "\u2022"
            }
        }
        return r
    }

    readonly property string _displayName: cardName.length > 0 ? cardName.toUpperCase() : "YOUR NAME"
    readonly property string _displayExpiry: cardExpiry.length > 0 ? cardExpiry : "MM/YY"

    // Shadow beneath the card
    Rectangle {
        width: _cardWidth - 6
        height: _cardHeight - 2
        radius: _cardRadius
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 5
        z: -1
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.18) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    // ---- Front face ----
    Rectangle {
        id: _front
        width: _cardWidth
        height: _cardHeight
        radius: _cardRadius
        anchors.centerIn: parent
        visible: _front.opacity > 0.01
        opacity: root.flipped ? 0.0 : 1.0

        property real _sx: root.flipped ? 0.85 : 1.0
        Behavior on _sx { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        transform: Scale { origin.x: _front.width / 2; origin.y: _front.height / 2; xScale: _front._sx }

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0;  color: "#1A1A2E" }
            GradientStop { position: 0.5;  color: "#16213E" }
            GradientStop { position: 1.0;  color: "#0F3460" }
        }

        // Decorative circles
        Rectangle {
            width: 150; height: 150; radius: 75
            anchors.right: parent.right; anchors.top: parent.top
            anchors.topMargin: -50; anchors.rightMargin: -30
            color: Qt.rgba(1,1,1,0.04)
        }
        Rectangle {
            width: 100; height: 100; radius: 50
            anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.bottomMargin: -25; anchors.rightMargin: 15
            color: Qt.rgba(1,1,1,0.03)
        }

        // Border
        Rectangle {
            anchors.fill: parent; radius: _cardRadius
            color: "transparent"
            border.color: Qt.rgba(1,1,1,0.12); border.width: 1
        }

        Column {
            anchors.fill: parent
            anchors.margins: 20
            anchors.topMargin: 18
            spacing: 0

            // Brand + contactless
            Row {
                width: parent.width; height: 24
                Item {
                    width: 42; height: 24
                    visible: root._brand.length > 0
                    Text {
                        visible: root._brand === "visa"
                        anchors.centerIn: parent
                        text: "VISA"; color: "#FFF"
                        font.family: "Inter, Arial"; font.pixelSize: 14
                        font.weight: Font.Bold; font.italic: true
                    }
                    Item {
                        visible: root._brand === "mastercard"
                        anchors.centerIn: parent; width: 30; height: 18
                        Rectangle { width: 16; height: 16; radius: 8; x: 0; y: 1; color: "#EB001B"; opacity: 0.9 }
                        Rectangle { width: 16; height: 16; radius: 8; x: 12; y: 1; color: "#F79E1B"; opacity: 0.9 }
                        Rectangle { width: 16; height: 16; radius: 8; x: 6; y: 1; color: "#FF5F00"; opacity: 0.85 }
                    }
                    Text {
                        visible: root._brand === "amex"
                        anchors.centerIn: parent
                        text: "AMEX"; color: "#FFF"
                        font.family: "Inter, Arial"; font.pixelSize: 10
                        font.weight: Font.Bold; font.letterSpacing: 1.2
                    }
                }
                Item { width: 1; height: 1 }
                Canvas {
                    width: 18; height: 18
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset()
                        ctx.strokeStyle = "rgba(255,255,255,0.5)"; ctx.lineWidth = 1.2; ctx.lineCap = "round"
                        ctx.beginPath(); ctx.arc(3,9,2.5,-0.8,0.8); ctx.stroke()
                        ctx.beginPath(); ctx.arc(3,9,5,-0.8,0.8); ctx.stroke()
                        ctx.beginPath(); ctx.arc(3,9,7.5,-0.8,0.8); ctx.stroke()
                        ctx.fillStyle = "rgba(255,255,255,0.6)"
                        ctx.beginPath(); ctx.arc(1.5,9,1.2,0,Math.PI*2); ctx.fill()
                    }
                }
            }

            Item { width: 1; height: 16 }

            // Chip
            Rectangle {
                width: 36; height: 27; radius: 5
                color: "#D4AF37"; border.color: "#C5A028"; border.width: 0.5
                Rectangle { width: parent.width; height: 1; color: "#B8960F"; anchors.centerIn: parent }
                Rectangle { width: 1; height: parent.height; color: "#B8960F"; anchors.centerIn: parent }
            }

            Item { width: 1; height: 20 }

            // Card number
            Text {
                text: root._formattedNumber; color: "#FFF"
                font.family: Theme.font.familyMono; font.pixelSize: 17
                font.weight: Font.Bold; font.letterSpacing: 1.8
                width: parent.width
            }

            Item { width: 1; height: 14 }

            // Name + Expiry
            Row {
                width: parent.width; spacing: 12
                Column {
                    width: (parent.width - 12) * 0.6; spacing: 2
                    Text { text: "CARD HOLDER"; color: Qt.rgba(1,1,1,0.45); font.family: Theme.font.family; font.pixelSize: 7; font.weight: Font.Bold; font.letterSpacing: 1 }
                    Text { text: root._displayName; color: "#FFF"; font.family: Theme.font.family; font.pixelSize: 13; font.weight: Font.Medium; font.letterSpacing: 0.6; elide: Text.ElideRight; width: parent.width }
                }
                Column {
                    width: (parent.width - 12) * 0.4; spacing: 2
                    Text { text: "EXPIRES"; color: Qt.rgba(1,1,1,0.45); font.family: Theme.font.family; font.pixelSize: 7; font.weight: Font.Bold; font.letterSpacing: 1 }
                    Text { text: root._displayExpiry; color: "#FFF"; font.family: Theme.font.familyMono; font.pixelSize: 13; font.weight: Font.Medium }
                }
            }
        }
    }

    // ---- Back face ----
    Rectangle {
        id: _back
        width: _cardWidth
        height: _cardHeight
        radius: _cardRadius
        anchors.centerIn: parent
        visible: _back.opacity > 0.01
        opacity: root.flipped ? 1.0 : 0.0

        property real _sx: root.flipped ? 1.0 : 0.85
        Behavior on _sx { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        transform: Scale { origin.x: _back.width / 2; origin.y: _back.height / 2; xScale: _back._sx }

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0;  color: "#1A1A2E" }
            GradientStop { position: 0.5;  color: "#16213E" }
            GradientStop { position: 1.0;  color: "#0F3460" }
        }

        Rectangle {
            anchors.fill: parent; radius: _cardRadius
            color: "transparent"
            border.color: Qt.rgba(1,1,1,0.12); border.width: 1
        }

        Column {
            anchors.fill: parent; spacing: 0

            // Magnetic strip
            Rectangle {
                width: parent.width; height: 42; color: "#000000"
                Rectangle {
                    anchors.fill: parent; anchors.margins: 2; color: "transparent"
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: Qt.rgba(1,1,1,0.03) }
                        GradientStop { position: 0.5; color: Qt.rgba(1,1,1,0.0) }
                        GradientStop { position: 1.0; color: Qt.rgba(1,1,1,0.03) }
                    }
                }
            }

            Item { width: 1; height: 16 }

            // CVC
            Row {
                leftPadding: 20; spacing: 10
                Text {
                    text: "CVC"; color: Qt.rgba(1,1,1,0.55)
                    font.family: Theme.font.family; font.pixelSize: 9
                    font.weight: Font.Bold; font.letterSpacing: 0.8
                    anchors.verticalCenter: parent.verticalCenter
                }
                Rectangle {
                    width: 44; height: 28; radius: 4
                    color: Qt.rgba(1,1,1,0.9)
                    Text {
                        anchors.centerIn: parent
                        text: root.cardCvc.length > 0 ? root.cardCvc : "\u2022\u2022\u2022"
                        color: "#1A1A2E"; font.family: Theme.font.familyMono
                        font.pixelSize: 14; font.weight: Font.Bold; font.letterSpacing: 2
                    }
                }
            }

            Item { width: 1; height: 14 }

            // Signature strip
            Rectangle {
                width: parent.width - 40; anchors.horizontalCenter: parent.horizontalCenter
                height: 30; radius: 3
                color: Qt.rgba(1,1,1,0.12)
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.cardName.length > 0 ? root.cardName : "AUTHORIZED SIGNATURE"
                    color: Qt.rgba(1,1,1,0.35)
                    font.family: Theme.font.family; font.pixelSize: 10
                    font.italic: true; elide: Text.ElideRight; width: parent.width - 16
                }
            }

            Item { width: 1; height: 10 }

            // Brand bottom-right
            Row {
                width: parent.width; height: 20
                leftPadding: 20; rightPadding: 20
                Text {
                    text: "This card is property of the issuing bank."
                    color: Qt.rgba(1,1,1,0.25); font.family: Theme.font.family
                    font.pixelSize: 6; anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 50; elide: Text.ElideRight
                }
                Text {
                    visible: root._brand === "visa"
                    anchors.verticalCenter: parent.verticalCenter
                    text: "VISA"; color: "#FFF"; opacity: 0.6
                    font.family: "Inter, Arial"; font.pixelSize: 12
                    font.weight: Font.Bold; font.italic: true
                }
                Item {
                    visible: root._brand === "mastercard"
                    width: 24; height: 14; anchors.verticalCenter: parent.verticalCenter
                    Rectangle { width: 12; height: 12; radius: 6; x: 0; y: 1; color: "#EB001B"; opacity: 0.6 }
                    Rectangle { width: 12; height: 12; radius: 6; x: 8; y: 1; color: "#F79E1B"; opacity: 0.6 }
                }
            }
        }
    }
}