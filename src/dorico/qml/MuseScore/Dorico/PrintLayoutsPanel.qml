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

//! Print mode's Layouts panel (left zone, spec 58): which layouts to print, and the System Dialogs footer.
Item {
    id: root

    property PrintModel printModel: null
    property DoricoTopBarModel layouts: null
    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1
    NavigationPanel {
        id: layoutsNav
        name: "DoricoPrintLayouts"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    ZoneHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        title: qsTrc("dorico", "Layouts")
    }

    StyledListView {
        id: layoutList

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: systemDialogs.top
        anchors.bottomMargin: 8

        spacing: 4
        model: root.printModel

        // Dorico-style layout rows: icon and name; the layouts to print are filled blue. Click toggles.
        delegate: Rectangle {
            id: card

            required property int index
            required property string title
            required property string info
            required property bool checked

            width: ListView.view.width - 16
            x: 8
            height: 26
            radius: 3
            color: card.checked ? "#45A6F5" : (ui.theme.isDark ? "#3D4A58" : "#DDE2E7")

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                DoricoButton {
                    anchors.verticalCenter: parent.verticalCenter
                    doricoIcon: card.index === 0 ? "svg:file-music" : "svg:file-user"
                    iconBoxSize: 14
                    transparent: true
                    iconColor: card.checked ? "#0C0C0C" : ui.theme.fontPrimaryColor
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: card.title
                    font: ui.theme.bodyFont
                    color: card.checked ? "#0C0C0C" : ui.theme.fontPrimaryColor
                }
            }

            MouseArea {
                id: cardMouse

                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.printModel.setChecked(card.index, !card.checked)
                onContainsMouseChanged: {
                    if (containsMouse) {
                        ui.tooltip.show(card, card.info)
                    } else {
                        ui.tooltip.hide(card)
                    }
                }
            }

            NavigationControl {
                name: card.title
                panel: layoutsNav
                order: card.index
                accessible.role: MUAccessible.CheckBox
                accessible.name: card.title
                accessible.checked: card.checked
                onTriggered: root.printModel.setChecked(card.index, !card.checked)
            }
        }
    }

    // Dorico's "System Dialogs" footer: the page settings dialog and the system print dialog.
    Column {
        id: systemDialogs

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        spacing: 6

        StyledTextLabel { text: qsTrc("dorico", "System Dialogs:") }

        Row {
            spacing: 8

            FlatButton {
                text: qsTrc("dorico", "Page Setup...")
                navigation.panel: layoutsNav
                navigation.order: 1000
                onClicked: root.layouts.dispatch("command://notation/open-page-settings")
            }

            FlatButton {
                text: qsTrc("dorico", "Print...")
                enabled: root.printModel.hasChecked
                navigation.panel: layoutsNav
                navigation.order: 1001
                onClicked: root.printModel.print()
            }
        }
    }
}
