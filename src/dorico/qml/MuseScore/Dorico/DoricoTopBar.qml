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
import QtQuick.Controls
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents

//! Dorico-style top bar (spec 50): mode tabs, layout selector, View menu (eye), Hide/Restore Zones, Mixer.
//! Home and Publish are Window > Hub and File > Publish, as Dorico has no such buttons.
Item {
    id: root

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "DoricoTopBar"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("dorico", "Top bar")
    }

    readonly property var modes: [
        qsTrc("dorico", "Setup"),
        qsTrc("dorico", "Write"),
        qsTrc("dorico", "Engrave"),
        qsTrc("dorico", "Play"),
        qsTrc("dorico", "Print")
    ]
    readonly property int printMode: 4

    readonly property color stripColor: ui.theme.isDark ? ui.theme.strokeColor : "#DDE2E7"

    implicitWidth: row.implicitWidth
    implicitHeight: 37
    width: implicitWidth
    height: implicitHeight

    DoricoTopBarModel {
        id: topBar
    }

    DoricoViewMenuModel {
        id: viewMenuModel
    }

    Component.onCompleted: {
        topBar.load()
        viewMenuModel.load()
    }

    Row {
        id: row

        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        // Mode tabs: one rounded strip, the current mode filled (colours measured on Dorico 6)
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: modeRow.width + 4
            height: 33
            radius: 4
            color: root.stripColor

            Row {
                id: modeRow

                anchors.centerIn: parent

                Repeater {
                    model: root.modes

                    FlatButton {
                        id: tab

                        required property int index
                        required property string modelData

                        readonly property bool current: topBar.currentMode === index

                        height: 29
                        minWidth: 0
                        margins: 12
                        text: modelData
                        textFont: ui.theme.bodyBoldFont
                        transparent: false
                        accentButton: current

                        navigation.name: modelData
                        navigation.panel: root.navigationPanel
                        navigation.order: 2 + index

                        onClicked: topBar.selectMode(index)
                    }
                }
            }
        }

        StyledDropdown {
            width: 160
            anchors.verticalCenter: parent.verticalCenter

            enabled: topBar.currentMode !== root.printMode
            model: topBar.layoutTitles
            currentIndex: topBar.currentLayoutIndex

            navigation.panel: root.navigationPanel
            navigation.order: 10

            onActivated: function(index, value) {
                topBar.selectLayout(index)
            }
        }

        // View options (eye) and Hide/Restore Zones (window), in one strip like Dorico's
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: viewRow.width + 4
            height: 33
            radius: 4
            color: root.stripColor

            Row {
                id: viewRow

                anchors.centerIn: parent

                DoricoButton {
                    id: viewButton

                    doricoIcon: "svg:eye"
                    iconBoxSize: 18
                    transparent: true
                    toolTipTitle: qsTrc("dorico", "View")
                    navigation.panel: root.navigationPanel
                    navigation.order: 11

                    onTriggered: viewMenuLoader.toggleOpened(viewMenuModel.items)

                    StyledMenuLoader {
                        id: viewMenuLoader

                        menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

                        onHandleMenuItem: function(itemId) {
                            Qt.callLater(viewMenuModel.handleMenuItem, itemId)
                        }
                    }
                }

                DoricoButton {
                    doricoIcon: "svg:panel-top"
                    iconBoxSize: 18
                    transparent: true
                    toolTipTitle: qsTrc("dorico", "Hide/Restore Zones")
                    navigation.panel: root.navigationPanel
                    navigation.order: 12

                    onTriggered: topBar.dispatch("command://dorico/toggle-all-zones")
                }
            }
        }
    }
}
