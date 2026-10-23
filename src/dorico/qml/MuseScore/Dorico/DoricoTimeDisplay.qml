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

//! The mini transport's time display (spec 91): click to cycle bars.beats.sixteenths.ticks, elapsed time and the
//! flow's timecode, as in Dorico. Starts on the format chosen in Preferences > Play.
Rectangle {
    id: root

    property int mode: 0    // 0 bars and beats, 1 elapsed time, 2 timecode

    implicitWidth: 104
    implicitHeight: 24
    radius: 3
    color: ui.theme.textFieldColor

    VideoModel {
        id: video
        Component.onCompleted: {
            video.load()
            root.mode = video.defaultTimeDisplay()
        }
    }

    Text {
        anchors.centerIn: parent
        text: [video.playheadBars, video.playheadElapsed, video.playheadTimecode][root.mode]
        font.family: ui.theme.bodyFont.family
        font.pixelSize: ui.theme.bodyFont.pixelSize
        font.features: { "tnum": 1 }   // tabular digits, so the text does not jitter while playing
        color: ui.theme.fontPrimaryColor
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.mode = (root.mode + 1) % 3
    }
}
