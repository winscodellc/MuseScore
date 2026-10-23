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

import Muse.Ui
import Muse.UiComponents

//! Dorico's timecode spin box (spec 91): a masked hh:mm:ss:ff field in overtype mode (digits replace the one at the
//! cursor, the separators are skipped, an impossible digit is ignored), with Up / Down and the arrows stepping the
//! field under the cursor. Drop-frame rates use ';' before the frames.
Rectangle {
    id: root

    property string timecode: "00:00:00:00"
    property bool dropFrame: false
    property int framesPerSecond: 25

    signal edited(string timecode)       // every change
    signal editingFinished(string timecode)
    signal accepted()
    signal escaped()

    // Qt cuts an input mask at its first ';', escaped or not, so drop frame shows U+037E (which looks the same) in the
    // field, and every signal carries a real ';'
    readonly property string semicolon: "\u037E"
    readonly property string shown: root.timecode.replace(";", root.semicolon)
    function out(text) {
        return text.replace(root.semicolon, ";")
    }

    // Back to the given timecode, dropping any abandoned edit
    function reset() {
        input.text = Qt.binding(function() { return root.shown })
    }

    function forceInputFocus() {
        root.reset()
        input.forceActiveFocus()
        input.cursorPosition = 0
    }

    implicitWidth: 112
    implicitHeight: 24
    color: ui.theme.textFieldColor
    border.color: input.activeFocus ? ui.theme.accentColor : ui.theme.strokeColor
    border.width: Math.max(ui.theme.borderWidth, 1)
    radius: 3

    // A click in the padding focuses the field too, so typed digits never reach the score
    MouseArea {
        anchors.fill: parent
        onClicked: input.forceActiveFocus()
    }

    // One step of the field the cursor is in: 0 hours, 1 minutes, 2 seconds, 3 frames
    function step(delta) {
        var fields = root.out(input.text).split(/[:;]/).map(function(f) { return parseInt(f) || 0 })
        if (fields.length !== 4) {
            return
        }
        var field = Math.min(3, Math.floor(input.cursorPosition / 3))
        var max = [99, 59, 59, root.framesPerSecond - 1][field]
        fields[field] = Math.max(0, Math.min(max, fields[field] + delta))
        var cursor = input.cursorPosition
        input.text = fields.map(function(f) { return (f < 10 ? "0" : "") + f }).slice(0, 3).join(":")
                + (root.dropFrame ? root.semicolon : ":") + (fields[3] < 10 ? "0" : "") + fields[3]
        input.cursorPosition = cursor
        root.edited(root.out(input.text))
    }

    TextInput {
        id: input

        anchors.left: parent.left
        anchors.right: arrows.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter

        text: root.shown
        inputMask: "99:99:99" + (root.dropFrame ? root.semicolon : ":") + "99"
        overwriteMode: true
        // The class holds a literal U+037E: the QML regex engine rejects a \u escape and then matches nothing
        validator: RegularExpressionValidator { regularExpression: /\d\d:[0-5]\d:[0-5]\d[:;]\d\d/ }
        font: ui.theme.bodyFont
        color: ui.theme.fontPrimaryColor
        selectByMouse: true

        onTextEdited: root.edited(root.out(text))
        onEditingFinished: root.editingFinished(root.out(text))
        onAccepted: root.accepted()

        // Keep Return, Escape, digits and the arrows from the app's shortcuts; Cmd combos (undo, save...) pass on
        Keys.onShortcutOverride: function(event) {
            event.accepted = !(event.modifiers & (Qt.ControlModifier | Qt.MetaModifier))
        }

        Keys.onUpPressed: root.step(1)
        Keys.onDownPressed: root.step(-1)
        Keys.onEscapePressed: { root.reset(); root.escaped() }
    }

    Column {
        id: arrows

        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: [1, -1]

            StyledIconLabel {
                required property int modelData

                width: 14
                height: (root.height - 4) / 2
                iconCode: modelData > 0 ? IconCode.SMALL_ARROW_UP : IconCode.SMALL_ARROW_DOWN
                font.pixelSize: 10

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.step(parent.modelData)
                }
            }
        }
    }
}
