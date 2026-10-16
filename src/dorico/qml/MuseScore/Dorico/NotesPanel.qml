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

    readonly property var commonDurations: ["half", "quarter", "eighth", "16th", "32nd"]
    readonly property var allDurations: ["breve", "whole", "half", "quarter", "eighth", "16th", "32nd", "64th", "128th"]
    readonly property var durationIcons: ({
        "breve": "\uECA0", "whole": "\uECA2", "half": "\uECA3", "quarter": "\uECA5", "eighth": "\uECA7",
        "16th": "\uECA9", "32nd": "\uECAB", "64th": "\uECAD", "128th": "\uECAF"
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

            FlatButton {
                width: parent.width
                icon: root.showAllDurations ? IconCode.SMALL_ARROW_UP : IconCode.SMALL_ARROW_DOWN
                transparent: true
                toolTipTitle: root.showAllDurations ? qsTrc("dorico", "Hide All Notes") : qsTrc("dorico", "Show All Notes")
                navigation.panel: navPanel
                navigation.order: 0
                onClicked: root.showAllDurations = !root.showAllDurations
            }

            ToolSection {
                width: parent.width
                navigationPanel: navPanel
                navigationOrderStart: 1
                codes: root.durations.map(d => "command://notation/set-duration-" + d)
                icons: root.durations.map(d => root.durationIcons[d])
            }

            SeparatorLine {}

            ToolSection {
                width: parent.width
                navigationPanel: navPanel
                navigationOrderStart: 20
                codes: [
                    "command://notation/toggle-flat",
                    "command://notation/toggle-nat",
                    "command://notation/toggle-sharp",
                    "command://notation/add-slur"
                ]
                labels: [qsTrc("dorico", "Flat"), qsTrc("dorico", "Natural"), qsTrc("dorico", "Sharp"), qsTrc("dorico", "Slur")]
                icons: ["\uE260", "\uE261", "\uE262", "svg:slur"]
            }

            SeparatorLine {}

            // Accent, marcato, staccato and tenuto are MuseScore's toggles; the other four are spec 11's Dorico
            // commands (MuseScore has no command for them), shown as text since MuseScore has no icon for them.
            ToolSection {
                width: parent.width
                navigationPanel: navPanel
                navigationOrderStart: 30
                codes: [
                    "command://notation/toggle-sforzato",
                    "command://notation/toggle-marcato",
                    "command://dorico/toggle-articulation-stressed",
                    "command://dorico/toggle-articulation-unstressed",
                    "command://notation/toggle-staccato",
                    "command://notation/toggle-tenuto",
                    "command://dorico/toggle-articulation-staccatissimo",
                    "command://dorico/toggle-articulation-tenuto-staccato"
                ]
                labels: [
                    qsTrc("dorico", "Accent"), qsTrc("dorico", "Marcato"), qsTrc("dorico", "Stressed"), qsTrc("dorico", "Unstressed"),
                    qsTrc("dorico", "Staccato"), qsTrc("dorico", "Tenuto"), qsTrc("dorico", "Staccatissimo"), qsTrc("dorico", "Tenuto-staccato")
                ]
                icons: ["\uE4A0", "\uE4AC", "\uE4B6", "\uE4B8", "\uE4A2", "\uE4A4", "\uE4A6", "\uE4B2"]
            }
        }
    }
}
