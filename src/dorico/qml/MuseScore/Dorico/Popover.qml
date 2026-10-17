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

//! The popover (spec 20): drag handle, icon, entry field. Fills the score view (DoricoPage puts it on the paint view);
//! the box sits above the caret or selection and follows scroll and zoom through the view's matrix.
//! Return applies, Opt-Return applies the local variant, Esc cancels. While the field has focus, no score shortcut fires.
Item {
    id: root

    property var view: null // NotationPaintView

    readonly property int statusUnsupported: 2
    readonly property int statusInvalid: 3
    // The theme has no error colour.
    readonly property color errorColor: "#E0453D"

    PopoverModel {
        id: popoverModel

        viewMatrix: root.view ? root.view.matrix : undefined
    }

    Component.onCompleted: popoverModel.load()

    Connections {
        target: popoverModel

        function onOpened() {
            box.dragOffset = 0
            input.text = popoverModel.text
            input.selectAll()
            input.forceActiveFocus()
        }
    }

    function giveFocusBackIfClosed() {
        if (!popoverModel.isOpen && root.view) {
            root.view.forceFocusIn()
        }
    }

    Rectangle {
        id: box

        property real dragOffset: 0
        readonly property rect anchor: popoverModel.anchorRect
        readonly property real gap: 4
        readonly property bool roomAbove: anchor.y - height - gap >= 0
        readonly property bool invalid: popoverModel.status === root.statusInvalid
                                        || popoverModel.status === root.statusUnsupported

        visible: popoverModel.isOpen

        width: row.implicitWidth + 12
        height: 32
        x: Math.max(0, Math.min(anchor.x, root.width - width))
        y: Math.max(0, Math.min(root.height - height,
                                (roomAbove ? anchor.y - height - gap : anchor.y + anchor.height + gap) + dragOffset))

        color: ui.theme.popupBackgroundColor
        radius: 4
        border.width: 1
        border.color: invalid ? root.errorColor : ui.theme.strokeColor

        Row {
            id: row

            anchors.verticalCenter: parent.verticalCenter
            x: 4
            spacing: 6

            // Drag handle: moves the box vertically, until the next popover opens.
            Rectangle {
                width: 6
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                radius: 2
                color: ui.theme.strokeColor

                MouseArea {
                    property real pressY: 0

                    anchors.fill: parent
                    anchors.margins: -2
                    cursorShape: Qt.SizeVerCursor
                    onPressed: function(mouse) { pressY = mouse.y }
                    onPositionChanged: function(mouse) { box.dragOffset += mouse.y - pressY }
                }
            }

            StyledIconLabel {
                anchors.verticalCenter: parent.verticalCenter
                iconCode: popoverModel.iconCode
            }

            TextInput {
                id: input

                width: 180
                anchors.verticalCenter: parent.verticalCenter
                font: ui.theme.bodyFont
                color: ui.theme.fontPrimaryColor
                selectByMouse: true
                clip: true

                onTextEdited: popoverModel.text = text

                // Accepting every override keeps all keys (letters, Return, Esc, Up/Down, Cmd-Z) in the field.
                Keys.onShortcutOverride: function(event) { event.accepted = true }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        // macOS Option arrives as Alt
                        popoverModel.accept((event.modifiers & Qt.AltModifier) !== 0)
                        root.giveFocusBackIfClosed()
                        event.accepted = true
                    } else if (event.key === Qt.Key_Escape) {
                        popoverModel.cancel()
                        root.giveFocusBackIfClosed()
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Down) {
                        popoverModel.moveSuggestion(event.key === Qt.Key_Down ? 1 : -1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                        event.accepted = true // in-popover navigation (specs 32, 33) comes later
                    }
                }

                // Clicking elsewhere closes the popover, so it never acts on a selection made after it opened.
                onActiveFocusChanged: {
                    if (!activeFocus && popoverModel.isOpen) {
                        popoverModel.cancel()
                    }
                }
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                visible: popoverModel.supportsLocal
                text: qsTrc("dorico", "Global")
                opacity: 0.7
            }
        }

        StyledTextLabel {
            anchors.top: parent.bottom
            anchors.topMargin: 2
            anchors.left: parent.left
            visible: popoverModel.status === root.statusUnsupported
            text: qsTrc("dorico", "Not supported yet")
            color: root.errorColor
        }

        // Suggestion menu (spec 26): Down / Up highlight an entry, Return or a click applies it.
        Rectangle {
            anchors.top: parent.bottom
            anchors.topMargin: 2
            anchors.left: parent.left
            width: parent.width // wider than any suggestion: the entry field alone is 180 px
            height: suggestionColumn.implicitHeight + 4
            visible: popoverModel.suggestions.length > 0 && popoverModel.status !== root.statusUnsupported
            color: ui.theme.popupBackgroundColor
            radius: 4
            border.width: 1
            border.color: ui.theme.strokeColor

            Column {
                id: suggestionColumn

                y: 2
                width: parent.width

                Repeater {
                    model: popoverModel.suggestions

                    Rectangle {
                        required property string modelData
                        required property int index

                        width: suggestionColumn.width
                        height: 22
                        color: index === popoverModel.currentSuggestion ? ui.theme.accentColor : "transparent"

                        StyledTextLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            x: 8
                            text: modelData
                        }

                        // A MouseArea takes no focus, so the click does not close the popover first.
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                popoverModel.moveSuggestion(index - popoverModel.currentSuggestion)
                                popoverModel.accept(false)
                                root.giveFocusBackIfClosed()
                            }
                        }
                    }
                }
            }
        }
    }
}
