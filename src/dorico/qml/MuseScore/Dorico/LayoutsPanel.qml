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

//! Setup mode's Layouts panel (spec 55): one card per layout (full score, then the parts) and Dorico's action bar.
//! The list and card details come from DoricoTopBarModel (same order as the layout selector); create / rename /
//! delete go through MuseScore's own Parts-dialog model, PartListModel, whose row i is layout i + 1.
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

        spacing: 6
        model: layouts.layoutTitles

        delegate: Rectangle {
            id: card

            required property int index
            required property string modelData

            readonly property bool isSelected: root.selectedIndex === index

            width: ListView.view.width
            height: cardColumn.implicitHeight + 16
            radius: 4
            color: isSelected ? ui.theme.accentColor : ui.theme.backgroundSecondaryColor
            opacity: isSelected ? 1.0 : 0.9
            border.width: 1
            border.color: ui.theme.strokeColor

            MouseArea {
                anchors.fill: parent
                onClicked: root.selectedIndex = card.index
                // Double-click shows the layout in the music area, like choosing it in the layout selector.
                onDoubleClicked: layouts.selectLayout(card.index)
            }

            Column {
                id: cardColumn

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                spacing: 2

                StyledTextLabel {
                    text: card.index === 0 ? qsTrc("dorico", "Full score") : qsTrc("dorico", "Instrumental part")
                    opacity: 0.7
                }

                StyledTextLabel {
                    visible: root.editingIndex !== card.index
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    text: card.index === 0 ? layouts.projectTitle : card.modelData
                    font: ui.theme.bodyBoldFont
                }

                TextInputField {
                    visible: root.editingIndex === card.index
                    width: parent.width
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

                StyledTextLabel {
                    visible: text !== ""
                    text: layouts.layoutInfos[card.index] ?? ""
                    opacity: 0.7
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
            icon: IconCode.SCORE
            toolTipTitle: qsTrc("dorico", "Add Full Score Layout")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 1
        }

        DoricoButton {
            icon: IconCode.PLUS
            toolTipTitle: qsTrc("dorico", "Add Instrumental Part Layout")
            transparent: true
            navigation.panel: navPanel
            navigation.order: 2
            onTriggered: root.parts.createNewPart()
        }

        DoricoButton {
            icon: IconCode.NOTE_PLUS
            toolTipTitle: qsTrc("dorico", "Add Custom Score Layout")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 3
        }

        DoricoButton {
            icon: IconCode.LIST
            toolTipTitle: qsTrc("dorico", "Sort Layouts")
            available: false
            transparent: true
            navigation.panel: navPanel
            navigation.order: 4
        }

        DoricoButton {
            icon: IconCode.CONFIGURE
            toolTipTitle: qsTrc("dorico", "Layout Options")
            transparent: true
            navigation.panel: navPanel
            navigation.order: 5
            onTriggered: layouts.dispatch("command://notation/open-edit-style")
        }

        DoricoButton {
            icon: IconCode.EDIT
            toolTipTitle: qsTrc("dorico", "Layout Settings")
            toolTipDescription: available ? qsTrc("dorico", "Rename the selected part") : qsTrc("dorico", "The full score is named after the project")
            available: root.selectedIndex > 0
            transparent: true
            navigation.panel: navPanel
            navigation.order: 6
            onTriggered: root.editingIndex = root.selectedIndex
        }

        DoricoButton {
            icon: IconCode.DELETE_TANK
            toolTipTitle: qsTrc("dorico", "Delete Layout")
            toolTipDescription: available ? "" : qsTrc("dorico", "The full score cannot be deleted")
            available: root.selectedIndex > 0
            transparent: true
            navigation.panel: navPanel
            navigation.order: 7
            onTriggered: root.parts.removePart(root.selectedIndex - 1)
        }
    }
}
