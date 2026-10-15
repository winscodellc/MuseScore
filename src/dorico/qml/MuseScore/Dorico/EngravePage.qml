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

//! Engrave mode (spec 56): Engrave toolbox (left edge) choosing the tool and the left-zone panel (Formatting, Frames,
//! Graphic Slices), the Pages panel (right, stub) and a lower zone with Properties and Mixer. The score is always in
//! page view here: DoricoActionsController switches to page view on the way in and restores the old view on the way
//! out, and this mode's status bar has no view-type control.
DoricoPage {
    id: root

    objectName: "DoricoEngrave"
    uri: "musescore://dorico/engrave"

    statusItems: ["grid", "pitch", "summary", "zoneArrow", "tools", "arrangement", "zoom", "midi"]

    // "formatting" (Graphic Editing), "frames" or "slices"; Staff/Note Spacing have no MuseScore equivalent.
    property string tool: "formatting"

    toolBars: [
        DockToolBar {
            id: engraveToolbox

            objectName: "engraveToolbox"
            title: qsTrc("dorico", "Engrave toolbox")

            // A non-floatable toolbar keeps the orientation it is given (DockToolBarView::canChangeOrientation).
            location: Location.Left
            orientation: Qt.Vertical
            floatable: false
            closable: false
            resizable: false
            thickness: 44

            navigationSection: root.noteInputKeyNavSec

            Column {
                id: toolColumn

                spacing: 2
                padding: 4

                readonly property var tools: [
                    { "id": "", "title": qsTrc("dorico", "Show Left Zone"), "icon": IconCode.SPLIT_VIEW_HORIZONTAL, "available": true },
                    { "id": "formatting", "title": qsTrc("dorico", "Graphic Editing"), "icon": IconCode.EDIT, "available": true },
                    { "id": "frames", "title": qsTrc("dorico", "Frames"), "icon": IconCode.FRAME_SQUARE, "available": true },
                    { "id": "staffSpacing", "title": qsTrc("dorico", "Staff Spacing"), "icon": IconCode.VERTICAL, "available": false },
                    { "id": "noteSpacing", "title": qsTrc("dorico", "Note Spacing"), "icon": IconCode.HORIZONTAL, "available": false },
                    { "id": "slices", "title": qsTrc("dorico", "Graphic Slices"), "icon": IconCode.CAMERA, "available": true }
                ]

                NavigationPanel {
                    id: toolsNav
                    name: "DoricoEngraveToolbox"
                    section: engraveToolbox.navigationSection
                    order: 1
                    direction: NavigationPanel.Vertical
                }

                DoricoTopBarModel {
                    id: toolboxDispatcher
                }

                Repeater {
                    model: toolColumn.tools

                    DoricoButton {
                        required property var modelData
                        required property int index

                        readonly property bool isActive: modelData.id !== "" && root.tool === modelData.id

                        icon: modelData.icon
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
                                root.tool = modelData.id
                            }
                        }
                    }
                }
            }
        }
    ]

    panels: [
        DockPanel {
            id: engraveLeftPanel

            objectName: "engraveLeftPanel"
            title: qsTrc("dorico", "Engrave")

            navigationSection: root.navigationPanelSec(engraveLeftPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: 200
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left
            dropDestinations: root.verticalPanelDropDestinations

            Item {
                id: leftContent

                NavigationPanel {
                    id: leftNav
                    name: "DoricoEngraveLeftZone"
                    section: engraveLeftPanel.navigationSection
                    order: engraveLeftPanel.contentNavigationPanelOrderStart
                    direction: NavigationPanel.Both
                    enabled: leftContent.enabled && leftContent.visible
                }

                // Only the brackets: kept here so it lives as long as the Formatting panel.
                DoricoPanelModel {
                    id: bracketing
                    panelId: "engraveBracketing"
                }

                StyledFlickable {
                    anchors.fill: parent
                    visible: root.tool === "formatting"
                    contentHeight: formatting.implicitHeight + 16

                    Column {
                        id: formatting

                        x: 8
                        y: 8
                        width: parent.width - 16
                        spacing: 12

                        ToolSection {
                            width: parent.width
                            title: qsTrc("dorico", "Moving Bars Between Systems")
                            navigationPanel: leftNav
                            navigationOrderStart: 0
                            codes: ["command://notation/move-measure-to-prev-system", "command://notation/move-measure-to-next-system"]
                            labels: [qsTrc("dorico", "Move to Previous System"), qsTrc("dorico", "Move to Next System")]
                        }

                        ToolSection {
                            width: parent.width
                            title: qsTrc("dorico", "Format Systems")
                            navigationPanel: leftNav
                            navigationOrderStart: 10
                            codes: ["command://notation/toggle-system-break", "command://notation/toggle-system-lock", "command://notation/make-into-system"]
                            labels: [qsTrc("dorico", "Insert System Break"), qsTrc("dorico", "Lock System"), qsTrc("dorico", "Make into System")]
                        }

                        // MuseScore has no music frames; its page break is the closest to a frame break.
                        ToolSection {
                            width: parent.width
                            title: qsTrc("dorico", "Format Music Frames")
                            navigationPanel: leftNav
                            navigationOrderStart: 20
                            codes: ["command://notation/toggle-page-break", "command://dorico/lock-frame", "command://dorico/make-into-frame"]
                            labels: [qsTrc("dorico", "Insert Frame Break"), qsTrc("dorico", "Lock Frame"), qsTrc("dorico", "Make into Frame")]
                        }

                        ToolSection {
                            width: parent.width
                            title: qsTrc("dorico", "Lock Layout")
                            navigationPanel: leftNav
                            navigationOrderStart: 30
                            codes: ["command://dorico/lock-layout", "command://notation/reset-to-default-layout"]
                            labels: [qsTrc("dorico", "Lock Layout"), qsTrc("dorico", "Reset Layout")]
                        }

                        ToolSection {
                            width: parent.width
                            title: qsTrc("dorico", "Coordination Lines")
                            navigationPanel: leftNav
                            navigationOrderStart: 40
                            codes: ["command://dorico/create-coordination-line"]
                            labels: [qsTrc("dorico", "Create Coordination Line")]
                        }

                        PaletteSections {
                            width: parent.width
                            panelModel: bracketing
                            navigationPanel: leftNav
                            navigationRowStart: 50
                        }

                        ToolSection {
                            width: parent.width
                            navigationPanel: leftNav
                            navigationOrderStart: 60
                            codes: ["command://dorico/change-barline-joins"]
                            labels: [qsTrc("dorico", "Change Barline Joins")]
                        }
                    }
                }

                Column {
                    x: 8
                    y: 8
                    width: parent.width - 16
                    spacing: 12
                    visible: root.tool === "frames"

                    // MuseScore's frames, under their MuseScore names: there are no music or graphics frames.
                    ToolSection {
                        width: parent.width
                        title: qsTrc("dorico", "Insert Frames")
                        navigationPanel: leftNav
                        navigationOrderStart: 100
                        codes: ["command://notation/insert-vbox", "command://notation/insert-textframe", "command://notation/insert-hbox"]
                        labels: [qsTrc("dorico", "Vertical Frame"), qsTrc("dorico", "Text Frame"), qsTrc("dorico", "Horizontal Frame")]
                        icons: [IconCode.VERTICAL_FRAME, IconCode.TEXT_FRAME, IconCode.HORIZONTAL_FRAME]
                    }

                    ToolSection {
                        width: parent.width
                        title: qsTrc("dorico", "Constraints")
                        navigationPanel: leftNav
                        navigationOrderStart: 110
                        codes: ["command://dorico/frame-constraints"]
                        labels: [qsTrc("dorico", "Lock to Page Margins")]
                    }
                }

                DoricoPlaceholder {
                    anchors.fill: parent
                    visible: root.tool === "slices"
                    text: qsTrc("dorico", "Graphic Slices are not available yet. Use File > Export (PNG or SVG) for images.")
                }
            }
        },

        DockPanel {
            id: pagesPanel

            objectName: "pagesPanel"
            title: qsTrc("dorico", "Pages")

            navigationSection: root.navigationPanelSec(pagesPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: 200
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Right
            visible: false
            dropDestinations: root.verticalPanelDropDestinations

            DoricoPlaceholder {
                text: qsTrc("dorico", "Page templates, flow headings and page overrides are not available yet.")
            }
        },

        LowerZonePanel {
            id: lowerZonePanel

            objectName: root.dockName("lowerZonePanel")

            tabs: ["properties", "mixer"]
            notationView: root.notationView

            navigationSection: root.navigationPanelSec(lowerZonePanel.location)

            dropDestinations: root.horizontalPanelDropDestinations
        }
    ]
}
