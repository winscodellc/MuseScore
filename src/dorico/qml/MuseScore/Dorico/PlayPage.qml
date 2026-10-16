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
                        text: qsTrc("dorico", "Track Inspector")
                        accentButton: leftContent.showInspector
                        transparent: !leftContent.showInspector
                        navigation.panel: leftNav
                        navigation.order: 0
                        onClicked: leftContent.showInspector = true
                    }

                    FlatButton {
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
                    contentHeight: inspectorColumn.implicitHeight + 16

                    Column {
                        id: inspectorColumn

                        x: 8
                        y: 8
                        width: parent.width - 16
                        spacing: 8

                        StyledTextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            text: inspector.trackTitle !== "" ? inspector.trackTitle : qsTrc("dorico", "Select an instrument")
                            font: ui.theme.bodyBoldFont
                        }

                        // Dorico's collapsible sections. Routing: MuseScore has a sound (instrument) picker per channel;
                        // ports, MIDI channels and endpoints do not exist.
                        ExpandableBlank {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Routing")
                            isExpanded: true
                            navigation.panel: leftNav
                            navigation.order: 2

                            contentItemComponent: MixerSoundSection {
                                headerVisible: false
                                channelItemWidth: inspectorColumn.width
                                model: inspector
                            }
                        }

                        ExpandableBlank {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Insert Effects")
                            isExpanded: false
                            navigation.panel: leftNav
                            navigation.order: 3

                            contentItemComponent: MixerFxSection {
                                headerVisible: false
                                channelItemWidth: inspectorColumn.width
                                model: inspector
                            }
                        }

                        ExpandableBlank {
                            width: inspectorColumn.width
                            title: qsTrc("dorico", "Channel")
                            isExpanded: false
                            navigation.panel: leftNav
                            navigation.order: 4

                            contentItemComponent: Column {
                                width: inspectorColumn.width
                                spacing: 8

                                MixerBalanceSection {
                                    headerVisible: false
                                    channelItemWidth: inspectorColumn.width
                                    model: inspector
                                }

                                MixerVolumeSection {
                                    headerVisible: false
                                    channelItemWidth: inspectorColumn.width
                                    model: inspector
                                }

                                MixerFaderSection {
                                    headerVisible: false
                                    channelItemWidth: inspectorColumn.width
                                    model: inspector
                                }

                                MixerMuteAndSoloSection {
                                    headerVisible: false
                                    channelItemWidth: inspectorColumn.width
                                    model: inspector
                                }
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
