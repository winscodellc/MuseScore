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

//! Play mode's track overview (spec 57): track headers on the left (click one to show it in the Track Inspector),
//! a bar ruler, a block wherever an instrument has notes, and the playhead.
Rectangle {
    id: root

    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 1

    property string selectedPartId: ""
    signal trackClicked(string partId)

    readonly property int headerWidth: 160
    readonly property int rulerHeight: 24
    readonly property int trackHeight: 48
    // Fit the piece to the width, but never squeeze a quarter note below 8 px
    readonly property real pxPerQuarter: overview.length > 0 ? Math.max(8, (flick.width - 16) / overview.length) : 8
    readonly property real playheadX: overview.playheadPosition * pxPerQuarter
    // Label every n-th bar so numbers do not overlap
    readonly property int barLabelStep: overview.bars.length > 0
                                        ? Math.max(1, Math.ceil(32 / (overview.length * pxPerQuarter / overview.bars.length))) : 1

    color: ui.theme.backgroundPrimaryColor

    TrackOverviewModel {
        id: overview
        Component.onCompleted: overview.load()
    }

    // Keep the playhead in view while playing
    onPlayheadXChanged: {
        if (root.playheadX < flick.contentX || root.playheadX > flick.contentX + flick.width - 16) {
            flick.contentX = Math.max(0, Math.min(root.playheadX - 16, flick.contentWidth - flick.width))
        }
    }

    // Ruler: scrolls horizontally with the tracks, stays at the top
    Item {
        id: ruler

        anchors.left: headers.right
        anchors.right: parent.right
        height: root.rulerHeight
        clip: true

        Item {
            x: -flick.contentX
            height: parent.height

            Repeater {
                model: overview.bars

                Item {
                    id: barMark

                    required property var modelData
                    required property int index

                    x: barMark.modelData.start * root.pxPerQuarter
                    height: root.rulerHeight

                    Rectangle {
                        width: 1
                        height: barMark.index % root.barLabelStep === 0 ? parent.height : parent.height / 3
                        anchors.bottom: parent.bottom
                        color: ui.theme.strokeColor
                    }

                    StyledTextLabel {
                        x: 3
                        y: 2
                        visible: barMark.index % root.barLabelStep === 0
                        text: barMark.modelData.number
                    }
                }
            }

            Rectangle {
                x: root.playheadX
                width: 2
                height: root.rulerHeight
                color: ui.theme.accentColor
            }
        }

        SeparatorLine { anchors.bottom: parent.bottom }
    }

    // Track headers: scroll vertically with the tracks
    Item {
        id: headers

        anchors.top: parent.top
        anchors.topMargin: root.rulerHeight
        anchors.bottom: parent.bottom
        width: root.headerWidth
        clip: true

        Column {
            y: -flick.contentY
            width: parent.width

            Repeater {
                model: overview.tracks

                Rectangle {
                    id: header

                    required property var modelData

                    readonly property bool selected: header.modelData.partId === root.selectedPartId

                    width: root.headerWidth
                    height: root.trackHeight
                    color: header.selected ? Utils.colorWithAlpha(ui.theme.accentColor, 0.3) : ui.theme.backgroundSecondaryColor

                    StyledTextLabel {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        horizontalAlignment: Text.AlignLeft
                        font: header.selected ? ui.theme.bodyBoldFont : ui.theme.bodyFont
                        text: header.modelData.title
                    }

                    SeparatorLine { anchors.bottom: parent.bottom }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.trackClicked(header.modelData.partId)
                    }
                }
            }
        }

        SeparatorLine {
            anchors.right: parent.right
            orientation: Qt.Vertical
        }
    }

    StyledFlickable {
        id: flick

        anchors.top: parent.top
        anchors.topMargin: root.rulerHeight
        anchors.left: headers.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        contentWidth: overview.length * root.pxPerQuarter + 16
        contentHeight: overview.tracks.length * root.trackHeight

        Column {
            Repeater {
                model: overview.tracks

                Rectangle {
                    id: track

                    required property var modelData

                    width: flick.contentWidth
                    height: root.trackHeight
                    color: track.modelData.partId === root.selectedPartId ? Utils.colorWithAlpha(ui.theme.accentColor, 0.1)
                                                                          : "transparent"

                    Repeater {
                        model: track.modelData.blocks

                        Rectangle {
                            id: block

                            required property var modelData

                            x: block.modelData.start * root.pxPerQuarter
                            y: 8
                            width: (block.modelData.end - block.modelData.start) * root.pxPerQuarter
                            height: root.trackHeight - 16
                            radius: 3
                            color: Utils.colorWithAlpha(ui.theme.accentColor, 0.5)
                            border.color: ui.theme.accentColor
                        }
                    }

                    SeparatorLine { anchors.bottom: parent.bottom }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.trackClicked(track.modelData.partId)
                    }
                }
            }
        }

        // Bar lines over all tracks
        Repeater {
            model: overview.bars

            Rectangle {
                required property var modelData

                x: modelData.start * root.pxPerQuarter
                width: 1
                height: flick.contentHeight
                color: ui.theme.strokeColor
                opacity: 0.5
            }
        }

        Rectangle {
            x: root.playheadX
            width: 2
            height: Math.max(flick.contentHeight, flick.height)
            color: ui.theme.accentColor
        }
    }
}
