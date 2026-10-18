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
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents

//! Write mode's Notes toolbox (spec 52): the vertical icon strip at the far left, 15 buttons in the manual's order.
//! Highlight and enabled state follow the commands (DoricoToolsModel). Buttons with a `menu` open it on
//! click-and-hold, like Dorico's; a plain click runs the button's own command.
Column {
    id: root

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "DoricoNotesToolbox"
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("dorico", "Notes toolbox")
    }

    // ponytail: command ids for features no spec has built yet (select tool, force duration, insert
    // scopes, slash voices) are unregistered on purpose: DoricoToolsModel turns them into unavailable buttons.
    readonly property var buttons: [
        { "code": "command://dorico/toggle-zone-left", "title": qsTrc("dorico", "Show Left Zone"), "icon": "svg:panel-left", "groupEnd": true },
        { "code": "command://dorico/select-tool", "title": qsTrc("dorico", "Select"), "icon": "svg:mouse-pointer-2" },
        { "code": "command://dorico/return", "title": qsTrc("dorico", "Start Note Input"), "icon": "svg:pencil" },
        { "code": "command://notation/toggle-note-input-by-duration", "title": qsTrc("dorico", "Pitch Before Duration"), "icon": "\uE0A4\uEB62\uECA5" },
        { "code": "command://dorico/toggle-chord-mode", "title": qsTrc("dorico", "Chords"), "icon": "svg:chord" },
        { "code": "command://dorico/toggle-insert-mode", "title": qsTrc("dorico", "Insert"), "icon": "svg:text-cursor-input",
            "groupEnd": true, "menu": [
                { "code": "command://dorico/toggle-insert-mode", "title": qsTrc("dorico", "Voice") },
                { "code": "command://dorico/insert-scope-player", "title": qsTrc("dorico", "Player") },
                { "code": "command://dorico/insert-scope-global", "title": qsTrc("dorico", "Global") },
                { "code": "command://dorico/insert-scope-global-adjustment", "title": qsTrc("dorico", "Global Adjustment") }
            ] },
        // Spec 11 maps Lock Duration (L) to MuseScore's re-pitch input method.
        { "code": "command://notation/toggle-note-input-repitch", "title": qsTrc("dorico", "Lock Duration"), "icon": "svg:lock" },
        { "code": "command://dorico/force-duration", "title": qsTrc("dorico", "Force Duration"), "icon": "svg:pin", "groupEnd": true },
        { "code": "command://dorico/next-voice", "title": qsTrc("dorico", "Create Voice"), "icon": "svg:voice",
            "menu": [
                { "code": "command://dorico/new-voice", "title": qsTrc("dorico", "Create Voice") },
                { "code": "command://dorico/new-slash-voice", "title": qsTrc("dorico", "Create Slash Voice") },
                { "code": "command://dorico/next-voice", "title": qsTrc("dorico", "Next Voice") }
            ] },
        { "code": "command://dorico/cycle-dots", "title": qsTrc("dorico", "Dotted Notes"), "icon": "\uECA5\uECB7",
            "menu": [
                { "code": "command://notation/toggle-dot", "title": qsTrc("dorico", "One Rhythm Dot") },
                { "code": "command://notation/toggle-dot2", "title": qsTrc("dorico", "Two Rhythm Dots") },
                { "code": "command://notation/toggle-dot3", "title": qsTrc("dorico", "Three Rhythm Dots") },
                { "code": "command://notation/toggle-dot4", "title": qsTrc("dorico", "Four Rhythm Dots") }
            ] },
        { "code": "command://dorico/rest", "title": qsTrc("dorico", "Rests"), "icon": "\uE4E5" },
        { "code": "command://notation/add-triplet", "title": qsTrc("dorico", "Tuplets"), "icon": "\uE883\uE88A\uE882",
            "menu": [
                { "code": "command://notation/add-duplet", "title": "2:3" },
                { "code": "command://notation/add-triplet", "title": "3:2" },
                { "code": "command://notation/add-quadruplet", "title": "4:3" },
                { "code": "command://notation/add-quintuplet", "title": "5:4" },
                { "code": "command://notation/add-sextuplet", "title": "6:4" },
                { "code": "command://notation/add-septuplet", "title": "7:8" },
                { "code": "command://dorico/popover/tuplets", "title": "x:y" }
            ] },
        { "code": "command://notation/add-appoggiatura", "title": qsTrc("dorico", "Grace Notes"), "icon": "\uE560",
            "menu": [
                { "code": "command://notation/add-appoggiatura", "title": qsTrc("dorico", "Unslashed") },
                { "code": "command://notation/add-acciaccatura", "title": qsTrc("dorico", "Slashed") }
            ] },
        { "code": "command://notation/toggle-tie", "title": qsTrc("dorico", "Tie"), "icon": "\uE1F0\uE1FD\uE1F0" },
        { "code": "command://dorico/scissors", "title": qsTrc("dorico", "Scissors"), "icon": "svg:scissors" }
    ]

    spacing: 2
    padding: 4

    DoricoToolsModel {
        id: tools
        codes: root.buttons.map(b => b.code)
        Component.onCompleted: tools.load()
    }

    Repeater {
        model: tools.items

        // A button, plus Dorico's divider line under the last button of a group.
        Column {
            id: entry

            required property var modelData
            required property int index

            readonly property var spec: root.buttons[index]
            readonly property bool hasMenu: spec.menu !== undefined

            spacing: root.spacing

            DoricoButton {
                id: button

                doricoIcon: entry.spec.icon
                iconBoxSize: 28
                available: entry.modelData.enabled
                accentButton: entry.modelData.checked
                transparent: !entry.modelData.checked

                toolTipTitle: entry.spec.title
                toolTipDescription: entry.hasMenu ? qsTrc("dorico", "Click and hold for more options") : entry.modelData.description
                toolTipShortcut: entry.modelData.shortcuts

                navigation.panel: root.navigationPanel
                navigation.order: entry.index
                navigation.accessible.name: entry.spec.title

                onTriggered: tools.handleMenuItem(entry.modelData.id)

                DoricoToolsModel {
                    id: menuModel
                    codes: entry.hasMenu ? entry.spec.menu.map(m => m.code) : []
                    titles: entry.hasMenu ? entry.spec.menu.map(m => m.title) : []
                    Component.onCompleted: menuModel.load()
                }

                Connections {
                    target: button.mouseArea
                    enabled: entry.hasMenu

                    function onPressAndHold() {
                        menuLoader.toggleOpened(menuModel.items)
                    }
                }

                StyledMenuLoader {
                    id: menuLoader

                    menuAnchorItem: root.Window.window ? root.Window.window.contentItem : null

                    onHandleMenuItem: function(itemId) {
                        Qt.callLater(menuModel.handleMenuItem, itemId)
                    }
                }
            }

            // Not SeparatorLine: it anchors to the parent's edges (see NotationsToolbox).
            Rectangle {
                visible: entry.spec.groupEnd === true
                width: button.width
                height: 1
                color: ui.theme.strokeColor
            }
        }
    }
}
