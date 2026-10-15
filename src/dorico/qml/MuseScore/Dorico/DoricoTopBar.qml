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

//! Dorico-style top bar (spec 50): Home/Publish, mode tabs, layout selector, View menu, Hide/Restore Zones, Mixer.
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

    implicitWidth: row.implicitWidth
    implicitHeight: 36
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

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTrc("dorico", "Home")
            navigation.panel: root.navigationPanel
            navigation.order: 0

            onClicked: topBar.openPage("musescore://home")
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTrc("dorico", "Publish")
            navigation.panel: root.navigationPanel
            navigation.order: 1

            onClicked: topBar.openPage("musescore://publish")
        }

        SeparatorLine {
            orientation: Qt.Vertical
        }

        ButtonGroup {
            id: modeGroup
        }

        Repeater {
            model: root.modes

            PageTabButton {
                id: tab

                required property int index
                required property string modelData

                ButtonGroup.group: modeGroup

                title: modelData
                spacing: 0
                leftPadding: 12
                rightPadding: 12

                checked: topBar.currentMode === index

                navigation.name: title
                navigation.panel: root.navigationPanel
                navigation.order: 2 + index

                onToggled: topBar.selectMode(index)
            }
        }

        SeparatorLine {
            orientation: Qt.Vertical
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

        FlatButton {
            id: viewButton

            anchors.verticalCenter: parent.verticalCenter

            text: qsTrc("dorico", "View")
            navigation.panel: root.navigationPanel
            navigation.order: 11

            onClicked: viewMenuLoader.toggleOpened(viewMenuModel.items)

            StyledMenuLoader {
                id: viewMenuLoader

                menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

                onHandleMenuItem: function(itemId) {
                    Qt.callLater(viewMenuModel.handleMenuItem, itemId)
                }
            }
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter

            icon: IconCode.EYE_OPEN
            toolTipTitle: qsTrc("dorico", "Hide/Restore Zones")
            navigation.panel: root.navigationPanel
            navigation.order: 12

            onClicked: topBar.dispatch("command://dorico/toggle-all-zones")
        }

        FlatButton {
            anchors.verticalCenter: parent.verticalCenter

            icon: IconCode.MIXER
            toolTipTitle: qsTrc("dorico", "Mixer")
            navigation.panel: root.navigationPanel
            navigation.order: 13

            onClicked: topBar.dispatch("command://app/dock/toggle-mixer")
        }
    }
}
