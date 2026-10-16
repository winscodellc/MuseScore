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
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents

import MuseScore.Playback

import "trackcolor.js" as TrackColor

//! Dorico's Mixer (spec 57): a row of compact channel strips (DoricoChannelStrip) over MuseScore's mixer model, so
//! volume, pan, mute and solo are MuseScore's. `headerComponent` is the Mixer toolbar, which the lower zone puts in
//! its tab row: channel type buttons (Instruments and FX work; MIDI, Video and Unused have no MuseScore channels),
//! Deactivate all mute / solo, and unavailable Add / Remove Effects Channel, Live Stage and Live Space.
Item {
    id: root

    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 1

    property bool showInstruments: true
    property bool showFx: true

    // Colour slot of each row: the n-th instrument (secondary instruments share their part's slot)
    property var colorSlots: []
    property int instrumentCount: 0

    function updateColorSlots() {
        const slots = []
        let primaries = 0
        for (let i = 0; i < mixerModel.rowCount(); ++i) {
            const type = mixerModel.get(i).channelItem.type
            if (type === MixerChannelItem.PrimaryInstrument) {
                ++primaries
            }
            slots.push(type === MixerChannelItem.PrimaryInstrument || type === MixerChannelItem.SecondaryInstrument
                       ? Math.max(0, primaries - 1) : -1)
        }
        root.instrumentCount = primaries
        root.colorSlots = slots
    }

    function shows(channel) {
        switch (channel.type) {
        case MixerChannelItem.Aux: return root.showFx
        case MixerChannelItem.Master: return true
        default: return root.showInstruments
        }
    }

    // Re-evaluated whenever a channel's mute / solo changes (the binding reads every channel's property)
    readonly property bool anyMuted: {
        for (let i = 0; i < mixerModel.count; ++i) {
            const channel = mixerModel.get(i).channelItem
            if (channel.muted && !channel.forceMute) {
                return true
            }
        }
        return false
    }
    readonly property bool anySolo: {
        for (let i = 0; i < mixerModel.count; ++i) {
            if (mixerModel.get(i).channelItem.solo) {
                return true
            }
        }
        return false
    }

    function deactivateAll(mute) {
        for (let i = 0; i < mixerModel.count; ++i) {
            const channel = mixerModel.get(i).channelItem
            if (mute) {
                channel.muted = false
            } else {
                channel.solo = false
            }
        }
    }

    MixerPanelModel {
        id: mixerModel

        navigationSection: root.navigationSection
        navigationOrderStart: root.contentNavigationPanelOrderStart + 1

        onModelReset: Qt.callLater(root.updateColorSlots)
        onRowCountChanged: Qt.callLater(root.updateColorSlots)
    }

    NavigationPanel {
        id: toolbarNav
        name: "DoricoMixerToolbar"
        section: root.navigationSection
        order: root.contentNavigationPanelOrderStart
        direction: NavigationPanel.Horizontal
    }

    property Component headerComponent: Row {
        spacing: 4

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTrc("dorico", "Show:")
        }

        Repeater {
            model: [
                { "title": qsTrc("dorico", "Instruments"), "kind": "instruments" },
                { "title": qsTrc("dorico", "MIDI"), "kind": "" },
                { "title": qsTrc("dorico", "Video"), "kind": "" },
                { "title": qsTrc("dorico", "FX"), "kind": "fx" },
                { "title": qsTrc("dorico", "Unused"), "kind": "" }
            ]

            DoricoButton {
                required property var modelData
                required property int index

                anchors.verticalCenter: parent.verticalCenter
                height: 26
                minWidth: 0
                margins: 8
                textFont.bold: true
                text: modelData.title
                available: modelData.kind !== ""
                accentButton: modelData.kind === "instruments" ? root.showInstruments
                                                               : modelData.kind === "fx" ? root.showFx : false
                transparent: !accentButton
                navigation.panel: toolbarNav
                navigation.order: index

                onTriggered: {
                    if (modelData.kind === "instruments") {
                        root.showInstruments = !root.showInstruments
                    } else {
                        root.showFx = !root.showFx
                    }
                }
            }
        }

        Item { width: 12; height: 1 }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTrc("dorico", "Deactivate all:")
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 22
            text: "m"
            toolTipTitle: qsTrc("dorico", "Deactivate all mute states")
            accentButton: root.anyMuted
            transparent: !root.anyMuted
            navigation.panel: toolbarNav
            navigation.order: 10
            onClicked: root.deactivateAll(true)
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 22
            text: "s"
            toolTipTitle: qsTrc("dorico", "Deactivate all solo states")
            accentButton: root.anySolo
            transparent: !root.anySolo
            navigation.panel: toolbarNav
            navigation.order: 11
            onClicked: root.deactivateAll(false)
        }

        Item { width: 12; height: 1 }

        // MuseScore has a fixed set of FX (aux) channels and no Live Stage / Live Space
        Repeater {
            model: [
                { "title": qsTrc("dorico", "Add Effects Channel"), "text": "FX+", "icon": "" },
                { "title": qsTrc("dorico", "Remove Effects Channel"), "text": "FX−", "icon": "" },
                { "title": qsTrc("dorico", "Live Stage"), "text": "", "icon": "svg:rainbow" },
                { "title": qsTrc("dorico", "Live Space"), "text": "", "icon": "svg:waves" }
            ]

            DoricoButton {
                required property var modelData
                required property int index

                anchors.verticalCenter: parent.verticalCenter
                text: modelData.text
                doricoIcon: modelData.icon !== "" ? modelData.icon : IconCode.NONE
                iconBoxSize: 18
                available: false
                transparent: true
                toolTipTitle: modelData.title
                navigation.panel: toolbarNav
                navigation.order: 20 + index
            }
        }
    }

    ListView {
        id: channels

        anchors.fill: parent
        anchors.margins: 4

        orientation: ListView.Horizontal
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: mixerModel

        delegate: Item {
            id: cell

            required property MixerChannelItem channelItem
            required property int index

            readonly property bool shown: root.shows(cell.channelItem)

            width: cell.shown ? 88 : 0
            height: ListView.view.height
            visible: cell.shown

            DoricoChannelStrip {
                anchors.fill: parent
                anchors.rightMargin: 8

                channelItem: cell.channelItem
                labelColor: (root.colorSlots[cell.index] ?? -1) >= 0
                            ? TrackColor.trackColor(root.colorSlots[cell.index], root.instrumentCount)
                            : "#B8B8B8"
            }
        }

        ScrollBar.horizontal: StyledScrollBar {}
    }
}
