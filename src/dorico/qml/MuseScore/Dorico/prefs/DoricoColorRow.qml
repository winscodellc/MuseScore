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

import "prefscolors.js" as PC

//! Hex field + swatch + reset (spec 100). `value` is "#rrggbb"; `defaultValue` is what reset restores.
RowLayout {
    id: root

    property string value: "#000000"
    property string defaultValue: value

    signal colorEdited(string newValue)

    spacing: 8

    TextInputField {
        id: field

        Layout.preferredWidth: 96

        currentText: root.value
        maximumLength: 7
        enabled: root.enabled

        onTextEditingFinished: function(newValue) {
            const s = ("" + newValue).trim()
            if (/^#[0-9a-fA-F]{6}$/.test(s)) {
                root.colorEdited(s.toLowerCase())
            } else {
                field.currentText = root.value
            }
        }
    }

    Rectangle {
        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        radius: 4
        color: root.value
        border.width: 1
        border.color: PC.dimText(ui.theme.isDark)
    }

    FlatButton {
        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        minWidth: 28
        text: "↺"
        enabled: root.enabled && root.value !== root.defaultValue
        onClicked: root.colorEdited(root.defaultValue)
    }
}
