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
import MuseScore.NotationScene

//! Setup mode's Layouts panel (spec 55): one row per layout (full score, then the parts) and Dorico's action bar.
//! The list and card details come from DoricoTopBarModel (same order as the layout selector), which also deletes
//! layouts; create / rename go through MuseScore's own Parts-dialog model, PartListModel, whose row i is layout i + 1.
Item {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    property int selectedIndex: 0
    property int editingIndex: -1

    DoricoTopBarModel {
        id: layouts
        Component.onCompleted: layouts.load()

        // PartListModel builds its list once (it was written for a modal dialog), so it is rebuilt on each change.
        onLayoutsChanged: root.reloadParts()
    }

    property PartListModel parts: null

    // Right-click menu of a layout (spec 98)
    DoricoContextMenuModel {
        id: layoutMenu

        onSetupItemChosen: function(kind, itemId) {
            switch (itemId) {
            case "dorico-setup-rename": root.editingIndex = root.selectedIndex; break
            case "dorico-setup-delete": layouts.deleteLayout(root.selectedIndex); break
            case "dorico-setup-duplicate": root.parts.copyPart(root.selectedIndex - 1); break
            }
        }
    }

    Component {
        id: partsComponent
        PartListModel {}
    }

    function reloadParts() {
        if (root.parts) {
            root.parts.destroy()
        }
        root.parts = partsComponent.createObject(root)
        root.parts.load()
        root.selectedIndex = Math.min(root.selectedIndex, layouts.layoutTitles.length - 1)
    }

    Component.onCompleted: root.reloadParts()

    NavigationPanel {
        id: navPanel
        name: "DoricoLayoutsPanel"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    StyledListView {
        id: list

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: actionBar.top
        anchors.margins: 8

        spacing: 4
        model: layouts.layoutTitles

        // Dorico-style rows: score / part icon and name, selected = filled blue; details in the tooltip.
        delegate: Rectangle {
            id: card

            required property int index
            required property string modelData
            readonly property bool isSelected: root.selectedIndex === index
            readonly property color textColor: isSelected ? "#0C0C0C" : ui.theme.fontPrimaryColor

            width: ListView.view.width
            height: 26
            radius: 3
            color: isSelected ? "#45A6F5" : (ui.theme.isDark ? "#3D4A58" : "#DDE2E7")

            MouseArea {
                id: cardMouse

                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(mouse) {
                    root.selectedIndex = card.index
                    if (mouse.button === Qt.RightButton) {
                        // The full score is named after the project and cannot be deleted or duplicated
                        layoutMenu.popupSetup("layout", cardMouse, Qt.point(mouse.x, mouse.y),
                                              card.index === 0 ? ["dorico-setup-rename", "dorico-setup-delete", "dorico-setup-duplicate"] : [])
                    }
                }
                // Double-click shows the layout in the music area, like choosing it in the layout selector.
                onDoubleClicked: layouts.selectLayout(card.index)
                onContainsMouseChanged: {
                    const info = (card.index === 0 ? qsTrc("dorico", "Full score") : qsTrc("dorico", "Instrumental part"))
                               + ((layouts.layoutInfos[card.index] ?? "") !== "" ? " - " + layouts.layoutInfos[card.index] : "")
                    if (containsMouse) {
                        ui.tooltip.show(card, info)
                    } else {
                        ui.tooltip.hide(card)
                    }
                }
            }

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                DoricoButton {
                    anchors.verticalCenter: parent.verticalCenter
                    doricoIcon: card.index === 0 ? "svg:file-music" : "svg:file-user"
                    iconBoxSize: 14
                    transparent: true
                    iconColor: card.textColor
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.editingIndex !== card.index
                    text: card.index === 0 ? qsTrc("dorico", "Full score") : card.modelData
                    font: ui.theme.bodyFont
                    color: card.textColor
                }

                TextInputField {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.editingIndex === card.index
                    width: 180
                    height: 22
                    currentText: card.modelData
                    maximumLength: 40
                    onVisibleChanged: {
                        if (visible) {
                            forceActiveFocus()
                        }
                    }
                    onTextEditingFinished: function(newTextValue) {
                        // setPartTitle ignores empty and duplicate names.
                        root.parts.setPartTitle(card.index - 1, newTextValue)
                        root.editingIndex = -1
                    }
                }
            }
        }
    }

    Row {
        id: actionBar

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 8
        spacing: 4

        DoricoButton {
            doricoIcon: "svg:file-music"
            toolTipTitle: qsTrc("dorico", "Add Full Score Layout")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 1
        }

        DoricoButton {
            doricoIcon: "svg:file-user"
            toolTipTitle: qsTrc("dorico", "Add Instrumental Part Layout")
            transparent: true
            navigation.panel: navPanel
            navigation.order: 2
            onTriggered: root.parts.createNewPart()
        }

        DoricoButton {
            doricoIcon: "svg:file-pen"
            toolTipTitle: qsTrc("dorico", "Add Custom Score Layout")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 3
        }

        DoricoButton {
            doricoIcon: "svg:arrow-down-wide-narrow"
            toolTipTitle: qsTrc("dorico", "Sort Layouts")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 4
        }

        DoricoButton {
            doricoIcon: "svg:sliders-horizontal"
            toolTipTitle: qsTrc("dorico", "Layout Options")
            transparent: true
            navigation.panel: navPanel
            navigation.order: 5
            onTriggered: layouts.dispatch("command://notation/open-edit-style")
        }

        DoricoButton {
            doricoIcon: "svg:settings"
            toolTipTitle: qsTrc("dorico", "Layout Settings")
            toolTipDescription: available ? qsTrc("dorico", "Rename the selected part") : qsTrc("dorico", "The full score is named after the project")
            available: root.selectedIndex > 0
            transparent: true
            navigation.panel: navPanel
            navigation.order: 6
            onTriggered: root.editingIndex = root.selectedIndex
        }

        DoricoButton {
            doricoIcon: "svg:trash-2"
            toolTipTitle: qsTrc("dorico", "Delete Layout")
            toolTipDescription: available ? "" : qsTrc("dorico", "The full score cannot be deleted")
            available: root.selectedIndex > 0
            transparent: true
            navigation.panel: navPanel
            navigation.order: 7
            onTriggered: layouts.deleteLayout(root.selectedIndex)
        }
    }
}
