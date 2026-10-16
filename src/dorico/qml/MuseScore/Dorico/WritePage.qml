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
import Muse.Dock
import MuseScore.AppShell

import MuseScore.NotationScene
import MuseScore.Palette
import MuseScore.PropertiesPanel
import MuseScore.Playback

//! Write mode. `uri` is MuseScore's own notation page URI, so the Score tab and project opening need no change.
//! Left: Notes toolbox + Notes panel (spec 52). Right: Notations toolbox + Notations panel (spec 53), plus MuseScore's
//! Palettes, Selection filter and History in the same zone (closed by default). Lower: LowerZonePanel (spec 54).
//! The instruments panel lives in Setup mode.
DoricoPage {
    id: root

    objectName: "DoricoWrite"
    uri: "musescore://notation"

    onInited: {
        Qt.callLater(pageModel.init)
    }

    readonly property string rightPanelsGroup: "RIGHT_PANELS"

    // The Dorico panel shown in the right zone (spec 53); the Notations toolbox chooses it.
    property DoricoPanelModel notationsModel: DoricoPanelModel {}

    function showNotationsPanel(panelId) {
        // Clicking the active button closes the zone, like Dorico.
        if (root.notationsModel.panelId === panelId && root.notationsModel.isPanelOpen()) {
            root.notationsModel.setPanelOpen(false)
            return
        }
        root.notationsModel.panelId = panelId
        root.notationsModel.setPanelOpen(true)
    }


    toolBars: [
        DockToolBar {
            id: notesToolbox

            objectName: "notesToolbox"
            title: qsTrc("dorico", "Notes toolbox")

            // A non-floatable toolbar keeps the orientation it is given (DockToolBarView::canChangeOrientation).
            location: Location.Left
            orientation: Qt.Vertical
            floatable: false
            closable: false
            resizable: false
            thickness: 48

            navigationSection: root.noteInputKeyNavSec

            NotesToolbox {
                navigationPanel.section: notesToolbox.navigationSection
                navigationPanel.order: 1
            }
        },

        DockToolBar {
            id: notationsToolbox

            objectName: "notationsToolbox"
            title: qsTrc("dorico", "Notations toolbox")

            location: Location.Right
            orientation: Qt.Vertical
            floatable: false
            closable: false
            resizable: false
            thickness: 48

            navigationSection: root.noteInputKeyNavSec

            NotationsToolbox {
                navigationPanel.section: notationsToolbox.navigationSection
                navigationPanel.order: 2

                currentPanelId: root.notationsModel.panelId

                onPanelClicked: function(panelId) {
                    root.showNotationsPanel(panelId)
                }
            }
        }
    ]

    panels: [
        DockPanel {
            id: notesPanel

            objectName: "notesPanel"
            title: qsTrc("dorico", "Notes")

            navigationSection: root.navigationPanelSec(notesPanel.location)

            width: 132
            minimumWidth: 100
            maximumWidth: 300
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left

            // No title bar, as in Dorico (a panel that can neither float nor close gets none)
            floatable: false
            closable: false
            dropDestinations: root.verticalPanelDropDestinations

            NotesPanel {
                navigationSection: notesPanel.navigationSection
                navigationOrderStart: notesPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: notationsPanel

            objectName: "notationsPanel"
            title: qsTrc("dorico", "Notations")

            navigationSection: root.navigationPanelSec(notationsPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: 200
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default; a Notations toolbox button opens it
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            NotationsPanel {
                panelModel: root.notationsModel
                navigationSection: notationsPanel.navigationSection
                navigationOrderStart: notationsPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: palettesPanel

            objectName: root.pageModel.palettesPanelName()
            title: qsTrc("appshell", "Palettes")

            navigationSection: root.navigationPanelSec(palettesPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default; the Notations panel is the Dorico way in
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            PalettesPanel {
                navigationSection: palettesPanel.navigationSection
                navigationOrderStart: palettesPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    palettesPanel.contextMenuModel = contextMenuModel
                }
            }
        },

        DockPanel {
            id: selectionFilterPanel

            objectName: root.pageModel.selectionFiltersPanelName()
            title: qsTrc("appshell", "Selection filter")

            navigationSection: root.navigationPanelSec(selectionFilterPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            SelectionFilterPanel {
                navigationSection: selectionFilterPanel.navigationSection
                navigationOrderStart: selectionFilterPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: undoHistoryPanel

            objectName: root.pageModel.undoHistoryPanelName()
            title: qsTrc("notation", "History")

            navigationSection: root.navigationPanelSec(undoHistoryPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            UndoHistoryPanel {
                navigationSection: undoHistoryPanel.navigationSection
                navigationOrderStart: undoHistoryPanel.contentNavigationPanelOrderStart
            }
        },

        LowerZonePanel {
            id: lowerZonePanel

            notationView: root.notationView

            navigationSection: root.navigationPanelSec(lowerZonePanel.location)

            dropDestinations: root.horizontalPanelDropDestinations
        }
    ]
}
