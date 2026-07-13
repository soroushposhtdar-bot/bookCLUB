// =============================================================================
//  AuthLayout.qml
// =============================================================================
//  Shell layout for every Authentication page. Renders the signature
//  split-screen card described by the design system:
//
//      ┌─────────────────────────────────────────────────────────┐
//      │  ┌────────────────────┬──────────────────────────────┐  │
//      │  │                    │                              │  │
//      │  │   Hero Panel       │   Form Panel                 │  │
//      │  │   (55%)            │   (45%)                      │  │
//      │  │   - BrandLogo      │   - page child (slotted)     │  │
//      │  │   - headline       │                              │  │
//      │  │   - subtitle       │                              │  │
//      │  │   - security badge │                              │  │
//      │  │                    │                              │  │
//      │  └────────────────────┴──────────────────────────────┘  │
//      └─────────────────────────────────────────────────────────┘
//
//  Public API:
//      heroTitle       : string
//      heroSubtitle    : string
//      heroBadgeLabel  : string
//      default property alias content : _formSlot.data
// =============================================================================
import QtQuick 2.15
import "../theme"
import "../components/surfaces"
import "../components/branding"
import "../components/effects"

Item {
    id: root

    property string heroTitle: "Welcome to BookClub"
    property string heroSubtitle: "Sign in to continue to your reading journey"
    property string heroBadgeLabel: "Secure & Private"
    property string heroBadgeText: "Your data is encrypted and always protected."
    property string heroIcon: "auto_stories"

    // Default content slot — child items become the form panel content
    default property alias content: _formSlot.data

    // Centered card on neutral page background
    Rectangle {
        anchors.fill: parent
        color: Theme.color.pageBackground
    }

    // ----- Floating Card -----
    Rectangle {
        id: _card
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.space.huge, Theme.size.cardMaxWidth)
        height: Math.min(parent.height - 2 * Theme.space.xl, 620)

        radius: Theme.radius.xl
        color: Theme.color.cardBackground
        clip: true

        layer.enabled: true
        layer.effect: DropShadowBase {
            colorSpec: Theme.shadow.xl
        }

        // ----- Hero Panel (left) -----
        Item {
            id: _hero
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * 0.55

            // Soft gradient background
            Rectangle {
                id: _heroBg
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: Theme.color.heroGradientTop }
                    GradientStop { position: 1.0; color: Theme.color.heroGradientBottom }
                }
            }

            // Organic curve decoration (top-right)
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    var w = width, h = height
                    ctx.beginPath()
                    ctx.moveTo(w * 0.55, 0)
                    ctx.bezierCurveTo(w * 0.85, h * 0.10, w * 0.95, h * 0.45, w * 1.05, h * 0.78)
                    ctx.bezierCurveTo(w * 1.10, h * 0.92, w * 0.92, h, w * 0.78, h)
                    ctx.lineTo(0, h)
                    ctx.lineTo(0, 0)
                    ctx.closePath()
                    ctx.fillStyle = "rgba(255, 255, 255, 0.55)"
                    ctx.fill()
                }
            }

            // Decorative book spines (faint, bottom-right corner)
            Canvas {
                width: 200
                height: 200
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: -40
                anchors.bottomMargin: -40
                opacity: 0.18
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    var colors = ["#0A0A0B", "#1A73E8", "#5F6368", "#0A0A0B", "#1A73E8"]
                    var w = 22, h = 140, gap = 6
                    for (var i = 0; i < colors.length; i++) {
                        ctx.fillStyle = colors[i]
                        ctx.fillRect(i * (w + gap), 60 - i * 4, w, h - i * 8)
                    }
                }
            }

            // Hero content column
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: Theme.space.mega
                anchors.rightMargin: Theme.space.xxl
                spacing: Theme.space.lg

                BrandLogo {
                    size: Theme.size.logoSize
                }

                Text {
                    text: root.heroTitle
                    color: Theme.color.textPrimary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeHero
                    font.weight: Theme.font.weightBold
                    wrapMode: Text.WordWrap
                    width: parent.width
                    lineHeight: 1.15
                }

                Text {
                    text: root.heroSubtitle
                    color: Theme.color.textSecondary
                    font.family: Theme.font.family
                    font.pixelSize: Theme.font.sizeBodyLarge
                    font.weight: Theme.font.weightRegular
                    wrapMode: Text.WordWrap
                    width: parent.width
                    lineHeight: 1.45
                }

                Item { width: 1; height: Theme.space.xs }

                // Security badge + descriptor
                Column {
                    spacing: Theme.space.sm
                    width: parent.width

                    SecurityBadge {
                        label: root.heroBadgeLabel
                    }

                    Text {
                        text: root.heroBadgeText
                        color: Theme.color.textMuted
                        font.family: Theme.font.family
                        font.pixelSize: Theme.font.sizeSmall
                        font.weight: Theme.font.weightRegular
                        wrapMode: Text.WordWrap
                        width: parent.width
                        lineHeight: 1.5
                    }
                }
            }
        }

        // ----- Form Panel (right) -----
        Item {
            id: _formPanel
            anchors.left: _hero.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            // Subtle vertical line between hero and form
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.color.divider
            }

            // Form content slot — scrollable when overflowing
            Flickable {
                id: _flick
                anchors.fill: parent
                anchors.margins: Theme.space.xxxl
                anchors.leftMargin: Theme.space.xxl
                anchors.rightMargin: Theme.space.xxl
                contentWidth: width
                contentHeight: _formSlot.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentHeight > height

                Column {
                    id: _formSlot
                    width: parent.width
                    spacing: Theme.space.xl
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // ----- Responsive: collapse hero on narrow viewports -----
    states: [
        State {
            name: "narrow"
            when: root.width < 760
            PropertyChanges { target: _hero; visible: false; width: 0 }
            AnchorChanges {
                target: _formPanel
                anchors.left: _card.left
            }
            PropertyChanges { target: _card; width: Math.min(root.width - 2 * Theme.space.lg, Theme.size.formMaxWidth) }
        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "width"; duration: Theme.motion.durationBase; easing.type: Easing.OutCubic }
    }
}
