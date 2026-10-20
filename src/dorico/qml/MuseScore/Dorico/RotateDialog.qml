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

//! Write > Transform > Rotate Pitches / Rhythms / Pitches and Rhythms (spec 65). Answers "steps,fw" or "steps,bw".
StyledDialogView {
    id: root

    property string last: ""

    property int steps: 1
    property bool backwards: false

    function accept() {
        root.ret = { errcode: 0, value: Math.round(steps) + "," + (backwards ? "bw" : "fw") }
        root.hide()
    }

    contentWidth: content.implicitWidth
    contentHeight: content.implicitHeight
    margins: 16

    modal: true

    Component.onCompleted: {
        var f = last.split(",")
        if (f.length >= 2) {
            steps = parseInt(f[0])
            backwards = f[1] === "bw"
        }
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 12

        NavigationPanel {
            id: contentNavigationPanel
            name: "RotatePanel"
            enabled: content.enabled && content.visible
            section: root.navigationSection
            order: 1
            direction: NavigationPanel.Vertical
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: qsTrc("dorico", "Enter the number of steps by which to rotate.")
            horizontalAlignment: Text.AlignLeft
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            StyledTextLabel {
                text: qsTrc("dorico", "Number of steps:")
                horizontalAlignment: Text.AlignLeft
            }

            IncrementalPropertyControl {
                Layout.preferredWidth: 96
                navigation.panel: contentNavigationPanel
                navigation.row: 0
                currentValue: root.steps
                step: 1
                decimals: 0
                minValue: 1
                maxValue: 999
                onValueEdited: function(newValue) { root.steps = newValue }
            }

            StyledTextLabel {
                text: qsTrc("dorico", "Direction:")
                horizontalAlignment: Text.AlignLeft
            }

            FlatRadioButtonList {
                Layout.preferredWidth: 240

                navigationPanel: contentNavigationPanel
                navigationRowStart: 10
                accessibleName: qsTrc("dorico", "Direction")

                currentValue: root.backwards
                model: [
                    { text: qsTrc("dorico", "Forwards"), value: false },
                    { text: qsTrc("dorico", "Backwards"), value: true }
                ]

                onToggled: function(newValue) {
                    root.backwards = newValue
                }
            }
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
