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

//! Setup mode's Players panel (spec 55): a Dorico-style list (PlayersList) over MuseScore's instrument model, with
//! Dorico's action bar at the bottom.
ColumnLayout {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    spacing: 0

    NavigationPanel {
        id: headerNav
        name: "DoricoPlayersHeader"
        section: root.navigationSection
        order: root.navigationOrderStart
        enabled: root.enabled && root.visible
    }

    ZoneHeader {
        Layout.fillWidth: true

        title: qsTrc("dorico", "Players")
        navigationPanel: headerNav

        // The panel's "…" menu: stave sharing (it was a switch at the top of MuseScore's instrument tree)
        menuModel: DoricoToolsModel {
            id: playersMenu
            codes: ["command://dorico/toggle-stave-sharing"]
            Component.onCompleted: playersMenu.load()
        }
    }

    PlayersList {
        id: players

        Layout.fillWidth: true
        Layout.fillHeight: true

        navigationSection: root.navigationSection
        navigationOrderStart: root.navigationOrderStart + 1
    }

    SeparatorLine {}

    NavigationPanel {
        id: playersActionsNav
        name: "DoricoPlayersActions"
        section: root.navigationSection
        order: root.navigationOrderStart + 10
        direction: NavigationPanel.Horizontal
        enabled: root.enabled && root.visible
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
