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

import MuseScore.Playback

import "mixervalues.js" as MixerValues

//! One compact Dorico-style mixer channel (spec 57), used by DoricoMixer and the Track Inspector's Channel section:
//! pan box, fader value, mute / solo on the left, fader, a stereo meter on the right, and the channel name on a
//! label in the instrument's colour. FX channels are tinted green. Spec 63: double-click the name to rename it (kept in
//! the score, see DoricoMixerModel); Opt-click the pan or fader value to type one, Cmd-click or double-click resets it.
Rectangle {
    id: root

    property MixerChannelItem channelItem: null
    property color labelColor: ui.theme.strokeColor
    property string label: !root.channelItem ? "" : (names.names[names.keyOf(root.channelItem)] ?? root.channelItem.title)

    readonly property bool isFx: root.channelItem && root.channelItem.type === MixerChannelItem.Aux
    readonly property bool isOutput: root.channelItem && root.channelItem.type === MixerChannelItem.Master

    // Sizes and colours measured on Dorico 6's mixer (80 px strips, blue readouts in dark boxes)
    readonly property color readoutColor: "#5AAEEA"
    readonly property color boxColor: "#15191E"
    // Lit mute / solo, measured on the manual's mixer_button_*_activated pictures (spec 63)
    readonly property color muteLitColor: "#FFD608"
    readonly property color soloLitColor: "#A32A0A"

    width: 80
    radius: 4
    gradient: Gradient {
        GradientStop { position: 0.0; color: root.isFx ? "#2E4A38" : (root.isOutput ? "#4A2E30" : "#2E3339") }
        GradientStop { position: 1.0; color: root.isFx ? "#223A2B" : (root.isOutput ? "#3A2224" : "#22262B") }
    }
    border.width: 1
    border.color: "#3C4249"

    // Fader travel as in MuseScore's VolumeSlider: finer steps above -12 dB.
    QtObject {
        id: fader

        readonly property real from: -60
        readonly property real to: 12

        function toLocal(level) {
            return level > -12 ? fader.to - (fader.to - level) * 1.5 : -24 - (-12 - level) * 0.75
        }
        function fromLocal(local) {
            return local > -24 ? fader.to - (fader.to - local) / 1.5 : -12 - (-24 - local) / 0.75
        }
    }

    function panText(balance) {
        return balance === 0 ? "C" : (balance < 0 ? "L" + (-balance) : "R" + balance)
    }

    DoricoMixerModel {
        id: names
        Component.onCompleted: names.load()
    }

    // The inline field over the pan box, the fader value or the name: "pan", "volume" or "name" while typing
    property string editTarget: ""

    function startEdit(target) {
        const item = target === "pan" ? panBox : (target === "volume" ? valueBox : nameLabel)
        const pos = item.mapToItem(root, 0, 0)
        editor.x = pos.x
        editor.y = pos.y
        editor.width = item.width
        editor.height = item.height
        editor.currentText = target === "pan" ? root.panText(root.channelItem.balance)
                                              : (target === "volume" ? root.channelItem.volumeLevel.toFixed(2) : root.label)
        root.editTarget = target
        Qt.callLater(editor.ensureActiveFocus)
    }

    function commitEdit(text) {
        if (root.editTarget === "pan") {
            const value = MixerValues.parsePan(text, root.channelItem.balanceMin, root.channelItem.balanceMax)
            if (value !== null) {
                root.channelItem.balance = value
            }
        } else if (root.editTarget === "volume") {
            const value = MixerValues.parseVolume(text, root.channelItem.volumeLevelMin, root.channelItem.volumeLevelMax)
            if (value !== null) {
                root.channelItem.volumeLevel = value
            }
        } else if (root.editTarget === "name") {
            names.setName(names.keyOf(root.channelItem), text)
        }
    }

    // Pan: click a position or drag sideways; Opt-click types a value, Cmd-click or double-click centres
    Rectangle {
        id: panBox

        anchors.top: parent.top
        anchors.topMargin: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        height: 20
        radius: 3
        color: root.boxColor
        visible: root.channelItem !== null

        Text {
            anchors.centerIn: parent
            text: root.channelItem ? root.panText(root.channelItem.balance) : ""
            font.family: ui.theme.bodyFont.family
            font.pixelSize: 12
            color: root.readoutColor
        }

        MouseArea {
            property bool dragging: false

            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            preventStealing: true // the mixer scrolls on background drags
            enabled: root.channelItem && !root.channelItem.hasBalanceAutomation

            // The outer 6 px at each end give the full L100 / R100
            function setFromX(x) {
                const min = root.channelItem.balanceMin
                const max = root.channelItem.balanceMax
                const value = Math.round(min + (max - min) * (x - 6) / (panBox.width - 12))
                root.channelItem.balance = Math.max(min, Math.min(max, value))
            }

            onPressed: function(mouse) {
                dragging = false
                if (mouse.modifiers & Qt.AltModifier) {
                    root.startEdit("pan")
                } else if (mouse.modifiers & Qt.ControlModifier) { // Cmd on macOS
                    root.channelItem.balance = 0
                } else {
                    dragging = true
                    setFromX(mouse.x)
                }
            }
            onPositionChanged: function(mouse) {
                if (dragging) {
                    setFromX(mouse.x)
                }
            }
            onDoubleClicked: root.channelItem.balance = 0
        }
    }

    // Fader value (dB)
    Rectangle {
        id: valueBox

        anchors.top: panBox.bottom
        anchors.topMargin: 6
        anchors.right: parent.right
        anchors.rightMargin: 6
        width: 52
        height: 18
        radius: 3
        color: root.boxColor
        visible: root.channelItem !== null

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            text: root.channelItem ? root.channelItem.volumeLevel.toFixed(2) : ""
            font.family: ui.theme.bodyFont.family
            font.pixelSize: 11
            color: root.readoutColor
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.channelItem && !root.channelItem.hasVolumeAutomation

            onClicked: function(mouse) {
                if (mouse.modifiers & Qt.AltModifier) {
                    root.startEdit("volume")
                } else if (mouse.modifiers & Qt.ControlModifier) { // Cmd on macOS
                    root.channelItem.volumeLevel = 0
                }
            }
            onDoubleClicked: root.channelItem.volumeLevel = 0
        }
    }

    Item {
        id: body

        anchors.top: valueBox.bottom
        anchors.topMargin: 6
        anchors.bottom: nameLabel.top
        anchors.bottomMargin: 6
        anchors.left: parent.left
        anchors.right: parent.right
        visible: root.channelItem !== null

        Column {
            id: muteSolo

            x: 6
            y: 4
            spacing: 6

            FlatButton {
                width: 26
                height: 26
                text: "m"
                textFont.pixelSize: 15
                textFont.bold: true
                normalColor: accentButton ? accentColor : "#3A4048" // a fixed colour would hide the lit state
                accentColor: root.muteLitColor
                iconColor: accentButton ? "#000000" : ui.theme.fontPrimaryColor
                toolTipTitle: qsTrc("dorico", "Mute")
                accentButton: root.channelItem ? root.channelItem.muted : false
                onClicked: root.channelItem.muted = !root.channelItem.muted
            }

            FlatButton {
                width: 26
                height: 26
                text: "s"
                textFont.pixelSize: 15
                textFont.bold: true
                normalColor: accentButton ? accentColor : "#3A4048" // a fixed colour would hide the lit state
                accentColor: root.soloLitColor
                iconColor: accentButton ? "#FFFFFF" : ui.theme.fontPrimaryColor
                toolTipTitle: qsTrc("dorico", "Solo")
                // As MuseScore's mixer: no solo on the output and the click, and FX channels cannot be soloed
                visible: !root.isOutput && !(root.channelItem && root.channelItem.type === MixerChannelItem.Metronome)
                enabled: !root.isFx
                accentButton: root.channelItem ? root.channelItem.solo : false
                onClicked: root.channelItem.solo = !root.channelItem.solo
            }
        }

        // Fader: groove plus cap; drag the cap or click the groove, double-click resets to 0 dB
        Item {
            id: faderArea

            anchors.left: muteSolo.right
            anchors.leftMargin: 6
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 24

            readonly property real travel: height - cap.height
            readonly property real position: root.channelItem
                                             ? (fader.toLocal(root.channelItem.volumeLevel) - fader.from) / (fader.to - fader.from) : 0

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 5
                radius: 2
                color: "#0E1114"
            }

            Rectangle {
                id: cap

                anchors.horizontalCenter: parent.horizontalCenter
                y: faderArea.travel * (1 - faderArea.position)
                width: 22
                height: 40
                radius: 3
                gradient: Gradient {
                    GradientStop { position: 0.0; color: root.isOutput ? "#F07A72" : "#FAFAFA" }
                    GradientStop { position: 1.0; color: root.isOutput ? "#C0443E" : "#BDBDBD" }
                }
                border.color: "#6E6E6E"

                // Grip lines
                Column {
                    anchors.centerIn: parent
                    spacing: 3

                    Repeater {
                        model: 4

                        Rectangle {
                            width: 14
                            height: 1
                            color: root.isOutput ? "#8A2E2A" : "#8C8C8C"
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                preventStealing: true // the mixer scrolls on background drags
                enabled: root.channelItem && !root.channelItem.hasVolumeAutomation

                function setFromY(y) {
                    const position = 1 - Math.max(0, Math.min(1, (y - cap.height / 2) / faderArea.travel))
                    const level = fader.fromLocal(fader.from + position * (fader.to - fader.from))
                    root.channelItem.volumeLevel = Math.round(level * 10) / 10
                }

                onPressed: function(mouse) { setFromY(mouse.y) }
                onPositionChanged: function(mouse) { setFromY(mouse.y) }
                onDoubleClicked: root.channelItem.volumeLevel = 0
            }
        }

        // Stereo meter, -60..0 dBFS
        Row {
            anchors.left: faderArea.right
            anchors.leftMargin: 4
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            spacing: 1

            Repeater {
                model: 2

                Rectangle {
                    id: meter

                    required property int index

                    readonly property real pressure: !root.channelItem ? -60
                                                     : (meter.index === 0 ? root.channelItem.leftChannelPressure
                                                                          : root.channelItem.rightChannelPressure)

                    width: 4
                    height: body.height
                    color: "#0E1114"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: parent.height * Math.max(0, Math.min(1, (meter.pressure + 60) / 60))
                        color: meter.pressure >= 0 ? "#E0605A" : "#4CC36A"
                    }
                }
            }
        }
    }

    // Channel name on the instrument colour
    Rectangle {
        id: nameLabel

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 3
        height: 18
        radius: 3
        color: root.labelColor

        Text {
            anchors.fill: parent
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: root.isOutput ? qsTrc("dorico", "Output") : root.label
            font.family: ui.theme.tabFont.family
            font.pixelSize: 11
            font.bold: true
            color: "#0C0C0C"
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.channelItem !== null && !root.isOutput
            onDoubleClicked: root.startEdit("name")
        }
    }

    TextInputField {
        id: editor

        visible: root.editTarget !== ""
        textSidePadding: 4
        textHorizontalAlignment: Qt.AlignHCenter

        onAccepted: root.commitEdit(editor.inputField.text)

        // Return and Esc both drop the focus; so does clicking elsewhere, which cancels
        Connections {
            target: editor.inputField
            function onActiveFocusChanged() {
                if (!editor.inputField.activeFocus) {
                    root.editTarget = ""
                }
            }
        }
    }
}
