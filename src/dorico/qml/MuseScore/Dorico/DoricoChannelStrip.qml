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

//! One compact Dorico-style mixer channel (spec 57), used by DoricoMixer and the Track Inspector's Channel section:
//! pan box, fader value, mute / solo on the left, fader, a stereo meter on the right, and the channel name on a
//! label in the instrument's colour. FX channels are tinted green.
Rectangle {
    id: root

    property MixerChannelItem channelItem: null
    property color labelColor: ui.theme.strokeColor
    property string label: root.channelItem ? root.channelItem.title : ""

    readonly property bool isFx: root.channelItem && root.channelItem.type === MixerChannelItem.Aux
    readonly property bool isOutput: root.channelItem && root.channelItem.type === MixerChannelItem.Master

    // Sizes and colours measured on Dorico 6's mixer (80 px strips, blue readouts in dark boxes)
    readonly property color readoutColor: "#5AAEEA"
    readonly property color boxColor: "#15191E"

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

    // Pan: drag sideways, double-click centres
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
            property real startX: 0
            property int startBalance: 0

            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            enabled: root.channelItem && !root.channelItem.hasBalanceAutomation

            onPressed: function(mouse) {
                startX = mouse.x
                startBalance = root.channelItem.balance
            }
            onPositionChanged: function(mouse) {
                const value = Math.round(startBalance + (mouse.x - startX))
                root.channelItem.balance = Math.max(root.channelItem.balanceMin, Math.min(root.channelItem.balanceMax, value))
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
                normalColor: "#3A4048"
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
                normalColor: "#3A4048"
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
                    height: parent.height
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
    }
}
