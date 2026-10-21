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

//! A zone panel's title row with an optional "…" menu (spec 95). The zone docks have no dock title bar, because one
//! dock shows every mode's panel and only some of them have a title in Dorico.
RowLayout {
    id: root

    property string title
    property var menuModel: null
    property NavigationPanel navigationPanel: null
    property int navigationOrder: 0

    spacing: 4

    StyledTextLabel {
        Layout.fillWidth: true
        Layout.leftMargin: 12
        Layout.topMargin: 6
        Layout.bottomMargin: 6

        text: root.title
        font: ui.theme.bodyBoldFont
        horizontalAlignment: Qt.AlignLeft
    }

    MenuButton {
        Layout.rightMargin: 12
        width: 20
        height: width

        visible: Boolean(root.menuModel)
        navigation.panel: root.navigationPanel
        navigation.order: root.navigationOrder
        menuModel: root.menuModel

        onHandleMenuItem: function(itemId) {
            root.menuModel.handleMenuItem(itemId)
        }
    }
}
