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
import MuseScore.NotationScene

//! Engrave mode's Pages panel (right zone, spec 56). Pages shows MuseScore's own page navigator (spec 101, P8).
Column {
    id: pagesColumn

    spacing: 2

    Repeater {
        model: [
            { "title": qsTrc("dorico", "Pages"), "open": true },
            { "title": qsTrc("dorico", "Page Templates"), "open": true },
            { "title": qsTrc("dorico", "Flow Headings"), "open": false },
            { "title": qsTrc("dorico", "Page Template Sets"), "open": false }
        ]

        InspectorSection {
            id: section

            required property var modelData

            width: pagesColumn.width
            title: modelData.title
            expanded: modelData.open

            readonly property bool isPages: modelData.title === qsTrc("dorico", "Pages")

            NotationNavigator {
                visible: section.isPages
                width: parent.width
                height: section.isPages ? 220 : 0

                Component.onCompleted: load()
            }

            StyledTextLabel {
                visible: !section.isPages
                width: parent.width
                text: qsTrc("dorico", "Not available yet")
                opacity: 0.6
                horizontalAlignment: Text.AlignLeft
            }
        }
    }
}
