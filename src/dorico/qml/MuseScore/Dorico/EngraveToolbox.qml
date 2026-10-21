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

//! Engrave mode's toolbox (spec 56): Show Left Zone, then the tools that choose the left-zone panel.
Column {
    id: toolColumn

    property string tool: ""
    property NavigationSection navigationSection: null

    signal toolSelected(string toolId)

    spacing: 2
    padding: 4

    readonly property var tools: [
        { "id": "", "title": qsTrc("dorico", "Show Left Zone"), "icon": "svg:panel-left", "available": true },
        { "id": "formatting", "title": qsTrc("dorico", "Graphic Editing"), "icon": "svg:pen-tool", "available": true },
        { "id": "frames", "title": qsTrc("dorico", "Frames"), "icon": "svg:frame", "available": true },
        { "id": "staffSpacing", "title": qsTrc("dorico", "Staff Spacing"), "icon": "svg:move-vertical", "available": false },
        { "id": "noteSpacing", "title": qsTrc("dorico", "Note Spacing"), "icon": "svg:move-horizontal", "available": false },
        { "id": "slices", "title": qsTrc("dorico", "Graphic Slices"), "icon": "svg:crop", "available": true }
    ]

    NavigationPanel {
        id: toolsNav
        name: "DoricoEngraveToolbox"
        section: toolColumn.navigationSection
        order: 1
        direction: NavigationPanel.Vertical
        enabled: toolColumn.enabled && toolColumn.visible
    }

    DoricoTopBarModel {
        id: toolboxDispatcher
    }

    Repeater {
        model: toolColumn.tools

        DoricoButton {
            required property var modelData
            required property int index

            readonly property bool isActive: modelData.id !== "" && toolColumn.tool === modelData.id

            doricoIcon: modelData.icon
            iconBoxSize: 28
            available: modelData.available
            accentButton: isActive
            transparent: !isActive
            toolTipTitle: modelData.title

            navigation.panel: toolsNav
            navigation.order: index
            navigation.accessible.name: modelData.title

            onTriggered: {
                if (modelData.id === "") {
                    toolboxDispatcher.dispatch("command://dorico/toggle-zone-left")
                } else {
                    toolColumn.toolSelected(modelData.id)
                }
            }
        }
    }
}
