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
//! to `codes` (IconCode, SMuFL glyph or "svg:<name>", see DoricoButton): an entry with an icon is drawn as an icon button, otherwise as a text button. A code nobody handles
//! yet is an unavailable button with a "Not available yet" tooltip.
Column {
    id: root

    property string title: ""
    property alias codes: tools.codes
    property var labels: []
    property var icons: []
    property real iconSize: 24

    // Lay the buttons out in one column (toolboxes) instead of rows.
    property bool vertical: false
    // Icon + label buttons, one per line, on a filled background (Dorico's Engrave Formatting panel)
    property bool labelled: false
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

        flow: root.vertical || root.labelled ? Flow.TopToBottom : Flow.LeftToRight
        spacing: 4

        Repeater {
            model: tools.items

            DoricoButton {
                id: button

                required property var modelData
                required property int index

                readonly property string labelText: root.labels[index] ?? modelData.title
                readonly property var iconSpec: root.icons[index] ?? IconCode.NONE

                visible: modelData.id !== ""
                readonly property bool customIcon: typeof iconSpec === "string" && iconSpec !== ""

                doricoIcon: iconSpec
                iconBoxSize: root.labelled ? 16 : root.iconSize
                // Labelled: a custom icon gets the label beside it; an IconCode icon uses FlatButton's own icon + text
                label: root.labelled && button.customIcon ? button.labelText : ""
                text: iconSpec === IconCode.NONE || (root.labelled && !button.customIcon) ? button.labelText : ""
                orientation: root.labelled ? Qt.Horizontal : Qt.Vertical
                minWidth: root.labelled ? 0 : (isNarrow ? 24 : 132) // labelled buttons fit their text
                margins: root.labelled ? 8 : (isNarrow ? 12 : 16)
                height: root.labelled ? 26 : implicitHeight
                available: modelData.enabled
                accentButton: modelData.checked
                transparent: !modelData.checked && !root.labelled

                toolTipTitle: button.labelText
                // Placeholders (unregistered commands) carry "Not available yet" as their description.
                toolTipDescription: modelData.description
                toolTipShortcut: modelData.shortcuts

                navigation.panel: root.navigationPanel
                navigation.order: root.navigationOrderStart + index
                navigation.accessible.name: button.labelText

                onTriggered: tools.handleMenuItem(modelData.id)
            }
        }
    }
}
