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

//! Write mode's Notes panel (spec 52), the left zone: durations (common ones, or all with the arrow), accidentals and
//! slur, articulations. Each button toggles the property on the selection or on the next input note, like MuseScore's
//! note-input bar, and is highlighted when it is the current one.
Item {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    property bool showAllDurations: false

    // Dorico's full set runs maxima..1/1024 (screenshot); the common set is our choice (the manual only says "the most
    // common"). MuseScore has no maxima, so that button is an unavailable placeholder.
    readonly property var commonDurations: ["whole", "half", "quarter", "eighth", "16th", "32nd"]
    readonly property var allDurations: ["maxima", "longa", "breve", "whole", "half", "quarter", "eighth", "16th", "32nd",
        "64th", "128th", "256th", "512th", "1024th"]
    readonly property var durationIcons: ({
        "maxima": "\uE95C", "longa": "\uE95D", "breve": "\uECA1", "whole": "\uECA2", "half": "\uECA3",
        "quarter": "\uECA5", "eighth": "\uECA7", "16th": "\uECA9", "32nd": "\uECAB", "64th": "\uECAD",
        // Dorico labels the four shortest values with fractions.
        "128th": "frac:128", "256th": "frac:256", "512th": "frac:512", "1024th": "frac:1024"
    })
    readonly property var durations: root.showAllDurations ? root.allDurations : root.commonDurations

    NavigationPanel {
        id: navPanel
        name: "DoricoNotesPanel"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Both
        enabled: root.enabled && root.visible
    }

    StyledFlickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 16

        Column {
            id: content

            x: 8
            y: 8
            width: parent.width - 16
            spacing: 12

            // Show/Hide All Notes: one toggle with a disclosure arrow above and below the durations, as in Dorico.
            // Collapsed, the top arrow points up (longer values) and the bottom one down (shorter values).
            DoricoButton {
                anchors.horizontalCenter: parent.horizontalCenter
                doricoIcon: root.showAllDurations ? "svg:chevron-down" : "svg:chevron-up"
                iconBoxSize: 28
                transparent: true
                toolTipTitle: root.showAllDurations ? qsTrc("dorico", "Hide All Notes") : qsTrc("dorico", "Show All Notes")
                navigation.panel: navPanel
                navigation.order: 0
                onTriggered: root.showAllDurations = !root.showAllDurations
            }

            ToolSection {
                width: parent.width
                iconSize: 34
                navigationPanel: navPanel
                navigationOrderStart: 1
                // "maxima" has no command, so ToolSection shows it as a "Not available yet" placeholder.
                codes: root.durations.map(d => d === "maxima" ? "command://dorico/set-duration-maxima"
                                                              : "command://notation/set-duration-" + d)
                labels: root.durations.map(d => d === "maxima" ? qsTrc("dorico", "Maxima") : undefined)
                icons: root.durations.map(d => root.durationIcons[d])
            }

            DoricoButton {
                anchors.horizontalCenter: parent.horizontalCenter
                doricoIcon: root.showAllDurations ? "svg:chevron-up" : "svg:chevron-down"
                iconBoxSize: 28
                transparent: true
                toolTipTitle: root.showAllDurations ? qsTrc("dorico", "Hide All Notes") : qsTrc("dorico", "Show All Notes")
                navigation.panel: navPanel
                navigation.order: 19
                onTriggered: root.showAllDurations = !root.showAllDurations
            }

            SeparatorLine {}

            ToolSection {
                width: parent.width
                iconSize: 34
                navigationPanel: navPanel
                navigationOrderStart: 20
                codes: [
                    "command://notation/toggle-nat",
                    "command://notation/toggle-flat",
                    "command://notation/toggle-sharp",
                    "command://notation/add-slur"
                ]
                labels: [qsTrc("dorico", "Natural"), qsTrc("dorico", "Flat"), qsTrc("dorico", "Sharp"), qsTrc("dorico", "Slur")]
                icons: ["\uE261", "\uE260", "\uE262", "svg:slur"]
            }

            SeparatorLine {}

            // Accent, marcato, staccato and tenuto are MuseScore's toggles; the other four are spec 11's Dorico
            // commands (MuseScore has no command for them). Listed row by row so the two columns match Dorico's
            // (left: accent, marcato, stressed, unstressed; right: staccato, tenuto, staccatissimo, tenuto-staccato),
            // with Dorico's divider line between them.
            Item {
                width: parent.width
                implicitHeight: articulations.implicitHeight

                ToolSection {
                    id: articulations

                    width: parent.width
                    iconSize: 34
                    navigationPanel: navPanel
                    navigationOrderStart: 30
                    codes: [
                        "command://notation/toggle-sforzato",
                        "command://notation/toggle-staccato",
                        "command://notation/toggle-marcato",
                        "command://notation/toggle-tenuto",
                        "command://dorico/toggle-articulation-stressed",
                        "command://dorico/toggle-articulation-staccatissimo",
                        "command://dorico/toggle-articulation-unstressed",
                        "command://dorico/toggle-articulation-tenuto-staccato"
                    ]
                    labels: [
                        qsTrc("dorico", "Accent"), qsTrc("dorico", "Staccato"), qsTrc("dorico", "Marcato"), qsTrc("dorico", "Tenuto"),
                        qsTrc("dorico", "Stressed"), qsTrc("dorico", "Staccatissimo"), qsTrc("dorico", "Unstressed"), qsTrc("dorico", "Tenuto-staccato")
                    ]
                    icons: ["\uE4A0", "\uE4A2", "\uE4AC", "\uE4A4", "\uE4B6", "\uE4A6", "\uE4B8", "\uE4B2"]
                }

                Rectangle {
                    // Between the columns: a button is iconSize + 10 wide and the Flow's spacing is 4.
                    x: articulations.iconSize + 10 + 1
                    width: 1
                    height: parent.height
                    color: ui.theme.strokeColor
                }
            }
        }
    }
}
