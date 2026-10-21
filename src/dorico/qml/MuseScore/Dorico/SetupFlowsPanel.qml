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
