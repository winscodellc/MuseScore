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

import Muse.Ui
import Muse.UiComponents

import MuseScore.InstrumentsScene

//! Setup mode's Players panel (spec 55), Dorico style: one row per instrument (and per system-markings layer and
//! shared part) over MuseScore's own instrument model, LayoutPanelTreeModel, so adding, removing, reordering, hiding
//! and instrument settings are MuseScore's. Click selects (Shift-click: a range), drag reorders, right-click opens
//! Show/Hide, Settings, Enabled (shared parts) and Remove; Delete removes. Staff rows are not shown.
FocusScope {
    id: root

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    readonly property alias treeModel: tree

    // The tree item whose settings popup is open (the popups call back into it)
    property var popupItem: null

    LayoutPanelTreeModel {
        id: tree
    }

    onVisibleChanged: tree.setLayoutPanelVisible(root.visible)
    Component.onCompleted: tree.setLayoutPanelVisible(root.visible)

    Keys.onShortcutOverride: function(event) {
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
            event.accepted = true
            tree.removeSelectedRows()
        }
    }

    NavigationPanel {
        id: navPanel
        name: "DoricoPlayers"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    MouseArea {
        anchors.fill: parent
        onClicked: tree.clearSelection()
    }

    StyledTextLabel {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: tree.isEmpty
        text: qsTrc("dorico", "No players yet. Click Add Single Player.")
        wrapMode: Text.WordWrap
        opacity: 0.7
    }

    function typeIcon(type) {
        switch (type) {
        case LayoutPanelItemType.SHARED_PART: return "svg:users"
        case LayoutPanelItemType.SYSTEM_OBJECTS_LAYER: return "svg:minus"
        default: return "svg:user"
        }
    }

    function menuItems(item) {
        const items = []
        if (item.type !== LayoutPanelItemType.SYSTEM_OBJECTS_LAYER) {
            items.push({ "id": "visibility", "title": item.isVisible ? qsTrc("dorico", "Hide") : qsTrc("dorico", "Show") })
        }
        if (item.type === LayoutPanelItemType.SHARED_PART) {
            items.push({ "id": "enabled", "title": qsTrc("dorico", "Enabled"), "checkable": true, "checked": item.isEnabled,
                         "enabled": tree.isStaveSharingEnabled })
        }
        if (item.settingsAvailable) {
            items.push({ "id": "settings", "title": qsTrc("dorico", "Settings…"), "enabled": item.settingsEnabled })
        }
        if (item.isRemovable) {
            items.push({ "id": "", "title": "" }) // separator
            items.push({ "id": "remove", "title": qsTrc("dorico", "Remove") })
        }
        return items
    }

    function openSettings(item, anchor) {
        let comp = null
        const params = {}
        if (item.type === LayoutPanelItemType.PART) {
            comp = instrumentSettingsComp
            params["partId"] = item.id
            params["instrumentId"] = (item as PartTreeItem).instrumentId()
        } else if (item.type === LayoutPanelItemType.SYSTEM_OBJECTS_LAYER) {
            comp = systemObjectsLayerSettingsComp
            params["staffId"] = (item as SystemObjectsLayerTreeItem).staffId()
        }
        if (!comp) {
            return
        }
        root.popupItem = item
        popupLoader.openPopup(comp, anchor, params)
    }

    // Same pattern as MuseScore's LayoutPanelItemDelegate
    Loader {
        id: popupLoader

        readonly property StyledPopupView openedPopup: popupLoader.item as StyledPopupView

        function openPopup(comp, anchor, params) {
            popupLoader.sourceComponent = comp
            if (!openedPopup) {
                return
            }
            openedPopup.parent = anchor
            openedPopup.load(params)
            openedPopup.closed.connect(function() {
                popupLoader.sourceComponent = null
            })
            openedPopup.open()
        }
    }

    Component {
        id: instrumentSettingsComp

        InstrumentSettingsPopup {
            // Run the operation after the popup has closed itself: it is deleted on close, so doing it inside
            // the signal handler would crash (see LayoutPanelItemDelegate)
            onReplaceInstrumentRequested: Qt.callLater((root.popupItem as PartTreeItem).replaceInstrument)
            onResetAllFormattingRequested: Qt.callLater((root.popupItem as PartTreeItem).resetAllFormatting)
        }
    }

    Component {
        id: systemObjectsLayerSettingsComp

        SystemObjectsLayerSettingsPopup {}
    }

    ListView {
        id: list

        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: tree

        delegate: DropArea {
            id: cell

            required property var item
            required property int index

            width: ListView.view.width
            height: 26
            keys: ["doricoPlayer"]

            // Move the dragged row here. Also on every pointer move, not only on enter: after a move the rows shift
            // under a still pointer, so a fast drag would otherwise stop one row short.
            function moveDraggedHere(drag) {
                const source = drag.source
                if (!source || source.rowIndex === cell.index || !cell.item || !cell.item.canAcceptDrop(source.treeItem)) {
                    return
                }
                const from = tree.index(source.rowIndex, 0)
                const to = tree.index(cell.index, 0)
                Qt.callLater(tree.moveRows, from.parent, from.row, 1, to.parent, to.row)
            }

            onEntered: function(drag) { cell.moveDraggedHere(drag) }
            onPositionChanged: function(drag) { cell.moveDraggedHere(drag) }

            Rectangle {
                id: content

                readonly property int rowIndex: cell.index
                readonly property var treeItem: cell.item
                readonly property bool selected: Boolean(cell.item) && cell.item.isSelected
                readonly property bool dimmed: Boolean(cell.item)
                                               && (!cell.item.isVisible
                                                   || (cell.item.type === LayoutPanelItemType.SHARED_PART && !cell.item.isEnabled))
                readonly property color textColor: selected ? "#0C0C0C" : ui.theme.fontPrimaryColor

                width: cell.width
                height: cell.height
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 3
                color: selected ? "#45A6F5" : (ui.theme.isDark ? "#3D4A58" : "#DDE2E7")
                opacity: dimmed ? 0.55 : 1.0

                Drag.keys: ["doricoPlayer"]
                Drag.active: rowMouse.drag.active
                Drag.source: content
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                // While dragged, the row floats above the list
                states: State {
                    when: rowMouse.drag.active

                    ParentChange { target: content; parent: list }
                    AnchorChanges {
                        target: content
                        anchors.verticalCenter: undefined
                        anchors.horizontalCenter: undefined
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
                        doricoIcon: cell.item ? root.typeIcon(cell.item.type) : "svg:user"
                        iconBoxSize: 14
                        transparent: true
                        iconColor: content.textColor
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: cell.item ? cell.item.title : ""
                        font: ui.theme.bodyFont
                        color: content.textColor
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: rowMouse

                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    drag.target: content
                    drag.axis: Drag.YAxis
                    enabled: Boolean(cell.item) && cell.item.isSelectable

                    onClicked: function(mouse) {
                        root.forceActiveFocus()
                        if (mouse.button === Qt.RightButton) {
                            if (!cell.item.isSelected) {
                                tree.selectRow(tree.index(cell.index, 0))
                            }
                            menuLoader.toggleOpened(root.menuItems(cell.item))
                        } else {
                            tree.selectRow(tree.index(cell.index, 0))
                        }
                    }

                    drag.onActiveChanged: {
                        if (drag.active) {
                            tree.startActiveDrag()
                        } else {
                            content.Drag.drop()
                            tree.endActiveDrag()
                        }
                    }
                }

                StyledMenuLoader {
                    id: menuLoader

                    onHandleMenuItem: function(itemId) {
                        switch (itemId) {
                        case "visibility":
                            tree.changeVisibilityOfSelectedRows(!cell.item.isVisible)
                            break
                        case "enabled":
                            tree.changeEnabledOfSelectedRows(!cell.item.isEnabled)
                            break
                        case "settings":
                            root.openSettings(cell.item, content)
                            break
                        case "remove":
                            tree.removeSelectedRows()
                            break
                        }
                    }
                }

                NavigationControl {
                    name: cell.item ? cell.item.title : ""
                    panel: navPanel
                    row: cell.index
                    accessible.role: MUAccessible.ListItem
                    accessible.name: cell.item ? cell.item.title : ""
                    onTriggered: tree.selectRow(tree.index(cell.index, 0))
                }
            }
        }
    }
}
