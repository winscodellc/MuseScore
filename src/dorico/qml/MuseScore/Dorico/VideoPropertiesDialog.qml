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

//! Video Properties (spec 91): the project frame rate, where the video attaches to the flow and its timecode.
//! Opens from the flow card's Video menu and the Video panel. With no video attached Dorico shows no file rows and
//! no "Use video frame rate" (phase B adds them).
StyledDialogView {
    id: root

    title: qsTrc("dorico", "Video Properties")

    contentWidth: content.implicitWidth
    contentHeight: content.implicitHeight
    margins: 16

    modal: true

    property int rateIndex: 0
    property int attachBeats: 0
    property int noteTicks: 480     // undotted beat unit
    property int dots: 0
    property string startOffset: ""
    property string timecodeStart: ""
    property string error: ""

    readonly property var dotFactor: [1, 1.5, 1.75]
    readonly property bool rateIsDrop: video.frameRates[root.rateIndex].indexOf("dfps") >= 0

    VideoModel {
        id: video
    }

    function accept() {
        var unit = Math.round(root.noteTicks * root.dotFactor[root.dots])
        root.error = video.applyProperties(root.rateIndex, root.attachBeats, unit, root.startOffset, root.timecodeStart)
        if (root.error === "") {
            root.ret = { errcode: 0, value: "" }
            root.hide()
        }
    }

    // The separator follows the chosen rate
    function withSeparator(tc) {
        return tc.substring(0, 8) + (root.rateIsDrop ? ";" : ":") + tc.substring(9)
    }

    Component.onCompleted: {
        video.load()
        root.rateIndex = video.rateIndex
        root.attachBeats = video.attachBeats
        // Split the stored unit into a note value and dots
        var notes = [120, 240, 480, 960, 1920]
        for (var d = 0; d < 3; ++d) {
            for (var i = 0; i < notes.length; ++i) {
                if (Math.round(notes[i] * root.dotFactor[d]) === video.attachUnit) {
                    root.noteTicks = notes[i]
                    root.dots = d
                }
            }
        }
        root.startOffset = video.startOffset
        root.timecodeStart = video.timecodeStart
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 12

        NavigationPanel {
            id: contentNavigationPanel
            name: "VideoPropertiesPanel"
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

            StyledTextLabel { text: qsTrc("dorico", "Project frame rate:"); horizontalAlignment: Text.AlignLeft }
            StyledDropdown {
                Layout.preferredWidth: 140
                navigation.panel: contentNavigationPanel
                navigation.row: 0
                model: video.frameRates.map(function(r, i) { return { text: r, value: i } })
                currentIndex: root.rateIndex
                onActivated: function(index, value) {
                    root.rateIndex = index
                    root.startOffset = root.withSeparator(root.startOffset)
                    root.timecodeStart = root.withSeparator(root.timecodeStart)
                    root.error = ""
                }
            }

            StyledTextLabel {
                Layout.alignment: Qt.AlignTop
                topPadding: 20
                text: qsTrc("dorico", "Flow attachment position:")
                horizontalAlignment: Text.AlignLeft
            }
            RowLayout {
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    StyledTextLabel { text: qsTrc("dorico", "Beat count:"); horizontalAlignment: Text.AlignLeft }
                    IncrementalPropertyControl {
                        Layout.preferredWidth: 72
                        navigation.panel: contentNavigationPanel
                        navigation.row: 1
                        currentValue: root.attachBeats
                        step: 1
                        decimals: 0
                        minValue: 0
                        maxValue: 9999
                        onValueEdited: function(newValue) { root.attachBeats = newValue }
                    }
                }

                ColumnLayout {
                    spacing: 4
                    StyledTextLabel { text: qsTrc("dorico", "Beat unit:"); horizontalAlignment: Text.AlignLeft }
                    RowLayout {
                        spacing: 6
                        FlatRadioButtonList {
                            Layout.preferredWidth: 5 * ui.theme.defaultButtonSize
                            navigationPanel: contentNavigationPanel
                            navigationRowStart: 10
                            accessibleName: qsTrc("dorico", "Beat unit")
                            iconFontSize: 16
                            currentValue: root.noteTicks
                            model: [
                                { iconCode: IconCode.NOTE_16TH, value: 120 },
                                { iconCode: IconCode.NOTE_8TH, value: 240 },
                                { iconCode: IconCode.NOTE_QUARTER, value: 480 },
                                { iconCode: IconCode.NOTE_HALF, value: 960 },
                                { iconCode: IconCode.NOTE_WHOLE, value: 1920 }
                            ]
                            onToggled: function(newValue) { root.noteTicks = newValue }
                        }
                        // Single / double dot, both off = undotted
                        Repeater {
                            model: [ { iconCode: IconCode.NOTE_DOTTED, dots: 1 }, { iconCode: IconCode.NOTE_DOTTED_2, dots: 2 } ]
                            FlatButton {
                                required property var modelData
                                Layout.preferredWidth: ui.theme.defaultButtonSize
                                icon: modelData.iconCode
                                accentButton: root.dots === modelData.dots
                                navigation.panel: contentNavigationPanel
                                navigation.row: 20 + modelData.dots
                                onClicked: root.dots = root.dots === modelData.dots ? 0 : modelData.dots
                            }
                        }
                    }
                }
            }

            StyledTextLabel { text: qsTrc("dorico", "Video start offset:"); horizontalAlignment: Text.AlignLeft }
            DoricoTimecodeField {
                timecode: root.startOffset
                dropFrame: root.rateIsDrop
                framesPerSecond: Math.round(parseFloat(video.frameRates[root.rateIndex]))
                onEdited: function(tc) { root.startOffset = tc; root.error = "" }
                onAccepted: root.accept()
            }

            StyledTextLabel { text: qsTrc("dorico", "Timecode start:"); horizontalAlignment: Text.AlignLeft }
            DoricoTimecodeField {
                timecode: root.timecodeStart
                dropFrame: root.rateIsDrop
                framesPerSecond: Math.round(parseFloat(video.frameRates[root.rateIndex]))
                onEdited: function(tc) { root.timecodeStart = tc; root.error = "" }
                onAccepted: root.accept()
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            visible: root.error !== ""
            text: root.error
            color: "#E05050"
            horizontalAlignment: Text.AlignLeft
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
