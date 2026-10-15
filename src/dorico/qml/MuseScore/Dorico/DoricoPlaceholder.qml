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

import Muse.UiComponents

//! Shared stub for Dorico panels and tabs that have no MuseScore backing yet.
Item {
    property string text: qsTrc("dorico", "Not available yet")

    StyledTextLabel {
        anchors.centerIn: parent
        width: parent.width - 24
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        text: parent.text
        opacity: 0.6
    }
}
