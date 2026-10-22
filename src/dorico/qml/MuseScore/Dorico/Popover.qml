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
import Muse.GraphicalEffects

//! The popover (spec 20, look: spec 99): grip, icon, entry field, optional Local/Global button. Fills the score view
//! (DoricoPage puts it on the paint view); the box sits above-left of the caret or selection and follows scroll and zoom
//! through the view's matrix, at a fixed size (Dorico 6 does not scale it with zoom).
//! Return applies, Opt-Return applies the local variant, Esc cancels. While the field has focus, no score shortcut fires.
//! Also draws the lyrics box while a lyric is edited in the score (spec 99, the in-score editor of spec 33 does the typing).
Item {
    id: root

    property var view: null // NotationPaintView

    readonly property int statusUnsupported: 2
    readonly property int statusInvalid: 3
    // PopoverMove
    readonly property int moveNextBeat: 0
    readonly property int movePrevBeat: 1
    readonly property int moveNextBar: 2
    readonly property int movePrevBar: 3
    readonly property int moveNextItem: 4
    readonly property int movePrevItem: 5

    // Dorico 6 look, measured on its popovers at 1x (spec 99). Dorico keeps it dark in both themes.
    readonly property color boxColor: "#404347"
    readonly property color fieldColor: "#252C36"
    readonly property color focusColor: "#246DA6"
    readonly property color iconColor: "#DBE0E6"
    readonly property color gripColor: "#8E9196"
    readonly property color errorColor: "#E0453D" // the theme has no error colour
    readonly property int fieldX: 42 // grip and icon sit left of it
    readonly property int fieldHeight: 29
    readonly property int noteOffset: 54 // the box starts this far left of the note's centre, at every zoom

    // Opt-Return, or Return with the person button down
    property bool localScope: false

    PopoverModel {
        id: popoverModel

        // Only while open: the matrix changes on every scroll and zoom frame (spec 96)
        viewMatrix: root.view && (popoverModel.isOpen || popoverModel.lyricsOpen) ? root.view.matrix : undefined
    }

    Component.onCompleted: popoverModel.load()

    Connections {
        target: popoverModel

        function onOpened() {
            box.dragOffset = 0
            root.localScope = false
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

    function placeX(anchor, width) {
        // A note or the caret: from its centre. A wide range: from its left edge.
        const noteX = anchor.width < 60 ? anchor.x + anchor.width / 2 : anchor.x
        return Math.max(0, Math.min(noteX - root.noteOffset, root.width - width))
    }

    // Dorico's handle: a column of three dots. Dragging moves the box vertically, until the next popover opens.
    component DotGrip: Item {
        id: grip

        property Item target: null

        x: 6
        width: 4
        height: 16
        anchors.verticalCenter: parent.verticalCenter

        Column {
            anchors.centerIn: parent
            spacing: 4

            Repeater {
                model: 3

                Rectangle {
                    width: 2
                    height: 2
                    color: root.gripColor
                }
            }
        }

        MouseArea {
            property real pressY: 0

            anchors.fill: parent
            anchors.margins: -4
            cursorShape: Qt.SizeVerCursor
            onPressed: function(mouse) { pressY = mouse.y }
            onPositionChanged: function(mouse) { grip.target.dragOffset += mouse.y - pressY }
        }
    }

    // PopoverDef::glyph: SMuFL text in Bravura Text, one fitted line per "\n" (stacked time signatures), or "svg:<name>".
    component PopoverGlyph: Item {
        id: glyphItem

        property string glyph: ""
        readonly property var lines: glyph.startsWith("svg:") ? [] : glyph.split("\n")

        Column {
            anchors.centerIn: parent

            Repeater {
                model: glyphItem.lines

                Item {
                    id: line

                    required property string modelData
                    // SMuFL glyphs (private use area) in Bravura Text; plain text (3/4, C7, v1.) bold in the UI font, as in Dorico
                    readonly property bool music: /[\uE000-\uF8FF]/.test(modelData)
                    readonly property string family: music ? "Bravura Text" : ui.theme.bodyFont.family

                    width: glyphItem.width
                    height: glyphItem.height / glyphItem.lines.length

                    // Ink bounds at 100 px, as DoricoButton does: glyphs sit at very different heights around the baseline.
                    TextMetrics {
                        id: metrics
                        font.family: line.family
                        font.bold: !line.music
                        font.pixelSize: 100
                        text: line.modelData
                    }

                    Text {
                        readonly property rect ink: metrics.tightBoundingRect
                        readonly property real scale100: ink.width > 0 && ink.height > 0
                                                         ? Math.min(0.85 * line.width / ink.width, line.height / ink.height, 1.0) : 0 // wide icons (C7) clear the grip

                        text: line.modelData
                        font.family: line.family
                        font.bold: !line.music
                        font.pixelSize: Math.max(1, scale100 * 100)
                        renderType: Text.CurveRendering
                        color: root.iconColor
                        x: (line.width - ink.width * scale100) / 2 - ink.x * scale100
                        y: (line.height - ink.height * scale100) / 2 - ink.y * scale100 - baselineOffset
                    }
                }
            }
        }

        Image {
            id: svgImage
            anchors.fill: parent
            visible: false
            source: glyphItem.glyph.startsWith("svg:") ? Qt.resolvedUrl("icons/" + glyphItem.glyph.slice(4) + ".svg") : ""
            sourceSize: Qt.size(width, height)
        }

        EffectColorOverlay {
            anchors.fill: svgImage
            visible: glyphItem.glyph.startsWith("svg:")
            source: svgImage
            color: root.iconColor
        }
    }

    Rectangle {
        id: box

        property real dragOffset: 0
        readonly property rect anchor: popoverModel.anchorRect
        readonly property bool roomAbove: anchor.y - height >= 0

        visible: popoverModel.isOpen

        width: root.fieldX + popoverModel.fieldWidth + (popoverModel.scopeButton ? 34 : 5)
        height: popoverModel.scopeButton ? 49 : 39
        x: root.placeX(anchor, width)
        y: Math.max(0, Math.min(root.height - height,
                                (roomAbove ? anchor.y - height : anchor.y + anchor.height) + dragOffset))

        color: root.boxColor
        radius: 8

        DotGrip {
            target: box
        }

        PopoverGlyph {
            x: 15
            width: 24
            height: 22
            anchors.verticalCenter: parent.verticalCenter
            glyph: popoverModel.glyph
        }

        Rectangle {
            id: field

            x: root.fieldX
            width: popoverModel.fieldWidth
            height: root.fieldHeight
            anchors.verticalCenter: parent.verticalCenter
            radius: 4
            color: root.fieldColor
            border.width: 1
            // Dorico shows nothing for an invalid entry; only our "not supported yet" (a valid Dorico entry) is red.
            border.color: popoverModel.status === root.statusUnsupported ? root.errorColor : root.focusColor

            TextInput {
                id: input

                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                verticalAlignment: TextInput.AlignVCenter
                font.family: ui.theme.bodyFont.family
                font.pixelSize: 13
                color: "#FFFFFF"
                selectionColor: root.focusColor
                selectByMouse: true
                clip: true

                onTextEdited: popoverModel.text = text

                // Accepting every override keeps all keys (letters, Return, Esc, Up/Down, Cmd-Z) in the field.
                Keys.onShortcutOverride: function(event) { event.accepted = true }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        // macOS Option arrives as Alt
                        popoverModel.accept(((event.modifiers & Qt.AltModifier) !== 0) !== root.localScope)
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
                        if (popoverModel.navigates) {
                            popoverModel.move(event.key === Qt.Key_Tab ? root.moveNextBar : root.movePrevBar)
                        }
                        event.accepted = true
                    } else if (popoverModel.navigates && event.key === Qt.Key_Space) {
                        popoverModel.move((event.modifiers & Qt.ShiftModifier) ? root.movePrevBeat : root.moveNextBeat)
                        event.accepted = true
                    } else if (popoverModel.navigates && (event.modifiers & Qt.ControlModifier)
                               && (event.key === Qt.Key_Right || event.key === Qt.Key_Left)) {
                        // macOS Cmd arrives as Control
                        popoverModel.move(event.key === Qt.Key_Right ? root.moveNextItem : root.movePrevItem)
                        event.accepted = true
                    }
                }

                // Clicking elsewhere closes the popover, so it never acts on a selection made after it opened.
                onActiveFocusChanged: {
                    if (!activeFocus && popoverModel.isOpen) {
                        popoverModel.cancel()
                    }
                }
            }
        }

        // Local / Global (Dorico's person button): down = Return applies the local variant, like Opt-Return.
        Rectangle {
            x: field.x + field.width + 6
            width: 22
            height: 22
            anchors.verticalCenter: parent.verticalCenter
            visible: popoverModel.scopeButton
            radius: 4
            color: root.localScope ? root.focusColor : (scopeMouse.containsMouse ? root.fieldColor : "transparent")
            opacity: popoverModel.supportsLocal ? 1 : 0.4

            Image {
                id: personImage
                anchors.centerIn: parent
                visible: false
                source: Qt.resolvedUrl("icons/user.svg")
                sourceSize: Qt.size(13, 13)
            }

            EffectColorOverlay {
                anchors.fill: personImage
                source: personImage
                color: root.iconColor
            }

            // A MouseArea takes no focus, so the click does not close the popover first.
            MouseArea {
                id: scopeMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (popoverModel.supportsLocal) {
                        root.localScope = !root.localScope
                    }
                }
                onContainsMouseChanged: {
                    if (containsMouse) {
                        ui.tooltip.show(parent, popoverModel.supportsLocal ? qsTrc("dorico", "Local / Global")
                                                                           : qsTrc("dorico", "Local / Global: not supported yet"))
                    } else {
                        ui.tooltip.hide(parent)
                    }
                }
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
            width: parent.width
            height: suggestionColumn.implicitHeight + 8
            visible: popoverModel.suggestions.length > 0 && popoverModel.status !== root.statusUnsupported
            color: root.boxColor
            radius: 8

            Column {
                id: suggestionColumn

                y: 4
                width: parent.width

                Repeater {
                    model: popoverModel.suggestions

                    Rectangle {
                        required property string modelData
                        required property int index

                        width: suggestionColumn.width
                        height: 22
                        color: index === popoverModel.currentSuggestion ? root.focusColor : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            x: root.fieldX + 7
                            text: modelData
                            font.family: ui.theme.bodyFont.family
                            font.pixelSize: 13
                            color: "#FFFFFF"
                        }

                        // A MouseArea takes no focus, so the click does not close the popover first.
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                popoverModel.moveSuggestion(index - popoverModel.currentSuggestion)
                                popoverModel.accept(root.localScope)
                                root.giveFocusBackIfClosed()
                            }
                        }
                    }
                }
            }
        }
    }

    // Lyrics (Shift-L): the lyric is typed in the score, so this box only mirrors it: line, text, line stepper and
    // placement arrow. It sits under the lyric so it never covers it. Clicks take no focus, so editing goes on.
    Rectangle {
        id: lyricsBox

        property real dragOffset: 0
        readonly property rect anchor: popoverModel.lyricsAnchor

        visible: popoverModel.lyricsOpen && !popoverModel.isOpen
        width: 255
        height: 39
        x: root.placeX(anchor, width)
        y: Math.max(0, Math.min(root.height - height, anchor.y + anchor.height + 6 + dragOffset))
        color: root.boxColor
        radius: 8

        DotGrip {
            target: lyricsBox
        }

        PopoverGlyph {
            x: 15
            width: 24
            height: 14
            anchors.verticalCenter: parent.verticalCenter
            glyph: "v" + popoverModel.lyricsLine + "."
        }

        Rectangle {
            x: root.fieldX
            width: 133
            height: root.fieldHeight
            anchors.verticalCenter: parent.verticalCenter
            radius: 4
            color: root.fieldColor
            border.width: 1
            border.color: root.focusColor

            Text {
                anchors.fill: parent
                anchors.leftMargin: 7
                anchors.rightMargin: 7
                verticalAlignment: Text.AlignVCenter
                text: popoverModel.lyricsText
                font.family: ui.theme.bodyFont.family
                font.pixelSize: 13
                color: "#FFFFFF"
                elide: Text.ElideLeft
            }
        }

        // Line stepper: up = previous line, down = next line (added when missing), like Up / Down while typing
        Rectangle {
            x: root.fieldX + 133 + 8
            width: 44
            height: root.fieldHeight
            anchors.verticalCenter: parent.verticalCenter
            radius: 4
            color: root.fieldColor

            Text {
                x: 8
                anchors.verticalCenter: parent.verticalCenter
                text: popoverModel.lyricsLine
                font.family: ui.theme.bodyFont.family
                font.pixelSize: 11
                color: "#FFFFFF"
            }

            Column {
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter

                Repeater {
                    model: [-1, 1]

                    StyledIconLabel {
                        required property int modelData

                        width: 12
                        height: 12
                        iconCode: modelData < 0 ? IconCode.SMALL_ARROW_UP : IconCode.SMALL_ARROW_DOWN
                        color: root.iconColor

                        MouseArea {
                            anchors.fill: parent
                            onClicked: popoverModel.lyricsLineStep(parent.modelData)
                        }
                    }
                }
            }
        }

        // Placement: arrow up = lyrics above the staff, down = below (Dorico's staff-relative indicator)
        StyledIconLabel {
            x: root.fieldX + 133 + 8 + 44 + 4
            width: 20
            height: 20
            anchors.verticalCenter: parent.verticalCenter
            iconCode: popoverModel.lyricsAbove ? IconCode.ARROW_UP : IconCode.ARROW_DOWN
            color: root.iconColor

            MouseArea {
                anchors.fill: parent
                onClicked: popoverModel.toggleLyricsPlacement()
            }
        }
    }
}
