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
import MuseScore.Preferences

import "prefscolors.js" as PC

//! A MuseScore preferences page inside the dialog (spec 100), for the settings Dorico has no row for. It starts
//! collapsed (a title row), and is only created when first opened. The page keeps MuseScore's own look, in a fixed-height
//! frame that scrolls by itself. Its edits go straight to the settings, like MuseScore's own dialog, so Apply and Close
//! cover them; `applyPage()` and `resetPage()` call the page's own apply() / reset().
Item {
    id: root

    property string page: ""
    property string title: ""
    property var navigationSection: null

    readonly property bool isDark: ui.theme.isDark
    readonly property alias pageItem: loader.item
    property bool expanded: false

    signal pageLoaded(var pageItem)

    function applyPage() {
        return loader.item ? loader.item.apply() : true
    }

    function resetPage() {
        if (loader.item) {
            loader.item.reset()
        }
    }

    implicitHeight: header.height + (root.expanded ? frame.height + 8 : 0)

    Rectangle {
        id: header

        width: parent.width
        height: 36
        radius: 6
        color: headerMouse.containsMouse ? PC.control(root.isDark) : "transparent"

        RowLayout {
            anchors.fill: parent
            spacing: 8

            StyledIconLabel {
                iconCode: root.expanded ? IconCode.SMALL_ARROW_DOWN : IconCode.SMALL_ARROW_RIGHT
            }

            Text {
                Layout.fillWidth: true
                text: root.title
                color: PC.text(root.isDark)
                font.pixelSize: 13
            }
        }

        MouseArea {
            id: headerMouse

            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.expanded = !root.expanded
        }
    }

    Rectangle {
        id: frame

        visible: root.expanded
        anchors.top: header.bottom
        anchors.topMargin: 8
        width: parent.width
        height: 520
        radius: 8
        clip: true
        color: ui.theme.backgroundSecondaryColor
        border.width: 1
        border.color: PC.band(root.isDark)

        Loader {
            id: loader

            anchors.fill: parent
            active: root.expanded || item !== null
            sourceComponent: {
                switch (root.page) {
                case "AdvancedPreferencesPage": return advanced
                case "AppearancePreferencesPage": return appearance
                case "AudioMidiPreferencesPage": return audioMidi
                case "BraillePreferencesPage": return braille
                case "CanvasPreferencesPage": return canvas
                case "FoldersPreferencesPage": return folders
                case "GeneralPreferencesPage": return general
                case "ImportPreferencesPage": return importPage
                case "MidiDeviceMappingPreferencesPage": return midiMapping
                case "NoteInputPreferencesPage": return noteInput
                case "PercussionPreferencesPage": return percussion
                case "SaveAndPublishPreferencesPage": return saveAndPublish
                case "ScorePreferencesPage": return score
                case "ShortcutsPreferencesPage": return shortcuts
                case "VideoPreferencesPage": return video
                }
                return null
            }

            onLoaded: {
                loader.item.navigationSection = root.navigationSection
                root.pageLoaded(loader.item)
            }
        }
    }

    Component { id: advanced; AdvancedPreferencesPage {} }
    Component { id: appearance; AppearancePreferencesPage {} }
    Component { id: audioMidi; AudioMidiPreferencesPage {} }
    Component { id: braille; BraillePreferencesPage {} }
    Component { id: canvas; CanvasPreferencesPage {} }
    Component { id: folders; FoldersPreferencesPage {} }
    Component { id: general; GeneralPreferencesPage {} }
    Component { id: importPage; ImportPreferencesPage {} }
    Component { id: midiMapping; MidiDeviceMappingPreferencesPage {} }
    Component { id: noteInput; NoteInputPreferencesPage {} }
    Component { id: percussion; PercussionPreferencesPage {} }
    Component { id: saveAndPublish; SaveAndPublishPreferencesPage {} }
    Component { id: score; ScorePreferencesPage {} }
    Component { id: shortcuts; ShortcutsPreferencesPage {} }
    Component { id: video; VideoPreferencesPage {} }
}
