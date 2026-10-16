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


//! Setup mode (spec 55): Players panel (left), Layouts panel (right), Flows panel (lower), each with Dorico's action
//! bar. Players is a Dorico-style list (PlayersList) over MuseScore's instrument model.
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

                PlayersList {
                    id: players

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    navigationSection: playersPanel.navigationSection
                    navigationOrderStart: playersPanel.contentNavigationPanelOrderStart

                    // The panel's "…" menu: stave sharing (it was a switch at the top of MuseScore's instrument tree)
                    DoricoToolsModel {
                        id: playersMenu
                        codes: ["command://dorico/toggle-stave-sharing"]
                        Component.onCompleted: {
                            playersMenu.load()
                            playersPanel.contextMenuModel = playersMenu
                        }
                    }
                }

                SeparatorLine {}

                NavigationPanel {
                    id: playersActionsNav
                    name: "DoricoPlayersActions"
                    section: playersPanel.navigationSection
                    order: playersPanel.contentNavigationPanelOrderStart + 10
                    direction: NavigationPanel.Horizontal
                }

                // One action bar at the bottom: Dorico's add buttons, then MuseScore's Add System Markings and Remove
                // (rows are reordered by drag and drop). Wraps when narrow.
                Flow {
                    Layout.fillWidth: true
                    Layout.margins: 8
                    spacing: 4

                    ToolSection {
                        wrap: false
                        iconSize: 20 // all eight buttons on one row in the 300 px panel, as in Dorico

                        navigationPanel: playersActionsNav

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

                    Repeater {
                        model: [
                            { "title": qsTrc("dorico", "Add System Markings"), "icon": "svg:milestone", "action": "markings" },
                            { "title": qsTrc("dorico", "Remove"), "icon": "svg:trash-2", "action": "remove" }
                        ]

                        DoricoButton {
                            required property var modelData
                            required property int index

                            readonly property var treeModel: players.treeModel

                            doricoIcon: modelData.icon
                            iconBoxSize: 20
                            transparent: true
                            enabled: modelData.action === "markings" ? treeModel.isAddingSystemMarkingsAvailable
                                                                     : treeModel.isRemovingAvailable

                            toolTipTitle: modelData.title
                            navigation.panel: playersActionsNav
                            navigation.order: 100 + index
                            navigation.accessible.name: modelData.title

                            onTriggered: {
                                if (modelData.action === "markings") {
                                    treeModel.addSystemMarkings()
                                } else {
                                    treeModel.removeSelectedRows()
                                }
                            }
                        }
                    }
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

            // A MuseScore project is one score = one flow: one card filling the panel, as Dorico shows a single flow
            // (colour measured on Dorico 6), with the flow actions in a column on the right.
            RowLayout {
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 6

                    radius: 3
                    color: "#30678B"

                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: 8
                        text: qsTrc("dorico", "Flow 1")
                        font: ui.theme.bodyFont
                        color: "#FFFFFF"
                    }

                    // The flow is in every layout (MuseScore has one flow), so its layout checkbox is always ticked
                    CheckBox {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 6
                        checked: true
                        enabled: false
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 8
                        text: "1"
                        font: ui.theme.bodyFont
                        color: "#FFFFFF"
                    }

                    StyledTextLabel {
                        anchors.centerIn: parent
                        text: setupModel.projectTitle
                        opacity: 0.6
                    }
                }

                ToolSection {
                    vertical: true
                    iconSize: 18
                    Layout.rightMargin: 4
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 6

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
