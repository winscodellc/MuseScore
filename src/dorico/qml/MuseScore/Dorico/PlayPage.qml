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
import Muse.Dock

import MuseScore.Playback

import "trackcolor.js" as TrackColor

//! Play mode (spec 57): left zone (Track Inspector / VST and MIDI), the track overview in the centre,
//! lower zone (Mixer, Key Editor). No toolbox and no right zone (Cmd-9 does nothing here).
DoricoPage {
    id: root

    objectName: "DoricoPlay"
    uri: "musescore://dorico/play"

    statusItems: ["pitch", "summary", "zoneArrow", "zoom", "midi"]

    // The track overview (centre) and the Track Inspector (left zone content) live in separate components, so they
    // talk through the page: the inspector publishes its track, the overview asks for another one.
    property string inspectedPartId: ""
    signal trackPicked(string partId)

    // Dorico shows a track overview instead of the music. Clicking a track shows it in the Track Inspector.
    central: TrackOverview {
        navigationSection: root.keynavTopPanelSec
        contentNavigationPanelOrderStart: 1

        selectedPartId: root.inspectedPartId
        onTrackClicked: function(partId) { root.trackPicked(partId) }
    }

    panels: [
        DockPanel {
            id: playLeftPanel

            objectName: "playLeftPanel"
            title: qsTrc("dorico", "Play")

            navigationSection: root.navigationPanelSec(playLeftPanel.location)

            width: 240
            minimumWidth: 200
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left
            dropDestinations: root.verticalPanelDropDestinations

            // No title bar, as in Dorico's left zone
            floatable: false
            closable: false

            Item {
                id: leftContent

                property bool showInspector: true

                NavigationPanel {
                    id: leftNav
                    name: "DoricoPlayLeftZone"
                    section: playLeftPanel.navigationSection
                    order: playLeftPanel.contentNavigationPanelOrderStart
                    direction: NavigationPanel.Both
                    enabled: leftContent.enabled && leftContent.visible
                }

                // Dorico's flow selector. MuseScore has one flow per score, so it is a disabled "Flow 1".
                RowLayout {
                    id: flowRow

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 4
                    spacing: 4

                    StyledTextLabel { text: qsTrc("dorico", "Flow:") }

                    StyledDropdown {
                        Layout.fillWidth: true
                        enabled: false
                        model: [{ "text": qsTrc("dorico", "Flow 1"), "value": 0 }]
                        currentIndex: 0
                    }
                }

                RowLayout {
                    id: selector

                    anchors.top: flowRow.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 4
                    spacing: 4

                    FlatButton {
                        Layout.fillWidth: true
                        text: qsTrc("dorico", "Track Inspector")
                        accentButton: leftContent.showInspector
                        transparent: !leftContent.showInspector
                        navigation.panel: leftNav
                        navigation.order: 0
                        onClicked: leftContent.showInspector = true
                    }

                    FlatButton {
                        Layout.fillWidth: true
                        text: qsTrc("dorico", "VST and MIDI")
                        accentButton: !leftContent.showInspector
                        transparent: leftContent.showInspector
                        navigation.panel: leftNav
                        navigation.order: 1
                        onClicked: leftContent.showInspector = false
                    }
                }

                SeparatorLine {
                    id: selectorLine
                    anchors.top: selector.bottom
                    anchors.topMargin: 4
                }

                MixerPanelModel {
                    id: mixerModel

                    navigationSection: playLeftPanel.navigationSection
                    navigationOrderStart: playLeftPanel.contentNavigationPanelOrderStart + 1
                }

                TrackInspectorModel {
                    id: inspector

                    sourceModel: mixerModel
                    Component.onCompleted: inspector.load()
                    onChannelChanged: root.inspectedPartId = inspector.partId
                }

                Connections {
                    target: root
                    function onTrackPicked(partId) { inspector.partId = partId }
                }

                StyledFlickable {
                    anchors.top: selectorLine.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    visible: leftContent.showInspector
                    contentHeight: inspectorColumn.implicitHeight + 8

                    // Dorico's Track Inspector: Routing, Insert Effects and Channel. MuseScore has a sound per channel
                    // and up to four effects; ports, MIDI channels, endpoints, expression maps and independent voice
                    // playback have no MuseScore equivalent, so those controls are shown but unavailable.
                    Column {
                        id: inspectorColumn

                        readonly property MixerChannelItem channel: inspector.channelItem
                        readonly property bool hasChannel: channel !== null

                        width: parent.width
                        spacing: 2

                        InspectorSection {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Routing")

                            ResourceDropdown {
                                width: parent.width
                                resourceItemModel: inspectorColumn.hasChannel ? inspectorColumn.channel.inputResourceItem : null
                            }

                            RowLayout {
                                width: parent.width
                                spacing: 4

                                StyledTextLabel { text: qsTrc("dorico", "Port:") }

                                TextInputField {
                                    Layout.preferredWidth: 36
                                    currentText: "–" // no MuseScore ports
                                    enabled: false
                                }

                                StyledTextLabel { text: qsTrc("dorico", "Ch.:") }

                                TextInputField {
                                    Layout.preferredWidth: 36
                                    currentText: "–" // no MuseScore MIDI channels per track
                                    enabled: false
                                }

                                Item { Layout.fillWidth: true }

                                DoricoButton {
                                    doricoIcon: "svg:settings"
                                    iconBoxSize: 16
                                    transparent: true
                                    available: false
                                    toolTipTitle: qsTrc("dorico", "Endpoint Setup")
                                }

                                DoricoButton {
                                    doricoIcon: "svg:square-pen"
                                    iconBoxSize: 16
                                    transparent: true
                                    toolTipTitle: qsTrc("dorico", "Edit Instrument")
                                    available: inspectorColumn.hasChannel && inspectorColumn.channel.inputResourceItem
                                               && inspectorColumn.channel.inputResourceItem.hasNativeEditorSupport
                                    onTriggered: inspectorColumn.channel.inputResourceItem.requestToLaunchNativeEditorView()
                                }
                            }

                            RowLayout {
                                width: parent.width
                                spacing: 4

                                StyledTextLabel { text: qsTrc("dorico", "Ex. map:") }

                                StyledDropdown {
                                    Layout.fillWidth: true
                                    enabled: false
                                    model: [{ "text": qsTrc("dorico", "Default"), "value": 0 }]
                                    currentIndex: 0
                                }

                                DoricoButton {
                                    doricoIcon: "svg:move-up-right"
                                    iconBoxSize: 16
                                    transparent: true
                                    available: false
                                    toolTipTitle: qsTrc("dorico", "Edit Expression Map")
                                }
                            }

                            DoricoButton {
                                text: qsTrc("dorico", "Enable Independent Voice Playback")
                                available: false
                            }
                        }

                        InspectorSection {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Insert Effects")

                            Repeater {
                                model: 4

                                RowLayout {
                                    id: fxRow

                                    required property int index

                                    // MuseScore lists the channel's effects plus one empty slot
                                    readonly property var fx: inspectorColumn.hasChannel
                                                              ? (inspectorColumn.channel.outputResourceItemList[fxRow.index] ?? null) : null

                                    width: parent.width
                                    spacing: 2

                                    FlatButton {
                                        Layout.preferredWidth: 22
                                        Layout.preferredHeight: 22
                                        icon: IconCode.BYPASS
                                        transparent: true
                                        accentButton: fxRow.fx !== null && !fxRow.fx.isBlank && fxRow.fx.isActive
                                        enabled: fxRow.fx !== null && !fxRow.fx.isBlank
                                        toolTipTitle: qsTrc("dorico", "Activate/deactivate effect")
                                        onClicked: fxRow.fx.isActive = !fxRow.fx.isActive
                                    }

                                    ResourceDropdown {
                                        Layout.fillWidth: true
                                        resourceItemModel: fxRow.fx
                                    }

                                    DoricoButton {
                                        doricoIcon: "svg:square-pen"
                                        iconBoxSize: 14
                                        transparent: true
                                        toolTipTitle: qsTrc("dorico", "Edit effect")
                                        available: fxRow.fx !== null && !fxRow.fx.isBlank && fxRow.fx.hasNativeEditorSupport
                                        onTriggered: fxRow.fx.requestToLaunchNativeEditorView()
                                    }
                                }
                            }
                        }

                        InspectorSection {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Channel")

                            DoricoChannelStrip {
                                anchors.horizontalCenter: parent.horizontalCenter
                                height: 240
                                channelItem: inspectorColumn.channel
                                labelColor: TrackColor.trackColor(inspector.colorIndex, inspector.colorCount)
                            }
                        }
                    }
                }

                DoricoPlaceholder {
                    anchors.top: selectorLine.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right

                    visible: !leftContent.showInspector
                    text: qsTrc("dorico", "VST and MIDI instruments are not available yet. Sounds and effects are set per channel in the Track Inspector and the Mixer.")
                }
            }
        },

        LowerZonePanel {
            id: lowerZonePanel

            objectName: root.dockName("lowerZonePanel")

            tabs: ["mixer", "keyEditor"]
            notationView: root.notationView

            navigationSection: root.navigationPanelSec(lowerZonePanel.location)

            dropDestinations: root.horizontalPanelDropDestinations
        }
    ]
}
