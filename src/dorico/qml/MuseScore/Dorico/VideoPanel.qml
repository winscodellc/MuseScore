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

//! Write mode's Video panel (spec 91): Properties, and the markers table (Timecode / Text / Imp.) with Add, Delete
//! and Find Tempo. Double-click a timecode or text to edit it; important markers are bold.
Item {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    property int selectedId: -1
    property int editingId: -1
    property string editingColumn: ""   // "timecode" or "text"
    property string error: ""

    readonly property int timecodeWidth: 92
    readonly property int impWidth: 34

    VideoModel {
        id: video
        Component.onCompleted: video.load()
    }

    NavigationPanel {
        id: navPanel
        name: "DoricoVideoPanel"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    Column {
        id: videoSection

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        spacing: 6

        StyledTextLabel {
            text: qsTrc("dorico", "Video")
            font: ui.theme.bodyBoldFont
        }

        FlatButton {
            text: qsTrc("dorico", "Properties...")
            navigation.panel: navPanel
            navigation.order: 1
            onClicked: video.openProperties()
        }

        StyledTextLabel {
            topPadding: 8
            text: qsTrc("dorico", "Markers")
            font: ui.theme.bodyBoldFont
        }

        // Column headers
        Row {
            width: parent.width
            StyledTextLabel { width: root.timecodeWidth; text: qsTrc("dorico", "Timecode"); horizontalAlignment: Text.AlignLeft }
            StyledTextLabel { width: parent.width - root.timecodeWidth - root.impWidth; text: qsTrc("dorico", "Text"); horizontalAlignment: Text.AlignLeft }
            StyledTextLabel { width: root.impWidth; text: qsTrc("dorico", "Imp."); horizontalAlignment: Text.AlignHCenter }
        }
    }

    StyledListView {
        id: list

        anchors.top: videoSection.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: errorLabel.top
        anchors.margins: 8
        anchors.topMargin: 2

        spacing: 1
        model: video.markers

        delegate: Rectangle {
            id: row

            required property int index
            required property var modelData
            readonly property bool isSelected: root.selectedId === modelData.id
            readonly property color textColor: isSelected ? "#0C0C0C" : ui.theme.fontPrimaryColor

            width: ListView.view.width
            height: 24
            color: isSelected ? "#40ACFF" : (index % 2 ? "transparent" : (ui.theme.isDark ? "#2B333C" : "#E8ECF0"))

            function finishEdit(column, value) {
                if (root.editingId !== row.modelData.id || root.editingColumn !== column) {
                    return
                }
                root.editingId = -1
                if (column === "timecode") {
                    root.error = video.setMarkerTimecode(row.modelData.id, value)
                } else {
                    video.setMarkerText(row.modelData.id, value)
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.selectedId = row.modelData.id
                onDoubleClicked: function(mouse) {
                    root.selectedId = row.modelData.id
                    root.editingColumn = mouse.x < root.timecodeWidth ? "timecode" : "text"
                    root.editingId = row.modelData.id
                }
            }

            Text {
                x: 4
                width: root.timecodeWidth - 4
                anchors.verticalCenter: parent.verticalCenter
                visible: !(root.editingId === row.modelData.id && root.editingColumn === "timecode")
                text: row.modelData.timecode
                font.family: ui.theme.bodyFont.family
                font.pixelSize: ui.theme.bodyFont.pixelSize
                font.bold: row.modelData.important
                color: row.textColor
            }

            Text {
                x: root.timecodeWidth
                width: row.width - root.timecodeWidth - root.impWidth
                anchors.verticalCenter: parent.verticalCenter
                visible: !(root.editingId === row.modelData.id && root.editingColumn === "text")
                text: row.modelData.text
                elide: Text.ElideRight
                font.family: ui.theme.bodyFont.family
                font.pixelSize: ui.theme.bodyFont.pixelSize
                font.bold: row.modelData.important
                color: row.textColor
            }

            // Timecode: Dorico's timecode spin box
            DoricoTimecodeField {
                width: root.timecodeWidth
                height: 22
                anchors.verticalCenter: parent.verticalCenter
                visible: root.editingId === row.modelData.id && root.editingColumn === "timecode"
                timecode: row.modelData.timecode
                dropFrame: video.dropFrame
                framesPerSecond: video.framesPerSecond
                onVisibleChanged: {
                    if (visible) {
                        forceInputFocus()
                    }
                }
                onEditingFinished: function(tc) { row.finishEdit("timecode", tc) }
                onEscaped: root.editingId = -1
            }

            TextInputField {
                x: root.timecodeWidth
                width: row.width - root.timecodeWidth - root.impWidth
                height: 22
                anchors.verticalCenter: parent.verticalCenter
                visible: root.editingId === row.modelData.id && root.editingColumn === "text"
                currentText: row.modelData.text
                onVisibleChanged: {
                    if (visible) {
                        forceActiveFocus()
                    }
                }
                onTextEditingFinished: function(newTextValue) { row.finishEdit("text", newTextValue) }
                onEscaped: root.editingId = -1
            }

            CheckBox {
                x: row.width - root.impWidth + (root.impWidth - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                checked: row.modelData.important
                onClicked: video.setMarkerImportant(row.modelData.id, !row.modelData.important)
            }
        }
    }

    StyledTextLabel {
        id: errorLabel

        anchors.bottom: actionBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: root.error !== "" ? 8 : 0
        height: root.error !== "" ? implicitHeight : 0
        visible: root.error !== ""
        text: root.error
        color: "#E05050"
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
    }

    Row {
        id: actionBar

        anchors.bottom: findTempo.top
        anchors.left: parent.left
        anchors.margins: 8
        spacing: 4

        DoricoButton {
            doricoIcon: "svg:plus"
            toolTipTitle: qsTrc("dorico", "Add Marker")
            available: video.hasProject
            transparent: true
            navigation.panel: navPanel
            navigation.order: 2
            onTriggered: { root.error = ""; video.openAddMarker() }
        }

        DoricoButton {
            doricoIcon: "svg:trash-2"
            toolTipTitle: qsTrc("dorico", "Delete Marker")
            available: root.selectedId >= 0
            transparent: true
            navigation.panel: navPanel
            navigation.order: 3
            onTriggered: { video.deleteMarker(root.selectedId); root.selectedId = -1 }
        }
    }

    // Enabled once a marker is important (Find Tempo itself is spec 91 phase D)
    FlatButton {
        id: findTempo

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        text: qsTrc("dorico", "Find Tempo...")
        enabled: false
        navigation.panel: navPanel
        navigation.order: 4
    }
}
