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
//! icon in icons/). Checked icon-only buttons use Dorico's bright tool blue with a black (Dark) or white (Light)
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
    contentItem: root.hasCustomIcon ? (root.label !== "" ? labelledIconComponent : customIconComponent) : null

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
        id: customIconComponent

        Item {
            id: box

            readonly property bool isSvg: root.doricoIcon.startsWith("svg:")
            readonly property color tint: root.iconColor

            implicitWidth: root.iconBoxSize
            implicitHeight: root.iconBoxSize

            // Glyph ink bounds at 100 px, relative to the baseline origin.
            TextMetrics {
                id: metrics
                font.family: "Bravura Text"
                font.pixelSize: 100
                text: box.isSvg ? "" : root.doricoIcon
            }

            Text {
                id: glyph

                readonly property rect ink: metrics.tightBoundingRect
                // Fit the ink into 85% of the box; tiny glyphs (dots, accents) grow to at most 100 px.
                readonly property real scale100: ink.width > 0 && ink.height > 0
                                                 ? Math.min(0.85 * box.width / ink.width, 0.85 * box.height / ink.height, 1.0) : 0

                visible: !box.isSvg
                text: box.isSvg ? "" : root.doricoIcon
                font.family: "Bravura Text"
                font.pixelSize: Math.max(1, scale100 * 100)
                // Grayscale antialiasing: native (LCD) rendering gives thin stems and barlines blue/orange fringes.
                renderType: Text.QtRendering
                color: box.tint

                // Center the ink, not the text box: glyphs sit at very different heights around the baseline.
                x: (box.width - ink.width * scale100) / 2 - ink.x * scale100
                y: (box.height - ink.height * scale100) / 2 - ink.y * scale100 - baselineOffset
            }

            Image {
                id: svgImage
                anchors.centerIn: parent
                visible: false
                source: box.isSvg ? Qt.resolvedUrl("icons/" + root.doricoIcon.slice(4) + ".svg") : ""
                sourceSize: Qt.size(box.width - 2, box.height - 2)
            }

            EffectColorOverlay {
                anchors.fill: svgImage
                source: svgImage
                color: box.tint
                visible: box.isSvg
            }
        }
    }
}
