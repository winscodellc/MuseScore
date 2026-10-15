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

//! The right-zone panel of Write mode (spec 53): the Dorico panel chosen in the Notations toolbox. Each section
//! is MuseScore's palette grid over the cells DoricoPanelModel picked; clicking applies like the palette does.
//! Panels with no MuseScore backing (Cues, Video, Comments, Proofreading) show the "Not available yet" stub.
Item {
    id: root

    // Owned by the page, shared with the Notations toolbox that chooses the panel.
    required property DoricoPanelModel panelModel

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    NavigationPanel {
        id: navPanel
        name: "DoricoNotationsPanel"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Both
        enabled: root.enabled && root.visible
    }

    DoricoPlaceholder {
        anchors.fill: parent
        visible: root.panelModel.sectionTitles.length === 0
    }

    StyledFlickable {
        anchors.fill: parent
        visible: root.panelModel.sectionTitles.length > 0
        contentHeight: sectionsColumn.implicitHeight + 16

        Column {
            id: sectionsColumn

            x: 8
            y: 8
            width: parent.width - 16
            spacing: 12

            Repeater {
                model: root.panelModel.sectionTitles

                Column {
                    id: section

                    required property int index
                    required property string modelData

                    readonly property size cell: root.panelModel.cellSize(index)

                    width: sectionsColumn.width
                    spacing: 4

                    StyledTextLabel {
                        text: section.modelData
                        font: ui.theme.bodyBoldFont
                    }

                    PaletteGridView {
                        id: grid

                        readonly property int columns: Math.max(1, Math.floor(width / section.cell.width))

                        width: parent.width
                        height: Math.ceil(ncells / columns) * section.cell.height

                        cellSize: section.cell
                        drawGrid: root.panelModel.drawGrid(section.index)

                        paletteModel: root.panelModel.paletteModel
                        paletteRootIndex: root.panelModel.sectionIndex(section.index)
                        paletteController: root.panelModel.paletteController

                        navigationPanel: navPanel
                        navigationRow: section.index
                    }
                }
            }
        }
    }
}
