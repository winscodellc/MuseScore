/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import Muse.GraphicalEffects

//! A FlatButton that can be "unavailable": it looks disabled but stays hoverable, so its tooltip can say why
//! (a disabled FlatButton disables its MouseArea and shows no tooltip). Use `triggered`, not `clicked`.
//! `doricoIcon` (spec 60) is an IconCode, a SMuFL glyph string (drawn in Bravura Text) or "svg:<name>" (a Lucide
//! icon in icons/) or "frac:<n>" (a stacked 1/n, Dorico's label for the shortest durations). Checked icon-only
//! buttons use Dorico's bright tool blue with a black (Dark) or white (Light)
//! icon; the theme accent (a muted blue) stays for text buttons, where it keeps the text readable.
FlatButton {
    id: root

    property bool available: true
    property var doricoIcon: IconCode.NONE
    readonly property bool hasCustomIcon: typeof root.doricoIcon === "string" && root.doricoIcon !== ""
    // Spec 60 sizes, measured against Dorico 6: 28 toolbox, 34 notation in panels, 24 panel actions, 22 bars.
    property real iconBoxSize: 22
    // With a custom icon: show this text beside it (Dorico's labelled panel buttons, e.g. Engrave's Formatting panel)
    property string label: ""
    readonly property bool isToolIcon: root.text === "" && root.label === ""

    signal triggered()

    icon: root.hasCustomIcon ? IconCode.NONE : root.doricoIcon
    accentColor: root.isToolIcon ? (ui.theme.isDark ? "#40ACFF" : "#0068AD") : ui.theme.accentColor
    iconColor: root.accentButton && root.isToolIcon ? (ui.theme.isDark ? "#000000" : "#FFFFFF") : ui.theme.fontPrimaryColor
    contentItem: !root.hasCustomIcon ? null
               : root.label !== "" ? labelledIconComponent
               : root.doricoIcon.startsWith("frac:") ? fractionComponent : customIconComponent

    // A custom icon makes FlatButton a Custom button, which has no size of its own. A labelled one sizes itself from
    // its content (FlatButton: content + 2 * margins).
    Binding on implicitWidth {
        when: root.hasCustomIcon && root.label === ""
        value: root.iconBoxSize + 10
    }
    Binding on implicitHeight {
        when: root.hasCustomIcon
        value: root.iconBoxSize + 10
    }

    opacity: root.available ? 1.0 : ui.theme.itemOpacityDisabled
    // FlatButton shows no tooltip without a title, so text buttons use their text
    toolTipTitle: root.available ? "" : (root.text || root.label)
    toolTipDescription: root.available ? "" : qsTrc("dorico", "Not available yet")

    onClicked: {
        if (root.available) {
            root.triggered()
        }
    }

    Component {
        id: labelledIconComponent

        // Needs its own size: FlatButton centres its content, so a zero-size item would start at the button's middle
        Item {
            implicitWidth: labelledRow.implicitWidth
            implicitHeight: labelledRow.implicitHeight

            Row {
                id: labelledRow

                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Loader {
                    width: root.iconBoxSize
                    height: root.iconBoxSize
                    sourceComponent: customIconComponent
                }

                StyledTextLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.label
                }
            }
        }
    }

    Component {
        id: fractionComponent

        Item {
            implicitWidth: root.iconBoxSize
            implicitHeight: root.iconBoxSize

            Column {
                anchors.centerIn: parent
                spacing: 1

                StyledTextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "1"
                    font.pixelSize: 11
                    color: root.iconColor
                    renderType: Text.CurveRendering // grayscale, see the glyph below
                }

                Rectangle {
                    width: denominator.implicitWidth
                    height: 1
                    color: root.iconColor
                }

                StyledTextLabel {
                    id: denominator
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: String(root.doricoIcon).slice(5)
                    font.pixelSize: 11
                    color: root.iconColor
                    renderType: Text.CurveRendering // grayscale, see the glyph below
                }
            }
        }
    }

    Component {
        id: customIconComponent

        Item {
            id: box

            readonly property bool isSvg: String(root.doricoIcon).startsWith("svg:")
            readonly property color tint: root.iconColor

            implicitWidth: root.iconBoxSize
            implicitHeight: root.iconBoxSize

            // Spec 96: only the parts the icon needs. A glyph button gets no SVG image and no ColorOverlay (a ShaderEffect
            // that breaks scene-graph batching); an SVG button gets no text.
            Loader {
                anchors.fill: parent
                active: !box.isSvg

                sourceComponent: Item {
                    // Glyph ink bounds at 100 px, relative to the baseline origin.
                    TextMetrics {
                        id: metrics
                        font.family: "Bravura Text"
                        font.pixelSize: 100
                        text: root.doricoIcon
                    }

                    Text {
                        readonly property rect ink: metrics.tightBoundingRect
                        // Fit the ink into 85% of the box; tiny glyphs (dots, accents) grow to at most 100 px.
                        readonly property real scale100: ink.width > 0 && ink.height > 0
                                                         ? Math.min(0.85 * box.width / ink.width, 0.85 * box.height / ink.height, 1.0) : 0

                        text: root.doricoIcon
                        font.family: "Bravura Text"
                        font.pixelSize: Math.max(1, scale100 * 100)
                        // Grayscale antialiasing: native (LCD) rendering gives stems and noteheads blue/orange fringes, and
                        // QtRendering still falls back to it for Bravura on macOS (seen in the spec 52 review).
                        renderType: Text.CurveRendering
                        color: box.tint

                        // Center the ink, not the text box: glyphs sit at very different heights around the baseline.
                        x: (box.width - ink.width * scale100) / 2 - ink.x * scale100
                        y: (box.height - ink.height * scale100) / 2 - ink.y * scale100 - baselineOffset
                    }
                }
            }

            Loader {
                anchors.fill: parent
                active: box.isSvg

                sourceComponent: Item {
                    Image {
                        id: svgImage
                        anchors.centerIn: parent
                        visible: false
                        source: Qt.resolvedUrl("icons/" + root.doricoIcon.slice(4) + ".svg")
                        sourceSize: Qt.size(box.width - 2, box.height - 2)
                    }

                    EffectColorOverlay {
                        anchors.fill: svgImage
                        source: svgImage
                        color: box.tint
                    }
                }
            }
        }
    }
}
