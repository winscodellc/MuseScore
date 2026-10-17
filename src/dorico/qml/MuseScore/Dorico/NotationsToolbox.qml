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

//! Write mode's Notations toolbox (spec 53): the vertical strip at the far right. Two buttons at the top choose
//! Panels (each button shows that panel in the right zone, NotationsPanel.qml) or Popovers (each button opens a
//! popover, spec 20; Cues and MIDI Trigger Region have no command and stay unavailable).
Column {
    id: root

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "DoricoNotationsToolbox"
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("dorico", "Notations toolbox")
    }

    property string currentPanelId: ""
    property bool popoversMode: false

    signal panelClicked(string panelId)

    readonly property var panels: [
        { "id": "clefs", "title": qsTrc("dorico", "Clefs"), "icon": "\uE050" },
        { "id": "keySignatures", "title": qsTrc("dorico", "Key Signatures, Tonality Systems, and Accidentals"), "icon": "\uE262\uE262" },
        { "id": "timeSignatures", "title": qsTrc("dorico", "Time Signatures"), "icon": "\uE08A" },
        { "id": "tempo", "title": qsTrc("dorico", "Tempo"), "icon": "svg:tempo" },
        { "id": "dynamics", "title": qsTrc("dorico", "Dynamics"), "icon": "\uE522" },
        { "id": "ornaments", "title": qsTrc("dorico", "Ornaments"), "icon": "\uE566" },
        { "id": "repeatStructures", "title": qsTrc("dorico", "Repeat Structures"), "icon": "\uE040" },
        { "id": "barsAndBarlines", "title": qsTrc("dorico", "Bars and Barlines"), "icon": "\uE032" },
        { "id": "holdsAndPauses", "title": qsTrc("dorico", "Holds and Pauses"), "icon": "\uE4C0" },
        { "id": "playingTechniques", "title": qsTrc("dorico", "Playing Techniques"), "icon": "\uE610" },
        { "id": "lines", "title": qsTrc("dorico", "Lines"), "icon": "svg:move-up-right" },
        { "id": "cues", "title": qsTrc("dorico", "Cues"), "icon": "svg:cue" },
        { "id": "video", "title": qsTrc("dorico", "Video"), "icon": "svg:film" },
        { "id": "comments", "title": qsTrc("dorico", "Comments"), "icon": "svg:message-square" },
        { "id": "proofreading", "title": qsTrc("dorico", "Proofreading"), "icon": "svg:check-check" }
    ]

    // Popover ids follow spec 20. Rehearsal Marks inputs a mark directly and Text opens the text editor, as in
    // Dorico, so those two are MuseScore's own commands and already work.
    readonly property var popovers: [
        { "code": "command://dorico/popover/clef", "title": qsTrc("dorico", "Clefs"), "icon": "\uE050" },
        { "code": "command://dorico/popover/keysig", "title": qsTrc("dorico", "Key Signatures"), "icon": "\uE262\uE262" },
        { "code": "command://dorico/popover/timesig", "title": qsTrc("dorico", "Time Signatures"), "icon": "\uE08A" },
        { "code": "command://dorico/popover/tempo", "title": qsTrc("dorico", "Tempo"), "icon": "svg:tempo" },
        { "code": "command://dorico/popover/dynamics", "title": qsTrc("dorico", "Dynamics"), "icon": "\uE522" },
        { "code": "command://dorico/popover/ornaments", "title": qsTrc("dorico", "Ornaments"), "icon": "\uE566" },
        { "code": "command://dorico/popover/repeats", "title": qsTrc("dorico", "Repeat Structures"), "icon": "\uE040" },
        { "code": "command://dorico/popover/bars", "title": qsTrc("dorico", "Bars and Barlines"), "icon": "\uE032" },
        { "code": "command://dorico/popover/holds", "title": qsTrc("dorico", "Holds and Pauses"), "icon": "\uE4C0" },
        { "code": "command://dorico/popover/techniques", "title": qsTrc("dorico", "Playing Techniques"), "icon": "\uE610" },
        { "code": "command://dorico/popover/cues", "title": qsTrc("dorico", "Cues"), "icon": "svg:cue" },
        { "code": "command://notation/add-rehearsalmark-text", "title": qsTrc("dorico", "Rehearsal Marks"), "icon": "svg:rehearsal-mark" },
        { "code": "command://notation/add-staff-text", "title": qsTrc("dorico", "Text"), "icon": "svg:type" },
        { "code": "command://dorico/popover/lyrics", "title": qsTrc("dorico", "Lyrics"), "icon": "svg:mic-vocal" },
        { "code": "command://dorico/popover/chordsymbols", "title": qsTrc("dorico", "Chord Symbols"), "icon": "svg:chord-symbol" },
        { "code": "command://dorico/popover/notetools", "title": qsTrc("dorico", "Note Tools"), "icon": "\uE0A4" },
        { "code": "command://dorico/popover/fingering", "title": qsTrc("dorico", "Fingering"), "icon": "svg:hand" },
        { "code": "command://dorico/popover/figuredbass", "title": qsTrc("dorico", "Figured Bass"), "icon": "\uEA5B" },
        { "code": "command://dorico/popover/midi-trigger", "title": qsTrc("dorico", "MIDI Trigger Region"), "icon": "svg:keyboard-music" }
    ]

    spacing: 2
    padding: 4

    // Dorico's two mode buttons: Panels (palette) and Popovers (keyboard); the current mode is highlighted.
    Repeater {
        model: [
            { "popovers": false, "icon": "svg:palette", "title": qsTrc("dorico", "Panels") },
            { "popovers": true, "icon": "svg:keyboard", "title": qsTrc("dorico", "Popovers") }
        ]

        DoricoButton {
            required property var modelData
            required property int index

            readonly property bool isActive: root.popoversMode === modelData.popovers

            doricoIcon: modelData.icon
            iconBoxSize: 28
            accentButton: isActive
            transparent: !isActive
            toolTipTitle: modelData.title
            navigation.panel: root.navigationPanel
            navigation.order: index
            onTriggered: root.popoversMode = modelData.popovers
        }
    }

    // Not SeparatorLine: it anchors to the parent's edges, which fights this padded Column's positioning (endless
    // polish loop, laggy UI).
    Rectangle {
        width: 38
        height: 1
        color: ui.theme.strokeColor
    }

    Repeater {
        model: root.popoversMode ? [] : root.panels

        DoricoButton {
            required property var modelData
            required property int index

            readonly property bool isActive: root.currentPanelId === modelData.id

            doricoIcon: modelData.icon
            iconBoxSize: 28
            accentButton: isActive
            transparent: !isActive
            toolTipTitle: modelData.title

            navigation.panel: root.navigationPanel
            navigation.order: 2 + index
            navigation.accessible.name: modelData.title

            onTriggered: root.panelClicked(modelData.id)
        }
    }

    DoricoToolsModel {
        id: popoverTools
        codes: root.popovers.map(p => p.code)
        Component.onCompleted: popoverTools.load()
    }

    Repeater {
        model: root.popoversMode ? popoverTools.items : []

        DoricoButton {
            required property var modelData
            required property int index

            readonly property var spec: root.popovers[index]

            doricoIcon: spec.icon
            iconBoxSize: 28
            available: modelData.enabled
            transparent: true
            toolTipTitle: spec.title
            toolTipDescription: modelData.description
            toolTipShortcut: modelData.shortcuts

            navigation.panel: root.navigationPanel
            navigation.order: 2 + index
            navigation.accessible.name: spec.title

            onTriggered: popoverTools.handleMenuItem(modelData.id)
        }
    }
}
