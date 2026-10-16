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

import MuseScore.InstrumentsScene

//! Setup mode (spec 55): Players panel (left), Layouts panel (right), Flows panel (lower), each with Dorico's action
//! bar. Players reuses MuseScore's instruments panel (LayoutPanel), which keeps its own add/move/delete controls.
DoricoPage {
    id: root

    objectName: "DoricoSetup"
    uri: "musescore://dorico/setup"

    statusItems: ["order", "summary", "zoom", "midi"]

    DoricoTopBarModel {
        id: setupModel
        Component.onCompleted: setupModel.load()
    }

    panels: [
        DockPanel {
            id: playersPanel

            objectName: "playersPanel"
            title: qsTrc("dorico", "Players")

            navigationSection: root.navigationPanelSec(playersPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left
            dropDestinations: root.verticalPanelDropDestinations

            ColumnLayout {
                spacing: 0

                LayoutPanel {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    navigationSection: playersPanel.navigationSection
                    navigationOrderStart: playersPanel.contentNavigationPanelOrderStart

                    Component.onCompleted: {
                        playersPanel.contextMenuModel = contextMenuModel
                    }
                }

                SeparatorLine {}

                ToolSection {
                    Layout.margins: 8
                    wrap: false

                    navigationPanel: NavigationPanel {
                        name: "DoricoPlayersActions"
                        section: playersPanel.navigationSection
                        order: playersPanel.contentNavigationPanelOrderStart + 10
                        direction: NavigationPanel.Horizontal
                    }

                    // Add Single Player is MuseScore's Add instruments dialog; the rest has no MuseScore model.
                    codes: [
                        "command://instruments/select",
                        "command://dorico/add-section-player",
                        "command://dorico/add-ensemble",
                        "command://dorico/add-group",
                        "command://dorico/sort-players",
                        "command://dorico/player-settings"
                    ]
                    labels: [
                        qsTrc("dorico", "Add Single Player"),
                        qsTrc("dorico", "Add Section Player"),
                        qsTrc("dorico", "Add Ensemble"),
                        qsTrc("dorico", "Add Group"),
                        qsTrc("dorico", "Sort Players"),
                        qsTrc("dorico", "Player Settings")
                    ]
                    icons: ["svg:user-plus", "svg:users", "svg:boxes", "svg:folder-plus", "svg:arrow-down-wide-narrow", "svg:settings"]
                }
            }
        },

        DockPanel {
            id: layoutsPanel

            objectName: "layoutsPanel"
            title: qsTrc("dorico", "Layouts")

            navigationSection: root.navigationPanelSec(layoutsPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Right
            dropDestinations: root.verticalPanelDropDestinations

            LayoutsPanel {
                navigationSection: layoutsPanel.navigationSection
                navigationOrderStart: layoutsPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: flowsPanel

            objectName: "flowsPanel"
            title: qsTrc("dorico", "Flows")

            navigationSection: root.navigationPanelSec(flowsPanel.location)

            height: 130
            minimumHeight: 100
            maximumHeight: 300
            minimumWidth: root.panelMinDimension
            maximumWidth: root.panelMaxDimension

            location: Location.Bottom
            dropDestinations: root.horizontalPanelDropDestinations

            // A MuseScore project is one score = one flow, so there is exactly one read-only card.
            RowLayout {
                spacing: 8

                Rectangle {
                    Layout.leftMargin: 8
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 60

                    radius: 4
                    color: ui.theme.backgroundSecondaryColor
                    border.width: 1
                    border.color: ui.theme.strokeColor

                    Column {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 2

                        StyledTextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            text: setupModel.projectTitle
                            font: ui.theme.bodyBoldFont
                        }

                        StyledTextLabel {
                            text: qsTrc("dorico", "Flow 1")
                            opacity: 0.7
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                ToolSection {
                    wrap: false
                    Layout.rightMargin: 8
                    Layout.alignment: Qt.AlignBottom
                    Layout.bottomMargin: 8

                    navigationPanel: NavigationPanel {
                        name: "DoricoFlowsActions"
                        section: flowsPanel.navigationSection
                        order: flowsPanel.contentNavigationPanelOrderStart
                        direction: NavigationPanel.Horizontal
                    }

                    codes: [
                        "command://dorico/add-flow",
                        "command://dorico/notation-options",
                        "command://dorico/flow-settings",
                        "command://dorico/delete-flow"
                    ]
                    labels: [
                        qsTrc("dorico", "Add Flow"),
                        qsTrc("dorico", "Notation Options"),
                        qsTrc("dorico", "Flow Settings"),
                        qsTrc("dorico", "Delete Flow")
                    ]
                    icons: ["svg:plus", "svg:sliders-horizontal", "svg:settings", "svg:trash-2"]
                }
            }
        }
    ]
}
