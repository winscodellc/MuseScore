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
import Muse.GraphicalEffects

//! Setup mode's Flows panel (spec 55), shown in the lower zone.
ColumnLayout {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    spacing: 0

    DoricoTopBarModel {
        id: setupModel
        Component.onCompleted: setupModel.load()
    }

    ZoneHeader {
        Layout.fillWidth: true
        title: qsTrc("dorico", "Flows")
    }

    // A MuseScore project is one score = one flow: one card filling the panel, as Dorico shows a single flow
    // (colour measured on Dorico 6), with the flow actions in a column on the right.
    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 6

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6

            radius: 3
            color: "#30678B"

            // Dorico's flow menu (spec 98): MuseScore has one flow, so only its two settings rows work
            DoricoContextMenuModel {
                id: flowMenu
            }

            MouseArea {
                id: flowMouse

                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: function(mouse) { flowMenu.popupSetup("flow", flowMouse, Qt.point(mouse.x, mouse.y)) }
            }

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

            // The flow's start timecode (spec 91)
            VideoModel {
                id: flowVideo
                Component.onCompleted: flowVideo.load()
            }

            // The reel while a video is attached, a warning when its file is missing (phase B), then the timecode
            Row {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 8
                spacing: 6

                Item {
                    width: 18
                    height: 18
                    visible: flowVideo.hasVideo

                    Image {
                        id: flowVideoIcon
                        anchors.fill: parent
                        visible: false
                        source: Qt.resolvedUrl(flowVideo.videoMissing ? "icons/triangle-alert.svg" : "icons/film.svg")
                        sourceSize: Qt.size(18, 18)
                    }

                    EffectColorOverlay {
                        anchors.fill: flowVideoIcon
                        source: flowVideoIcon
                        color: flowVideo.videoMissing ? "#FFC83D" : "#FFFFFF"
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    // Dorico leaves it out while it is 00:00:00:00, unless a video is attached
                    visible: flowVideo.hasVideo || !/^00:00:00[:;]00$/.test(flowVideo.flowStartTimecode)
                    text: flowVideo.flowStartTimecode
                    font: ui.theme.bodyFont
                    color: "#FFFFFF"
                }
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
                section: root.navigationSection
                order: root.navigationOrderStart
                direction: NavigationPanel.Horizontal
                enabled: root.enabled && root.visible
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
