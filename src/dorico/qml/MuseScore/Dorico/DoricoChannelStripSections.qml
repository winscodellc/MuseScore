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
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import MuseScore.Playback

//! Dorico's channel strip (spec 63), above each mixer channel: Inserts (MuseScore's four insert FX slots), EQ (no
//! MuseScore equivalent, a disabled header) and Sends (MuseScore's aux sends). A header click opens / closes that
//! section on every channel, so the open state lives in DoricoMixer. The Inserts dot is off with no effect loaded,
//! blue when one is enabled and yellow when all are bypassed; clicking it enables / bypasses them all.
Rectangle {
    id: root

    property MixerChannelItem channelItem: null
    property bool insertsOpen: true
    property bool sendsOpen: true

    signal toggleInserts()
    signal toggleSends()

    readonly property bool isFx: root.channelItem && root.channelItem.type === MixerChannelItem.Aux
    readonly property bool isOutput: root.channelItem && root.channelItem.type === MixerChannelItem.Master

    // 0: no inserts loaded, 1: at least one enabled, 2: all bypassed
    readonly property int insertState: {
        const list = root.channelItem ? root.channelItem.outputResourceItemList : []
        let loaded = 0
        let active = 0
        for (let i = 0; i < list.length; ++i) {
            if (!list[i].isBlank) {
                ++loaded
                if (list[i].isActive) {
                    ++active
                }
            }
        }
        return loaded === 0 ? 0 : (active > 0 ? 1 : 2)
    }

    // Colours measured on the manual's mixer_inserts_indicator_* pictures
    readonly property color dotOff: "#EEF4FA"
    readonly property color dotOn: "#6FADDD"
    readonly property color dotBypassed: "#FAE94F"
    readonly property color headerColor: "#20252B"
    readonly property color slotLoadedColor: "#1E86C6"

    function setAllInsertsActive(active) {
        const list = root.channelItem.outputResourceItemList
        for (let i = 0; i < list.length; ++i) {
            if (!list[i].isBlank) {
                list[i].isActive = active
            }
        }
    }

    radius: 4
    clip: true
    gradient: Gradient {
        GradientStop { position: 0.0; color: root.isFx ? "#2E4A38" : (root.isOutput ? "#4A2E30" : "#2E3339") }
        GradientStop { position: 1.0; color: root.isFx ? "#223A2B" : (root.isOutput ? "#3A2224" : "#22262B") }
    }
    border.width: 1
    border.color: "#3C4249"

    component SectionHeader: Rectangle {
        id: header

        property string title: ""
        property color dotColor: root.dotOff
        property string toolTip: ""
        // Unavailable headers stay enabled so their tooltip still shows
        property bool available: true

        signal headerClicked()
        signal dotClicked()

        width: parent ? parent.width : 0
        height: 22
        radius: 4
        color: root.headerColor
        opacity: header.available ? 1.0 : ui.theme.itemOpacityDisabled

        Rectangle {
            id: dot

            x: 6
            anchors.verticalCenter: parent.verticalCenter
            width: 9
            height: 9
            radius: 4.5
            color: header.dotColor
        }

        StyledTextLabel {
            anchors.left: dot.right
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            text: header.title
            font.pixelSize: 11
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            enabled: header.available
            onClicked: function(mouse) {
                if (mouse.x < dot.x + dot.width + 4) {
                    header.dotClicked()
                } else {
                    header.headerClicked()
                }
            }
        }

        HoverHandler {
            enabled: header.toolTip !== ""
            onHoveredChanged: {
                if (hovered) {
                    ui.tooltip.show(header, header.toolTip)
                } else {
                    ui.tooltip.hide(header)
                }
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 3
        visible: root.channelItem !== null

        SectionHeader {
            title: qsTrc("dorico", "INSERTS")
            dotColor: root.insertState === 1 ? root.dotOn : (root.insertState === 2 ? root.dotBypassed : root.dotOff)
            onHeaderClicked: root.toggleInserts()
            onDotClicked: {
                if (root.insertState !== 0) {
                    root.setAllInsertsActive(root.insertState === 2)
                }
            }
        }

        // Dorico always shows four slots; MuseScore lists the loaded effects plus one empty slot
        Repeater {
            model: root.insertsOpen ? 4 : 0

            Item {
                id: slot

                required property int index

                readonly property var fx: root.channelItem ? (root.channelItem.outputResourceItemList[slot.index] ?? null) : null
                readonly property bool loaded: slot.fx !== null && !slot.fx.isBlank
                // Enable / edit appear on hover, so the effect's name gets the whole width otherwise
                readonly property bool showButtons: slot.loaded && slotHover.hovered

                width: root.width - 8
                height: 20

                HoverHandler { id: slotHover }

                RowLayout {
                    anchors.fill: parent
                    spacing: 1

                    FlatButton {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        visible: slot.showButtons
                        icon: IconCode.BYPASS
                        transparent: !accentButton
                        accentButton: slot.loaded && slot.fx.isActive
                        toolTipTitle: qsTrc("dorico", "Enable insert")
                        onClicked: slot.fx.isActive = !slot.fx.isActive
                    }

                    DoricoButton {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        visible: slot.showButtons && slot.fx.hasNativeEditorSupport
                        doricoIcon: "svg:square-pen"
                        iconBoxSize: 12
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Edit insert")
                        onTriggered: slot.fx.requestToLaunchNativeEditorView()
                    }

                    ResourceDropdown {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 20
                        resourceItemModel: slot.fx
                        enabled: slot.fx !== null
                        color: slot.loaded ? (slot.fx.isActive ? root.slotLoadedColor : "#3A4048") : ui.theme.textFieldColor
                    }
                }
            }
        }

        SectionHeader {
            title: qsTrc("dorico", "EQ")
            available: false
            toolTip: qsTrc("dorico", "Not available")
        }

        SectionHeader {
            title: qsTrc("dorico", "SENDS")
            dotColor: {
                const sends = root.channelItem ? root.channelItem.auxSendItemList : []
                for (let i = 0; i < sends.length; ++i) {
                    if (sends[i].isActive) {
                        return root.dotOn
                    }
                }
                return root.dotOff
            }
            onHeaderClicked: root.toggleSends()
            onDotClicked: root.toggleSends()
        }

        // One slot per shown FX channel: MuseScore's send control (amount knob, on / off button with the FX name)
        Repeater {
            model: root.sendsOpen && root.channelItem ? root.channelItem.auxSendItemList : []

            AuxSendControl {
                required property AuxSendItem modelData

                width: root.width - 8
                height: 22
                auxSendItemModel: modelData
            }
        }
    }
}
