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
import QtQuick.Controls

import "prefscolors.js" as PC

//! Blue round "?" whose tooltip is the help text (spec 100)
Rectangle {
    id: root

    property string text: ""

    width: 16
    height: 16
    radius: 8
    color: PC.accent()

    Text {
        anchors.centerIn: parent
        text: "?"
        color: "#000000"
        font.pixelSize: 11
        font.bold: true
    }

    HoverHandler { id: hover }

    ToolTip.visible: hover.hovered && root.text !== ""
    ToolTip.text: root.text
}
