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
import MuseScore.Palette

//! The titled palette grids of a DoricoPanelModel (Notations panels, spec 53; Engrave bracketing, spec 56).
//! Each grid is MuseScore's own PaletteGridView, so clicking a cell applies it exactly like the palette.
Column {
    id: root

    required property DoricoPanelModel panelModel
    property NavigationPanel navigationPanel: null
    property int navigationRowStart: 0
    // Off when a single section already sits under its own header (Engrave's Bracketing)
    property bool showTitles: true

    spacing: 12

    Repeater {
        model: root.panelModel.sectionTitles

        Column {
            id: section

            required property int index
            required property string modelData

            readonly property size cell: root.panelModel.cellSize(index)

            width: root.width
            spacing: 4

            StyledTextLabel {
                visible: root.showTitles
                text: section.modelData
                font: ui.theme.bodyBoldFont
            }

            PaletteGridView {
                readonly property int columns: Math.max(1, Math.floor(width / section.cell.width))

                width: parent.width
                height: Math.ceil(ncells / columns) * section.cell.height

                cellSize: section.cell
                drawGrid: root.panelModel.drawGrid(section.index)

                paletteModel: root.panelModel.paletteModel
                paletteRootIndex: root.panelModel.sectionIndex(section.index)
                paletteController: root.panelModel.paletteController

                navigationPanel: root.navigationPanel
                navigationRow: root.navigationRowStart + section.index
            }
        }
    }
}
