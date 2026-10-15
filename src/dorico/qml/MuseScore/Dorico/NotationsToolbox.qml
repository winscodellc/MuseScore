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

//! Write mode's Notations toolbox (spec 53): the vertical strip at the far right. A switch at the top chooses
//! Panels (each button shows that panel in the right zone, NotationsPanel.qml) or Popovers (each button opens a
//! popover; until the popover specs (20-36) register `command://dorico/popover/<id>` those buttons are unavailable).
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
        { "id": "clefs", "title": qsTrc("dorico", "Clefs"), "icon": IconCode.CLEF_TREBLE },
        { "id": "keySignatures", "title": qsTrc("dorico", "Key Signatures, Tonality Systems, and Accidentals"), "icon": IconCode.KEY_SIGNATURE },
        { "id": "timeSignatures", "title": qsTrc("dorico", "Time Signatures"), "icon": IconCode.TIME_SIGNATURE },
        { "id": "tempo", "title": qsTrc("dorico", "Tempo"), "icon": IconCode.TEMPO_CHANGE },
        { "id": "dynamics", "title": qsTrc("dorico", "Dynamics"), "icon": IconCode.DYNAMIC_FORTE },
        { "id": "ornaments", "title": qsTrc("dorico", "Ornaments"), "icon": IconCode.ORNAMENT },
        { "id": "repeatStructures", "title": qsTrc("dorico", "Repeat Structures"), "icon": IconCode.REPEAT_START },
        { "id": "barsAndBarlines", "title": qsTrc("dorico", "Bars and Barlines"), "icon": IconCode.DOUBLE_BAR_LINE },
        { "id": "holdsAndPauses", "title": qsTrc("dorico", "Holds and Pauses"), "icon": IconCode.FERMATA },
        { "id": "playingTechniques", "title": qsTrc("dorico", "Playing Techniques"), "icon": IconCode.ARTICULATION },
        { "id": "lines", "title": qsTrc("dorico", "Lines"), "icon": IconCode.LINE_NORMAL },
        { "id": "cues", "title": qsTrc("dorico", "Cues"), "icon": IconCode.MICROPHONE },
        { "id": "video", "title": qsTrc("dorico", "Video"), "icon": IconCode.VIDEO },
        { "id": "comments", "title": qsTrc("dorico", "Comments"), "icon": IconCode.FEEDBACK },
        { "id": "proofreading", "title": qsTrc("dorico", "Proofreading"), "icon": IconCode.TICK }
    ]

    // Popover ids follow spec 20. Rehearsal Marks inputs a mark directly and Text opens the text editor, as in
    // Dorico, so those two are MuseScore's own commands and already work.
    readonly property var popovers: [
        { "code": "command://dorico/popover/clef", "title": qsTrc("dorico", "Clefs"), "icon": IconCode.CLEF_TREBLE },
        { "code": "command://dorico/popover/keysig", "title": qsTrc("dorico", "Key Signatures"), "icon": IconCode.KEY_SIGNATURE },
        { "code": "command://dorico/popover/timesig", "title": qsTrc("dorico", "Time Signatures"), "icon": IconCode.TIME_SIGNATURE },
        { "code": "command://dorico/popover/tempo", "title": qsTrc("dorico", "Tempo"), "icon": IconCode.TEMPO_CHANGE },
        { "code": "command://dorico/popover/dynamics", "title": qsTrc("dorico", "Dynamics"), "icon": IconCode.DYNAMIC_FORTE },
        { "code": "command://dorico/popover/ornaments", "title": qsTrc("dorico", "Ornaments"), "icon": IconCode.ORNAMENT },
        { "code": "command://dorico/popover/repeats", "title": qsTrc("dorico", "Repeat Structures"), "icon": IconCode.REPEAT_START },
        { "code": "command://dorico/popover/bars", "title": qsTrc("dorico", "Bars and Barlines"), "icon": IconCode.DOUBLE_BAR_LINE },
        { "code": "command://dorico/popover/holds", "title": qsTrc("dorico", "Holds and Pauses"), "icon": IconCode.FERMATA },
        { "code": "command://dorico/popover/techniques", "title": qsTrc("dorico", "Playing Techniques"), "icon": IconCode.ARTICULATION },
        { "code": "command://dorico/popover/cues", "title": qsTrc("dorico", "Cues"), "icon": IconCode.MICROPHONE },
        { "code": "command://notation/add-rehearsalmark-text", "title": qsTrc("dorico", "Rehearsal Marks"), "icon": IconCode.MARKER },
        { "code": "command://notation/add-staff-text", "title": qsTrc("dorico", "Text"), "icon": IconCode.TEXT_BOLD },
        { "code": "command://dorico/popover/lyrics", "title": qsTrc("dorico", "Lyrics"), "icon": IconCode.LYRICS },
        { "code": "command://dorico/popover/chordsymbols", "title": qsTrc("dorico", "Chord Symbols"), "icon": IconCode.CHORD_SYMBOL },
        { "code": "command://dorico/popover/notetools", "title": qsTrc("dorico", "Note Tools"), "icon": IconCode.NOTE_HEAD },
        { "code": "command://dorico/popover/fingering", "title": qsTrc("dorico", "Fingering"), "icon": IconCode.FRETBOARD_DIAGRAM },
        { "code": "command://dorico/popover/figuredbass", "title": qsTrc("dorico", "Figured Bass"), "icon": IconCode.MUSIC_NOTES },
        { "code": "command://dorico/popover/midi-trigger", "title": qsTrc("dorico", "MIDI Trigger Region"), "icon": IconCode.MIDI_INPUT }
    ]

    spacing: 2
    padding: 4

    FlatButton {
        id: modeButton

        icon: root.popoversMode ? IconCode.EDIT : IconCode.GRID
        transparent: true
        toolTipTitle: root.popoversMode ? qsTrc("dorico", "Popovers") : qsTrc("dorico", "Panels")
        toolTipDescription: qsTrc("dorico", "Switch between panels and popovers")
        navigation.panel: root.navigationPanel
        navigation.order: 0
        onClicked: root.popoversMode = !root.popoversMode
    }

    // Not SeparatorLine: it anchors to the parent's edges, which fights this padded Column's positioning (endless
    // polish loop, laggy UI).
    Rectangle {
        width: modeButton.width
        height: 1
        color: ui.theme.strokeColor
    }

    Repeater {
        model: root.popoversMode ? [] : root.panels

        DoricoButton {
            required property var modelData
            required property int index

            readonly property bool isActive: root.currentPanelId === modelData.id

            icon: modelData.icon
            accentButton: isActive
            transparent: !isActive
            toolTipTitle: modelData.title

            navigation.panel: root.navigationPanel
            navigation.order: 1 + index
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

            icon: spec.icon
            available: modelData.enabled
            transparent: true
            toolTipTitle: spec.title
            toolTipDescription: modelData.description
            toolTipShortcut: modelData.shortcuts

            navigation.panel: root.navigationPanel
            navigation.order: 1 + index
            navigation.accessible.name: spec.title

            onTriggered: popoverTools.handleMenuItem(modelData.id)
        }
    }
}
