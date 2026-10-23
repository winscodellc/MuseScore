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

//! Dorico's Mixer (specs 57, 63) over MuseScore's mixer model, so volume, pan, mute, solo, inserts and sends are
//! MuseScore's. Each column is a channel strip (DoricoChannelStripSections: Inserts, EQ, Sends) above its channel
//! (DoricoChannelStrip), split by a draggable line; the strips hide when the zone is too short for them. The click
//! (metronome) channel comes first and Output is pinned right; the model order is left alone, because MuseScore's
//! mixer inserts channels by position. The wheel and background drags scroll sideways.
//! `headerComponent` is the Mixer toolbar, which the lower zone puts in its tab row: channel type buttons (Instruments
//! and FX work; MIDI, Video and Unused have no MuseScore channels), Deactivate all mute / solo, Add / Remove Effects
//! Channel (MuseScore's two aux channels), and unavailable Live Stage and Live Space.
Item {
    id: root

    property NavigationSection navigationSection: null
    // Shared with the Play inspector (spec 96), owned by DoricoShellPage
    property MixerPanelModel mixerModel: null
    property DoricoMixerModel mixerData: null
    property int contentNavigationPanelOrderStart: 1

    property bool showInstruments: true
    property bool showFx: true

    // Strip sections open on every channel at once, as in Dorico
    property bool insertsOpen: false
    property bool sendsOpen: false

    // Channel height below the split line; Dorico's channels are about 190 px
    property real channelHeight: 190
    readonly property real splitHeight: 4
    readonly property real channelAreaHeight: Math.min(root.channelHeight, root.height - 8)
    readonly property real stripAreaHeight: root.height - 8 - root.channelAreaHeight - root.splitHeight
    // The strips show only when their three 22 px headers fit (DoricoChannelStripSections)
    readonly property bool showStrips: root.stripAreaHeight >= 3 * 22 + 2 * 3 + 8
    readonly property real channelShownHeight: root.showStrips ? root.channelAreaHeight : root.height - 8

    // Shown columns in display order: { channel, slot } with slot = the instrument's colour slot (secondary
    // instruments share their part's), -1 for other channels
    property var rows: []
    property int instrumentCount: 0
    property int fxCount: 0
    // The master channel: pinned at the right edge, as in Dorico, instead of scrolling with the others
    property MixerChannelItem masterChannel: null

    function rebuild() {
        if (!root.mixerModel) {
            return
        }
        const rows = []
        let metronome = null
        let primaries = 0
        let fx = 0
        let master = null
        for (let i = 0; i < root.mixerModel.rowCount(); ++i) {
            const channel = root.mixerModel.get(i).channelItem
            const type = channel.type
            if (type === MixerChannelItem.PrimaryInstrument) {
                ++primaries
            } else if (type === MixerChannelItem.Aux) {
                ++fx
            } else if (type === MixerChannelItem.Master) {
                master = channel
                continue
            }
            if (!root.shows(channel)) {
                continue
            }
            const row = {
                "channel": channel,
                "slot": type === MixerChannelItem.PrimaryInstrument || type === MixerChannelItem.SecondaryInstrument
                        ? Math.max(0, primaries - 1) : -1
            }
            if (type === MixerChannelItem.Metronome) {
                metronome = row
            } else {
                rows.push(row)
            }
        }
        if (metronome) {
            rows.unshift(metronome) // Dorico lists the click first
        }
        root.instrumentCount = primaries
        root.fxCount = fx
        root.masterChannel = master
        // A new model scrolls the view back to the first channel: keep where it was (FX+/-, an added instrument)
        const x = channels.contentX
        root.rows = rows
        Qt.callLater(function() {
            const maxX = channels.originX + Math.max(0, channels.contentWidth - channels.width)
            channels.contentX = Math.max(channels.originX, Math.min(maxX, x))
        })
    }

    function shows(channel) {
        return channel.type === MixerChannelItem.Aux ? root.showFx : root.showInstruments
    }

    onShowInstrumentsChanged: root.rebuild()
    onShowFxChanged: root.rebuild()

    // Re-evaluated whenever a channel's mute / solo changes (the binding reads every channel's property)
    readonly property bool anyMuted: {
        if (!root.mixerModel) { // until the shell page's shared model is built
            return false
        }
        for (let i = 0; i < root.mixerModel.count; ++i) {
            const channel = root.mixerModel.get(i).channelItem
            if (channel.muted && !channel.forceMute) {
                return true
            }
        }
        return false
    }
    readonly property bool anySolo: {
        if (!root.mixerModel) {
            return false
        }
        for (let i = 0; i < root.mixerModel.count; ++i) {
            if (root.mixerModel.get(i).channelItem.solo) {
                return true
            }
        }
        return false
    }

    function deactivateAll(mute) {
        for (let i = 0; i < root.mixerModel.count; ++i) {
            const channel = root.mixerModel.get(i).channelItem
            if (mute) {
                channel.muted = false
            } else {
                channel.solo = false
            }
        }
    }

    Connections {
        target: root.mixerModel
        function onModelReset() { Qt.callLater(root.rebuild) }
        function onRowCountChanged() { Qt.callLater(root.rebuild) }
    }
    // Meter fall for every strip (spec 96). A meter still falling sets busy; the clock stops on a frame where none did,
    // so nothing redraws when the levels are still.
    FrameAnimation {
        id: meterClock
        property bool busy: false
        onTriggered: {
            if (!meterClock.busy) {
                meterClock.stop()
            }
            meterClock.busy = false
        }
    }

    // The shared model arrives after this view, or already holds its channels
    onMixerModelChanged: Qt.callLater(root.rebuild)
    Component.onCompleted: Qt.callLater(root.rebuild)

    NavigationPanel {
        id: toolbarNav
        name: "DoricoMixerToolbar"
        section: root.navigationSection
        order: root.contentNavigationPanelOrderStart
        direction: NavigationPanel.Horizontal
    }

    // Centred on the Mixer (not on the space right of the lower zone's tab buttons), as in Dorico
    property Component headerComponent: Item {
        id: header

        Row {
            id: toolbarRow

            // mapToItem is not a binding dependency; header.width and root.width make it follow resizes
            x: Math.max(0, Math.min(header.width - toolbarRow.width,
                                    root.width / 2 - header.mapToItem(root, 0, 0).x - toolbarRow.width / 2))
            anchors.verticalCenter: parent.verticalCenter
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

            // Lit in Dorico's mute yellow / solo red (DoricoChannelStrip)
            FlatButton {
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                text: "m"
                textFont.bold: true
                toolTipTitle: qsTrc("dorico", "Deactivate all mute states")
                accentButton: root.anyMuted
                accentColor: "#FFD608"
                iconColor: accentButton ? "#000000" : ui.theme.fontPrimaryColor
                transparent: !root.anyMuted
                navigation.panel: toolbarNav
                navigation.order: 10
                onClicked: root.deactivateAll(true)
            }

            FlatButton {
                anchors.verticalCenter: parent.verticalCenter
                width: 26
                height: 26
                text: "s"
                textFont.bold: true
                toolTipTitle: qsTrc("dorico", "Deactivate all solo states")
                accentButton: root.anySolo
                accentColor: "#A32A0A"
                iconColor: accentButton ? "#FFFFFF" : ui.theme.fontPrimaryColor
                transparent: !root.anySolo
                navigation.panel: toolbarNav
                navigation.order: 11
                onClicked: root.deactivateAll(false)
            }

            Item { width: 12; height: 1 }

            // MuseScore has two FX (aux) channels: Add shows the next hidden one, Remove hides the rightmost
            FlatButton {
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 26
                text: "FX+"
                textFont.bold: true
                transparent: true
                enabled: Boolean(root.mixerData) && root.fxCount < root.mixerData.fxChannelMax
                toolTipTitle: qsTrc("dorico", "Add Effects Channel")
                navigation.panel: toolbarNav
                navigation.order: 20
                onClicked: root.mixerData.addFxChannel()
            }

            FlatButton {
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 26
                text: "FX−"
                textFont.bold: true
                transparent: true
                enabled: Boolean(root.mixerData) && root.fxCount > 0
                toolTipTitle: qsTrc("dorico", "Remove Effects Channel")
                navigation.panel: toolbarNav
                navigation.order: 21
                onClicked: root.mixerData.removeFxChannel()
            }

            Item { width: 12; height: 1 }

            // No Live Stage / Live Space in MuseScore (spec 64)
            Repeater {
                model: [
                    { "title": qsTrc("dorico", "Live Stage"), "icon": "svg:rainbow" },
                    { "title": qsTrc("dorico", "Live Space"), "icon": "svg:waves" }
                ]

                DoricoButton {
                    required property var modelData
                    required property int index

                    anchors.verticalCenter: parent.verticalCenter
                    doricoIcon: modelData.icon
                    iconBoxSize: 18
                    available: false
                    transparent: true
                    toolTipTitle: modelData.title
                    navigation.panel: toolbarNav
                    navigation.order: 30 + index
                }
            }
        }
    }

    // One column: the channel strip above the channel
    component ChannelColumn: Column {
        id: column

        property MixerChannelItem channelItem: null
        property color labelColor: "#B8B8B8"

        width: 80
        spacing: root.splitHeight

        DoricoChannelStripSections {
            width: column.width
            height: root.stripAreaHeight
            visible: root.showStrips

            channelItem: column.channelItem
            insertsOpen: root.insertsOpen
            sendsOpen: root.sendsOpen
            onToggleInserts: root.insertsOpen = !root.insertsOpen
            onToggleSends: root.sendsOpen = !root.sendsOpen
        }

        DoricoChannelStrip {
            width: column.width
            height: root.channelShownHeight

            channelItem: column.channelItem
            labelColor: column.labelColor
            names: root.mixerData
            meterClock: meterClock
        }
    }

    ListView {
        id: channels

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: masterColumn.left
        anchors.margins: 4

        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.rows

        delegate: ChannelColumn {
            required property var modelData

            channelItem: modelData.channel
            labelColor: modelData.slot >= 0 ? TrackColor.trackColor(modelData.slot, root.instrumentCount) : "#B8B8B8"
        }

        // Dorico scrolls the channels sideways with the vertical wheel or a swipe in either direction
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: function(event) {
                const pixels = event.pixelDelta.x !== 0 || event.pixelDelta.y !== 0
                const dx = pixels ? event.pixelDelta.x : event.angleDelta.x / 2
                const dy = pixels ? event.pixelDelta.y : event.angleDelta.y / 2
                const delta = Math.abs(dx) > Math.abs(dy) ? dx : dy
                const maxX = channels.originX + Math.max(0, channels.contentWidth - channels.width)
                channels.contentX = Math.max(channels.originX, Math.min(maxX, channels.contentX - delta))
            }
        }

        ScrollBar.horizontal: StyledScrollBar {}

        // Channels cut by the edges fade out, as in Dorico. Parented to the view itself: a child declared in a
        // ListView goes into its contentItem and would scroll away with the channels.
        Rectangle {
            parent: channels
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 24
            visible: !channels.atXBeginning
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: ui.theme.backgroundPrimaryColor }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Rectangle {
            parent: channels
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 24
            visible: !channels.atXEnd
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: ui.theme.backgroundPrimaryColor }
            }
        }
    }

    ChannelColumn {
        id: masterColumn

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        width: root.masterChannel ? 80 : 0
        visible: root.masterChannel !== null

        channelItem: root.masterChannel
    }

    // Split line between the strips and the channels: drag it up or down
    MouseArea {
        x: 0
        y: root.showStrips ? 4 + root.stripAreaHeight : 0
        width: root.width
        height: root.splitHeight
        cursorShape: Qt.SplitVCursor
        preventStealing: true

        onPositionChanged: function(mouse) {
            if (pressed) {
                const y = mapToItem(root, mouse.x, mouse.y).y
                root.channelHeight = Math.max(140, Math.min(root.height - 8, root.height - 4 - y))
            }
        }
    }
}
