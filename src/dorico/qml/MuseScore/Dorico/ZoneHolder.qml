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

//! A dock's content that only borrows `item` (spec 95). DockPanel and DockToolBar build their content while open and
//! destroy it on close, so a zone that a mode closes would be rebuilt on the next switch. The zone contents live on
//! the page instead (parked in an invisible item); the dock shows them while it is open and gives them back on close.
Item {
    id: root

    property Item item: null
    property Item parking: null

    implicitWidth: root.item ? root.item.implicitWidth : 0
    implicitHeight: root.item ? root.item.implicitHeight : 0

    Component.onCompleted: {
        root.parking = root.item.parent
        root.item.parent = root
        root.item.anchors.fill = root
    }

    Component.onDestruction: {
        if (root.item && root.item.parent === root) {
            root.item.anchors.fill = undefined
            root.item.parent = root.parking
        }
    }
}
