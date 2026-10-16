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

import MuseScore.Playback

//! A dropdown-style picker for a mixer channel's sound or effect slot (spec 57). The menu is MuseScore's own
//! (the model's available resources, the same list the mixer's sound / FX buttons show); "---" for an empty slot.
Rectangle {
    id: root

    property AbstractAudioResourceItem resourceItemModel: null
    readonly property bool usable: root.resourceItemModel !== null && root.enabled

    height: 24
    radius: 3
    color: ui.theme.textFieldColor
    border.width: 1
    border.color: mouseArea.containsMouse && root.usable ? ui.theme.accentColor : ui.theme.strokeColor
    opacity: root.usable ? 1.0 : ui.theme.itemOpacityDisabled

    StyledTextLabel {
        anchors.left: parent.left
        anchors.right: arrow.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignLeft
        text: root.resourceItemModel && !root.resourceItemModel.isBlank ? root.resourceItemModel.title : "---"
    }

    StyledIconLabel {
        id: arrow

        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        iconCode: IconCode.SMALL_ARROW_DOWN
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.usable
        onClicked: root.resourceItemModel.requestAvailableResources()
    }

    StyledMenuLoader {
        id: menuLoader

        isSearchable: true

        onHandleMenuItem: function(itemId) {
            if (root.resourceItemModel) {
                Qt.callLater(root.resourceItemModel.handleMenuItem, itemId)
            }
        }
    }

    Connections {
        target: root.resourceItemModel

        function onAvailableResourceListResolved(resources) {
            menuLoader.toggleOpened(resources)
        }
    }
}
