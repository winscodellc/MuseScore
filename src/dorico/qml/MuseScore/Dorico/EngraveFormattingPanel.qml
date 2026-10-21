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

//! Engrave mode's left zone (spec 56): the Formatting, Frames or Graphic Slices panel, chosen by the Engrave toolbox.
Item {
    id: leftContent

    // "formatting" (Graphic Editing), "frames" or "slices"
    property string tool: "formatting"
    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    NavigationPanel {
        id: leftNav
        name: "DoricoEngraveLeftZone"
        section: leftContent.navigationSection
        order: leftContent.navigationOrderStart
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
        visible: leftContent.tool === "formatting"
        contentHeight: formatting.implicitHeight + 8

        // Dorico's Formatting panel: collapsible sections of labelled buttons (names as in Dorico 6)
        Column {
            id: formatting

            width: parent.width
            spacing: 2

            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Move Bars Between Systems")

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 0
                    codes: ["command://notation/move-measure-to-prev-system", "command://notation/move-measure-to-next-system"]
                    labels: [qsTrc("dorico", "Move to Previous System"), qsTrc("dorico", "Move to Next System")]
                    icons: ["svg:corner-left-up", "svg:corner-right-down"]
                }
            }

            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Format Systems")

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 10
                    codes: ["command://notation/toggle-system-break", "command://notation/toggle-system-lock", "command://notation/make-into-system"]
                    labels: [qsTrc("dorico", "Create System Break"), qsTrc("dorico", "Lock System"), qsTrc("dorico", "Make Into System")]
                    icons: ["svg:wrap-text", "svg:lock", "svg:fold-horizontal"]
                }
            }

            // MuseScore has no music frames; its page break is the closest to a frame break.
            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Format Music Frames")

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 20
                    codes: ["command://notation/toggle-page-break", "command://dorico/lock-frame", "command://dorico/make-into-frame"]
                    labels: [qsTrc("dorico", "Create Frame Break"), qsTrc("dorico", "Lock Frame"), qsTrc("dorico", "Make Into Frame")]
                    icons: ["svg:file-down", "svg:lock", "svg:fold-vertical"]
                }
            }

            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Lock Layout")

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 30
                    codes: ["command://dorico/lock-layout", "command://notation/reset-to-default-layout"]
                    labels: [qsTrc("dorico", "Lock Layout"), qsTrc("dorico", "Reset Layout")]
                    icons: ["svg:lock", "svg:rotate-ccw"]
                }
            }

            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Coordination Lines")

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 40
                    codes: ["command://dorico/create-coordination-line"]
                    labels: [qsTrc("dorico", "Create Coordination Line")]
                    icons: ["svg:align-center-vertical"]
                }
            }

            InspectorSection {
                width: formatting.width
                title: qsTrc("dorico", "Bracketing")

                PaletteSections {
                    width: parent.width
                    panelModel: bracketing
                    showTitles: false
                    navigationPanel: leftNav
                    navigationRowStart: 50
                }

                ToolSection {
                    width: parent.width
                    labelled: true
                    navigationPanel: leftNav
                    navigationOrderStart: 60
                    codes: ["command://dorico/change-barline-joins"]
                    labels: [qsTrc("dorico", "Change Barline Joins")]
                    icons: ["svg:link-2"]
                }
            }
        }
    }

    Column {
        id: framesColumn

        width: parent.width
        spacing: 2
        visible: leftContent.tool === "frames"

        // MuseScore's frames, under their MuseScore names: there are no music or graphics frames.
        InspectorSection {
            width: framesColumn.width
            title: qsTrc("dorico", "Insert Frames")

            ToolSection {
                width: parent.width
                labelled: true
                navigationPanel: leftNav
                navigationOrderStart: 100
                codes: ["command://notation/insert-vbox", "command://notation/insert-textframe", "command://notation/insert-hbox"]
                labels: [qsTrc("dorico", "Vertical Frame"), qsTrc("dorico", "Text Frame"), qsTrc("dorico", "Horizontal Frame")]
                icons: [IconCode.VERTICAL_FRAME, IconCode.TEXT_FRAME, IconCode.HORIZONTAL_FRAME]
            }
        }

        InspectorSection {
            width: framesColumn.width
            title: qsTrc("dorico", "Constraints")

            ToolSection {
                width: parent.width
                labelled: true
                navigationPanel: leftNav
                navigationOrderStart: 110
                codes: ["command://dorico/frame-constraints"]
                labels: [qsTrc("dorico", "Lock to Page Margins")]
                icons: ["svg:scan"]
            }
        }
    }

    DoricoPlaceholder {
        anchors.fill: parent
        visible: leftContent.tool === "slices"
        text: qsTrc("dorico", "Graphic Slices are not available yet. Use File > Export (PNG or SVG) for images.")
    }
}
