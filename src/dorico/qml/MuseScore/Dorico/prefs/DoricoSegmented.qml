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

//! Segmented toggle (spec 100): the selected segment is bright blue with black bold text. `model` is a list of
//! { text, value }.
Rectangle {
    id: root

    property var model: []
    property var currentValue
    property alias count: repeater.count

    signal activated(var value)

    implicitWidth: row.implicitWidth
    implicitHeight: 28
    radius: 4
    color: PC.control(ui.theme.isDark)
    clip: true

    Row {
        id: row

        height: parent.height

        Repeater {
            id: repeater

            model: root.model

            delegate: Rectangle {
                id: segment

                required property var modelData
                required property int index

                readonly property bool selected: segment.modelData.value === root.currentValue

                width: label.implicitWidth + 28
                height: row.height
                color: segment.selected ? PC.accent() : "transparent"

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: segment.modelData.text
                    color: segment.selected ? "#000000" : PC.text(ui.theme.isDark)
                    font.pixelSize: 13
                    font.bold: segment.selected
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: root.enabled
                    onClicked: root.activated(segment.modelData.value)
                }
            }
        }
    }
}
