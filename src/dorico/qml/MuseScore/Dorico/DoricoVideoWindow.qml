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
import QtQuick.Window
import QtMultimedia

import MuseScore.Dorico

//! The Video window (spec 91 phase B): a separate window shown and hidden with F4, the toolbar's Video button and
//! Window > Video, in every mode. Its size and place are kept for all projects; Shift-drag keeps the picture's shape.
//! Only built with Qt Multimedia (the shell loads it with a Loader).
Window {
    id: root

    title: qsTrc("dorico", "Video")
    color: "black"
    // No keyboard focus: Space, F4 and every other shortcut keep working while it is shown (it has no controls)
    flags: Qt.Tool | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.WindowDoesNotAcceptFocus
    visible: false
    minimumWidth: 160
    minimumHeight: 90

    // Closing with the title bar's button hides it the same way as F4
    onClosing: player.windowVisible = false

    width: 640
    height: 360

    property bool fixingShape: false
    //! For the toolbar's Show Video button
    readonly property alias player: player

    VideoPlayerModel {
        id: player
        onWindowVisibleChanged: root.visible = player.windowVisible
    }

    function keepShape(fromWidth) {
        if (root.fixingShape || !player.shiftHeld()) {
            return
        }
        root.fixingShape = true
        if (fromWidth) {
            root.height = Math.round(root.width / player.aspectRatio)
        } else {
            root.width = Math.round(root.height * player.aspectRatio)
        }
        root.fixingShape = false
    }

    onWidthChanged: { keepShape(true); if (root.visible) saveTimer.restart() }
    onHeightChanged: { keepShape(false); if (root.visible) saveTimer.restart() }

    // Save the place and size after a move or resize settles
    Timer {
        id: saveTimer
        interval: 500
        onTriggered: player.saveGeometry(root.x, root.y, root.width, root.height)
    }
    onXChanged: if (root.visible) saveTimer.restart()
    onYChanged: if (root.visible) saveTimer.restart()

    // The picture is pulled once per display refresh, like Dorico's display link
    FrameAnimation {
        running: root.visible
        onTriggered: player.tick(root.screen ? root.screen.refreshRate : 60)
    }

    VideoOutput {
        id: output
        anchors.fill: parent
        visible: player.inPicture
        fillMode: VideoOutput.PreserveAspectFit
    }

    Component.onCompleted: {
        var g = player.savedGeometry()
        if (g.width > 0 && g.height > 0) {
            root.x = g.x
            root.y = g.y
            root.width = g.width
            root.height = g.height
        }
        player.load()
        player.setVideoSink(output.videoSink)
    }
}
