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

import "prefscolors.js" as PC

//! A dark full-width section title (bold 13 px) above a card; `page` makes it the lighter 16 px page title (spec 100)
Rectangle {
    id: root

    property string title: ""
    property bool page: false
    property bool museScore: false

    height: root.page ? 40 : 30
    color: root.page ? PC.pageBand(ui.theme.isDark) : PC.band(ui.theme.isDark)

    Text {
        anchors.verticalCenter: parent.verticalCenter
        x: 16
        text: root.title
        color: PC.text(ui.theme.isDark)
        font.pixelSize: root.page ? 16 : 13
        font.bold: !root.page
    }
}
