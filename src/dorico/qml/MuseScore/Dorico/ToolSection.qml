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

//! A titled group of tool buttons (used by every Dorico toolbox, panel and action bar). `codes` are command URIs or
//! legacy action codes; enabled/checked follow the command state (DoricoToolsModel). `labels`/`icons` run parallel
//! to `codes`: an entry with an icon is drawn as an icon button, otherwise as a text button. A code nobody handles
//! yet is an unavailable button with a "Not available yet" tooltip.
Column {
    id: root

    property string title: ""
    property alias codes: tools.codes
    property var labels: []
    property var icons: []

    // Lay the buttons out in one column (toolboxes) instead of rows.
    property bool vertical: false
    // Wrap rows at the section's width (panels). Off: one row/column sized to its buttons (action bars, toolboxes).
    property bool wrap: !vertical

    property NavigationPanel navigationPanel: null
    property int navigationOrderStart: 0

    spacing: 6

    DoricoToolsModel {
        id: tools
        Component.onCompleted: tools.load()
    }

    StyledTextLabel {
        visible: root.title !== ""
        text: root.title
        font: ui.theme.bodyBoldFont
    }

    Flow {
        id: flow

        // Without an explicit width a Flow does not wrap and takes its content's size.
        Binding on width {
            when: root.wrap
            value: root.width
        }

        flow: root.vertical ? Flow.TopToBottom : Flow.LeftToRight
        spacing: 4

        Repeater {
            model: tools.items

            DoricoButton {
                id: button

                required property var modelData
                required property int index

                readonly property string label: root.labels[index] ?? modelData.title
                readonly property int iconCode: root.icons[index] ?? IconCode.NONE

                visible: modelData.id !== ""
                icon: iconCode
                text: iconCode === IconCode.NONE ? label : ""
                available: modelData.enabled
                accentButton: modelData.checked
                transparent: !modelData.checked

                toolTipTitle: label
                // Placeholders (unregistered commands) carry "Not available yet" as their description.
                toolTipDescription: modelData.description
                toolTipShortcut: modelData.shortcuts

                navigation.panel: root.navigationPanel
                navigation.order: root.navigationOrderStart + index
                navigation.accessible.name: label

                onTriggered: tools.handleMenuItem(modelData.id)
            }
        }
    }
}
