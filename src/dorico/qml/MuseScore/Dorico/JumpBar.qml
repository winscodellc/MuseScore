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

//! The jump bar (spec 46): Commands mode runs a command by name, Go To mode jumps to a bar, rehearsal mark or page.
//! Fills the score view (DoricoPage puts it on the paint view); the box sits at the top centre.
//! Return applies, Opt-Return assigns an alias without running, Esc closes. While the field has focus, no score shortcut fires.
Item {
    id: root

    property var view: null // NotationPaintView

    readonly property int modeCommands: 0
    readonly property int modeGoTo: 1
    // The theme has no error colour.
    readonly property color errorColor: "#E0453D"
    // macOS Ctrl arrives as Meta; elsewhere Ctrl is Ctrl
    readonly property int ctrlModifier: Qt.platform.os === "osx" ? Qt.MetaModifier : Qt.ControlModifier

    JumpBarModel {
        id: jumpBarModel
    }

    Component.onCompleted: jumpBarModel.load()

    Connections {
        target: jumpBarModel

        function onOpened() {
            input.text = jumpBarModel.text
            if (jumpBarModel.mode === root.modeGoTo) {
                input.cursorPosition = input.length // "b" is typed on from
            } else {
                input.selectAll() // the last command is replaced by typing
            }
            input.forceActiveFocus()
        }
    }

    function giveFocusBackIfClosed() {
        if (!jumpBarModel.isOpen && root.view) {
            root.view.forceFocusIn()
        }
    }

    Rectangle {
        id: box

        visible: jumpBarModel.isOpen

        width: 440
        height: header.height + field.height + (results.visible ? results.height : 0) + 12
        x: Math.max(0, (root.width - width) / 2)
        y: 24

        color: ui.theme.popupBackgroundColor
        radius: 4
        border.width: 1
        border.color: jumpBarModel.invalid ? root.errorColor : ui.theme.strokeColor

        Column {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 4

            Row {
                id: header

                spacing: 12

                Repeater {
                    model: [qsTrc("dorico", "Commands"), qsTrc("dorico", "Go To")]

                    StyledTextLabel {
                        required property string modelData
                        required property int index

                        text: modelData
                        font: index === jumpBarModel.mode ? ui.theme.bodyBoldFont : ui.theme.bodyFont
                        opacity: index === jumpBarModel.mode ? 1 : 0.6

                        // A MouseArea takes no focus, so the click does not close the bar first.
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                jumpBarModel.setMode(index)
                                input.forceActiveFocus()
                            }
                        }
                    }
                }
            }

            Item {
                id: field

                width: parent.width
                height: 24

                TextInput {
                    id: input

                    anchors.fill: parent
                    verticalAlignment: TextInput.AlignVCenter
                    font: ui.theme.bodyFont
                    color: jumpBarModel.invalid ? root.errorColor : ui.theme.fontPrimaryColor
                    selectByMouse: true
                    clip: true

                    onTextEdited: jumpBarModel.text = text

                    // Accepting every override keeps all keys (letters, Return, Esc, Up/Down, Cmd-Z) in the field.
                    Keys.onShortcutOverride: function(event) { event.accepted = true }

                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            // macOS Option arrives as Alt
                            jumpBarModel.accept((event.modifiers & Qt.AltModifier) !== 0)
                            root.giveFocusBackIfClosed()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Escape) {
                            jumpBarModel.close()
                            root.giveFocusBackIfClosed()
                            event.accepted = true
                        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                            jumpBarModel.moveRow(event.key === Qt.Key_Down ? 1 : -1)
                            event.accepted = true
                        } else if ((event.modifiers & root.ctrlModifier) && (event.key === Qt.Key_1 || event.key === Qt.Key_2)) {
                            jumpBarModel.setMode(event.key === Qt.Key_1 ? root.modeCommands : root.modeGoTo)
                            event.accepted = true
                        }
                    }

                    // Clicking elsewhere closes the bar, so it never acts on a selection made after it opened.
                    onActiveFocusChanged: {
                        if (!activeFocus && jumpBarModel.isOpen) {
                            jumpBarModel.close()
                        }
                    }
                }
            }

            Column {
                id: results

                width: parent.width
                visible: jumpBarModel.rows.length > 0

                Repeater {
                    model: jumpBarModel.rows

                    Rectangle {
                        required property var modelData
                        required property int index

                        width: results.width
                        height: 22
                        radius: 2
                        color: index === jumpBarModel.currentRow ? ui.theme.accentColor : "transparent"

                        StyledTextLabel {
                            anchors.left: parent.left
                            anchors.leftMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.title
                        }

                        StyledTextLabel {
                            anchors.right: parent.right
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.detail
                            opacity: 0.7
                        }

                        // A MouseArea takes no focus, so the click does not close the bar first.
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                jumpBarModel.moveRow(index - jumpBarModel.currentRow)
                                jumpBarModel.accept(false)
                                root.giveFocusBackIfClosed()
                            }
                        }
                    }
                }
            }
        }
    }
}
