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

//! A rounded dark card holding label + control rows (spec 100)
Rectangle {
    id: root

    default property alias content: column.data
    property alias spacing: column.spacing

    color: PC.card(ui.theme.isDark)
    radius: 12
    implicitHeight: column.implicitHeight + 24

    Column {
        id: column

        x: 16
        y: 12
        width: parent.width - 32
        spacing: 4
    }
}
