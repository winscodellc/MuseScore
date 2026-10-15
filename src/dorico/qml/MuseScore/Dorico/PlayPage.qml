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

import MuseScore.NotationScene
import MuseScore.Playback

//! Play mode (spec 57): left zone (Track Inspector / VST and MIDI), the timeline as the track overview in the centre,
//! lower zone (Mixer, Key Editor). No toolbox and no right zone (Cmd-9 does nothing here).
DoricoPage {
    id: root

    objectName: "DoricoPlay"
    uri: "musescore://dorico/play"

    statusItems: ["pitch", "summary", "zoneArrow", "zoom", "midi"]

    // Dorico shows a track overview instead of the music; MuseScore's timeline (one row per instrument) is the
    // closest thing. Selecting in it selects in the score, which the Track Inspector follows.
    central: Timeline {
        navigationSection: root.keynavTopPanelSec
        contentNavigationPanelOrderStart: 1
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

                RowLayout {
                    id: selector

                    anchors.top: parent.top
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

                        // Routing: MuseScore has a sound (instrument) picker per channel; ports, MIDI channels and
                        // endpoints do not exist.
                        StyledTextLabel { text: qsTrc("dorico", "Routing"); opacity: 0.7 }

                        MixerSoundSection {
                            headerVisible: false
                            channelItemWidth: inspectorColumn.width
                            model: inspector
                        }

                        StyledTextLabel { text: qsTrc("dorico", "Insert Effects"); opacity: 0.7 }

                        MixerFxSection {
                            headerVisible: false
                            channelItemWidth: inspectorColumn.width
                            model: inspector
                        }

                        StyledTextLabel { text: qsTrc("dorico", "Channel"); opacity: 0.7 }

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

            tabs: ["mixer", "keyEditor"]
            notationView: root.notationView

            navigationSection: root.navigationPanelSec(lowerZonePanel.location)

            dropDestinations: root.horizontalPanelDropDestinations
        }
    ]
}
