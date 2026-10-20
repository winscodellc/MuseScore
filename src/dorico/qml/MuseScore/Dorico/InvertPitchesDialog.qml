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

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

//! Write > Transform > Invert Pitches, Reverse and Invert Pitches (spec 65). Answers "mode,type,letter,accidental,octave,chord":
//! mode outer / pitch / root / first, type diat / chrom, letter 0-6 (C-B), accidental "bb" / "b" / "" / "#" / "##",
//! chord top / bottom.
StyledDialogView {
    id: root

    property string last: ""

    property string mode: "outer"
    property string kind: "diat"
    property int letter: 0
    property string accidental: ""
    property int octave: 4
    property string chord: "top"

    readonly property var noteNames: ["C", "D", "E", "F", "G", "A", "B"]
    readonly property var accidentals: [
        { text: qsTrc("dorico", "Double flat"), value: "bb" },
        { text: qsTrc("dorico", "Flat"), value: "b" },
        { text: qsTrc("dorico", "Natural"), value: "" },
        { text: qsTrc("dorico", "Sharp"), value: "#" },
        { text: qsTrc("dorico", "Double sharp"), value: "##" }
    ]

    function accidentalIndex() {
        for (var i = 0; i < accidentals.length; ++i) {
            if (accidentals[i].value === accidental) {
                return i
            }
        }
        return 2
    }

    function accept() {
        root.ret = { errcode: 0, value: [mode, kind, letter, accidental, octave, chord].join(",") }
        root.hide()
    }

    contentWidth: content.implicitWidth
    contentHeight: content.implicitHeight
    margins: 16

    modal: true

    Component.onCompleted: {
        var f = last.split(",")
        if (f.length >= 6) {
            mode = f[0]
            kind = f[1]
            letter = parseInt(f[2])
            accidental = f[3]
            octave = parseInt(f[4])
            chord = f[5]
        }
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 12

        NavigationPanel {
            id: contentNavigationPanel
            name: "InvertPitchesPanel"
            enabled: content.enabled && content.visible
            section: root.navigationSection
            order: 1
            direction: NavigationPanel.Vertical
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            StyledTextLabel {
                text: qsTrc("dorico", "Invert using:")
                horizontalAlignment: Text.AlignLeft
            }

            FlatRadioButtonList {
                Layout.preferredWidth: 420

                navigationPanel: contentNavigationPanel
                navigationRowStart: 0
                accessibleName: qsTrc("dorico", "Invert using")

                currentValue: root.mode
                model: [
                    { text: qsTrc("dorico", "Outermost pitches"), value: "outer" },
                    { text: qsTrc("dorico", "Specific pitch"), value: "pitch" },
                    { text: qsTrc("dorico", "Root"), value: "root" },
                    { text: qsTrc("dorico", "First note"), value: "first" }
                ]

                onToggled: function(newValue) {
                    root.mode = newValue
                }
            }

            StyledTextLabel {
                text: qsTrc("dorico", "Inversion type:")
                horizontalAlignment: Text.AlignLeft
            }

            FlatRadioButtonList {
                Layout.preferredWidth: 200
                enabled: root.mode !== "outer"

                navigationPanel: contentNavigationPanel
                navigationRowStart: 10
                accessibleName: qsTrc("dorico", "Inversion type")

                currentValue: root.kind
                model: [
                    { text: qsTrc("dorico", "Diatonic"), value: "diat" },
                    { text: qsTrc("dorico", "Chromatic"), value: "chrom" }
                ]

                onToggled: function(newValue) {
                    root.kind = newValue
                }
            }

            StyledTextLabel {
                text: qsTrc("dorico", "Specific pitch:")
                horizontalAlignment: Text.AlignLeft
            }

            RowLayout {
                enabled: root.mode === "pitch"
                spacing: 8

                StyledDropdown {
                    Layout.preferredWidth: 72
                    navigation.panel: contentNavigationPanel
                    navigation.row: 20
                    model: root.noteNames.map(function(n) { return { text: n, value: n } })
                    currentIndex: root.letter
                    onActivated: function(index, value) { root.letter = index }
                }

                StyledDropdown {
                    Layout.preferredWidth: 140
                    navigation.panel: contentNavigationPanel
                    navigation.row: 21
                    model: root.accidentals
                    currentIndex: root.accidentalIndex()
                    onActivated: function(index, value) { root.accidental = root.accidentals[index].value }
                }

                StyledTextLabel {
                    text: qsTrc("dorico", "at octave:")
                }

                IncrementalPropertyControl {
                    Layout.preferredWidth: 80
                    navigation.panel: contentNavigationPanel
                    navigation.row: 22
                    currentValue: root.octave
                    step: 1
                    decimals: 0
                    minValue: -1
                    maxValue: 9
                    onValueEdited: function(newValue) { root.octave = newValue }
                }
            }

            StyledTextLabel {
                text: qsTrc("dorico", "When first note is a chord, use:")
                horizontalAlignment: Text.AlignLeft
            }

            FlatRadioButtonList {
                Layout.preferredWidth: 280
                enabled: root.mode === "first"

                navigationPanel: contentNavigationPanel
                navigationRowStart: 30
                accessibleName: qsTrc("dorico", "When first note is a chord, use")

                currentValue: root.chord
                model: [
                    { text: qsTrc("dorico", "Top note of chord"), value: "top" },
                    { text: qsTrc("dorico", "Bottom note of chord"), value: "bottom" }
                ]

                onToggled: function(newValue) {
                    root.chord = newValue
                }
            }
        }

        ButtonBox {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            buttons: [ ButtonBoxModel.Cancel ]

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 2

            FlatButton {
                text: qsTrc("global", "OK")
                buttonRole: ButtonBoxModel.AcceptRole
                buttonId: ButtonBoxModel.Ok
                accentButton: true

                onClicked: root.accept()
            }

            onStandardButtonClicked: function(buttonId) {
                if (buttonId === ButtonBoxModel.Cancel) {
                    root.reject()
                }
            }
        }
    }
}
