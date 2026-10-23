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

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

//! Add Marker (Shift-Alt-M, the Video panel's + and the Markers track, spec 91): text and timecode, which defaults to
//! the playhead. A timecode outside the flow is clamped into it, as in Dorico.
StyledDialogView {
    id: root

    title: qsTrc("dorico", "Add Marker")

    contentWidth: 360
    contentHeight: content.implicitHeight
    margins: 16

    modal: true

    property string markerText: "Marker"
    property string timecode: ""
    property string error: ""

    VideoModel {
        id: video
    }

    function accept() {
        root.error = video.addMarker(root.markerText, root.timecode)
        if (root.error === "") {
            root.ret = { errcode: 0, value: "" }
            root.hide()
        }
    }

    Component.onCompleted: {
        video.load()
        root.timecode = video.playheadTimecode
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 12

        NavigationPanel {
            id: contentNavigationPanel
            name: "AddMarkerPanel"
            enabled: content.enabled && content.visible
            section: root.navigationSection
            order: 1
            direction: NavigationPanel.Vertical
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            StyledTextLabel {
                text: qsTrc("dorico", "Text:")
                horizontalAlignment: Text.AlignLeft
            }

            TextInputField {
                id: textField
                Layout.fillWidth: true
                navigation.panel: contentNavigationPanel
                navigation.row: 0
                currentText: root.markerText
                Component.onCompleted: { forceActiveFocus(); selectAll() }
                onTextChanged: function(newTextValue) { root.markerText = newTextValue }
                onAccepted: root.accept()
            }

            StyledTextLabel {
                text: qsTrc("dorico", "Timecode:")
                horizontalAlignment: Text.AlignLeft
            }

            DoricoTimecodeField {
                timecode: root.timecode
                dropFrame: video.dropFrame
                framesPerSecond: video.framesPerSecond
                onEdited: function(tc) { root.timecode = tc; root.error = "" }
                onAccepted: root.accept()
            }

            StyledTextLabel {
                text: qsTrc("dorico", "Valid range:")
                horizontalAlignment: Text.AlignLeft
            }

            StyledTextLabel {
                text: video.rangeStart + " - " + video.rangeEnd
                horizontalAlignment: Text.AlignLeft
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            visible: root.error !== ""
            text: root.error
            color: "#E05050"
            horizontalAlignment: Text.AlignLeft
        }

        ButtonBox {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight | Qt.AlignBottom

            buttons: [ ButtonBoxModel.Cancel ]

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 2

            FlatButton {
                text: qsTrc("global", "OK")
                buttonRole: ButtonBoxModel.AcceptRole
                buttonId: ButtonBoxModel.Ok
                accentButton: true

                onClicked: root.accept()
            }

            onStandardButtonClicked: function(buttonId) {
                if (buttonId === ButtonBoxModel.Cancel) {
                    root.reject()
                }
            }
        }
    }
}
