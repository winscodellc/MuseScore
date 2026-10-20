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

//! Edit > Filter > Notes by Pitch (spec 45): a list of pitches to select. Answers "w:" or "s:" (written / sounding) and the
//! list as the tokens FilterNotesByPitch understands ("c4,eb,f#5"; no octave = every octave).
StyledDialogView {
    id: root

    property string pitches: ""
    property bool sounding: false

    property var entries: []
    property int selectedIndex: -1

    readonly property var noteNames: ["C", "D", "E", "F", "G", "A", "B"]
    readonly property var accidentals: [
        { text: qsTrc("dorico", "Double flat"), value: "bb" },
        { text: qsTrc("dorico", "Flat"), value: "b" },
        { text: qsTrc("dorico", "Natural"), value: "" },
        { text: qsTrc("dorico", "Sharp"), value: "#" },
        { text: qsTrc("dorico", "Double sharp"), value: "x" }
    ]

    function addEntry() {
        var token = noteNames[nameDropdown.currentIndex].toLowerCase() + accidentals[accidentalDropdown.currentIndex].value
        if (octaveCheck.checked) {
            token += Math.round(octaveField.currentValue)
        }
        if (entries.indexOf(token) < 0) {
            entries = entries.concat([token])
        }
        selectedIndex = entries.indexOf(token)
    }

    function removeSelected() {
        if (selectedIndex < 0) {
            return
        }
        var copy = entries.slice()
        copy.splice(selectedIndex, 1)
        entries = copy
        selectedIndex = -1
    }

    function describe(token) {
        var hasOctave = /[0-9]$/.test(token)
        return token.charAt(0).toUpperCase() + token.slice(1) + (hasOctave ? "" : "  " + qsTrc("dorico", "(every octave)"))
    }

    function accept() {
        root.ret = { errcode: 0, value: (soundingButtons.currentValue ? "s:" : "w:") + entries.join(",") }
        root.hide()
    }

    contentWidth: content.implicitWidth
    contentHeight: content.implicitHeight
    margins: 16

    modal: true

    Component.onCompleted: {
        entries = pitches.split(",").filter(function(s) { return s.length > 0 })
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 12

        NavigationPanel {
            id: contentNavigationPanel
            name: "FilterNotesByPitchPanel"
            enabled: content.enabled && content.visible
            section: root.navigationSection
            order: 1
            direction: NavigationPanel.Vertical
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: qsTrc("dorico", "Select notes with these pitches:")
            font: ui.theme.bodyBoldFont
            horizontalAlignment: Text.AlignLeft
        }

        FlatRadioButtonList {
            id: soundingButtons

            Layout.fillWidth: true
            Layout.preferredWidth: 320

            navigationPanel: contentNavigationPanel
            navigationRowStart: 0
            accessibleName: qsTrc("dorico", "Pitches")

            currentValue: root.sounding
            model: [
                { text: qsTrc("dorico", "Written pitches"), value: false },
                { text: qsTrc("dorico", "Sounding pitches"), value: true }
            ]

            onToggled: function(newValue) {
                currentValue = newValue
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140

            color: ui.theme.textFieldColor
            border.color: ui.theme.strokeColor
            border.width: 1

            ListView {
                id: pitchList

                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: root.entries

                delegate: ListItemBlank {
                    required property string modelData
                    required property int index

                    width: ListView.view.width
                    height: 24
                    isSelected: index === root.selectedIndex

                    StyledTextLabel {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        horizontalAlignment: Text.AlignLeft
                        text: root.describe(parent.modelData)
                    }

                    onClicked: root.selectedIndex = index
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            FlatButton {
                id: removeButton
                text: qsTrc("dorico", "Remove selected")
                enabled: root.selectedIndex >= 0
                navigation.panel: contentNavigationPanel
                navigation.row: 10
                onClicked: root.removeSelected()
            }

            FlatButton {
                id: clearButton
                text: qsTrc("dorico", "Clear")
                enabled: root.entries.length > 0
                navigation.panel: contentNavigationPanel
                navigation.row: 11
                onClicked: {
                    root.entries = []
                    root.selectedIndex = -1
                }
            }
        }

        SeparatorLine { Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledDropdown {
                id: nameDropdown
                Layout.preferredWidth: 72
                navigation.panel: contentNavigationPanel
                navigation.row: 20
                model: root.noteNames.map(function(n) { return { text: n, value: n } })
                currentIndex: 0
                onActivated: function(index, value) { currentIndex = index }
            }

            StyledDropdown {
                id: accidentalDropdown
                Layout.fillWidth: true
                navigation.panel: contentNavigationPanel
                navigation.row: 21
                model: root.accidentals
                currentIndex: 2
                onActivated: function(index, value) { currentIndex = index }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                id: octaveCheck
                text: qsTrc("dorico", "In octave")
                checked: false
                navigation.panel: contentNavigationPanel
                navigation.row: 30
                onClicked: checked = !checked
            }

            IncrementalPropertyControl {
                id: octaveField
                Layout.preferredWidth: 96
                enabled: octaveCheck.checked
                navigation.panel: contentNavigationPanel
                navigation.row: 31
                currentValue: 4
                step: 1
                decimals: 0
                minValue: -1
                maxValue: 9
                onValueEdited: function(newValue) { currentValue = newValue }
            }

            Item { Layout.fillWidth: true }

            FlatButton {
                text: qsTrc("dorico", "Add")
                navigation.panel: contentNavigationPanel
                navigation.row: 32
                onClicked: root.addEntry()
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
                enabled: root.entries.length > 0
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
