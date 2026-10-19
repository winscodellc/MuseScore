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

import "trackcolor.js" as TrackColor

//! Play mode's track overview (spec 57): track headers on the left (colour strip, name, mute / solo; click one to show
//! it in the Track Inspector), a bar ruler, a block in the track's colour wherever an instrument has notes, the
//! playhead, and a horizontal zoom slider at the bottom left.
Rectangle {
    id: root

    property NavigationSection navigationSection: null
    property int contentNavigationPanelOrderStart: 1

    property string selectedPartId: ""
    signal trackClicked(string partId)

    readonly property int headerWidth: 160
    readonly property int rulerHeight: 24
    readonly property int trackHeight: 48
    readonly property int zoomBarHeight: 28
    // Horizontal zoom (the slider), relative to fitting the piece to the width
    readonly property real zoom: zoomSlider.value
    // Fit the piece to the width, but never squeeze a quarter note below 8 px
    readonly property real fitPxPerQuarter: overview.length > 0 ? Math.max(8, (flick.width - 16) / overview.length) : 8
    readonly property real pxPerQuarter: root.fitPxPerQuarter * root.zoom

    function trackColor(index) {
        return TrackColor.trackColor(index, overview.tracks.length)
    }
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
        anchors.bottom: zoomBar.top
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
                    required property int index

                    readonly property bool selected: header.modelData.partId === root.selectedPartId
                    readonly property var soloMute: overview.soloMute[header.modelData.partId] ?? { "muted": false, "solo": false }

                    width: root.headerWidth
                    height: root.trackHeight
                    // Selected: filled with a light tint of the track colour and dark text, as in Dorico
                    color: header.selected ? Qt.tint(root.trackColor(header.index), "#99FFFFFF") : ui.theme.backgroundSecondaryColor

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.trackClicked(header.modelData.partId)
                    }

                    Rectangle {
                        width: 6
                        height: parent.height
                        color: root.trackColor(header.index)
                    }

                    StyledTextLabel {
                        anchors.left: parent.left
                        anchors.right: soloMute.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 14
                        anchors.rightMargin: 4
                        horizontalAlignment: Text.AlignLeft
                        font: header.selected ? ui.theme.bodyBoldFont : ui.theme.bodyFont
                        color: header.selected ? "#000000" : ui.theme.fontPrimaryColor
                        text: header.modelData.title
                    }

                    Row {
                        id: soloMute

                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        FlatButton {
                            width: 24
                            height: 24
                            text: "m"
                            toolTipTitle: qsTrc("dorico", "Mute")
                            accentButton: header.soloMute.muted
                            onClicked: overview.setMuted(header.modelData.partId, !header.soloMute.muted)
                        }

                        FlatButton {
                            width: 24
                            height: 24
                            text: "s"
                            toolTipTitle: qsTrc("dorico", "Solo")
                            accentButton: header.soloMute.solo
                            onClicked: overview.setSolo(header.modelData.partId, !header.soloMute.solo)
                        }
                    }

                    SeparatorLine { anchors.bottom: parent.bottom }
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
        anchors.bottom: zoomBar.top

        contentWidth: overview.length * root.pxPerQuarter + 16
        contentHeight: overview.tracks.length * root.trackHeight

        Column {
            Repeater {
                model: overview.tracks

                Rectangle {
                    id: track

                    required property var modelData
                    required property int index

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
                            color: Utils.colorWithAlpha(root.trackColor(track.index), 0.75)
                            border.color: root.trackColor(track.index)
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

    // Horizontal zoom: 1x fits the piece to the width.
    Item {
        id: zoomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: root.zoomBarHeight

        SeparatorLine { anchors.top: parent.top }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            FlatButton {
                width: 20
                height: 20
                icon: IconCode.ZOOM_OUT
                transparent: true
                toolTipTitle: qsTrc("dorico", "Zoom out")
                onClicked: zoomSlider.value = Math.max(zoomSlider.from, zoomSlider.value / 1.5)
            }

            StyledSlider {
                id: zoomSlider

                width: 120
                anchors.verticalCenter: parent.verticalCenter
                from: 1
                to: 16
                stepSize: 0
                value: 1
            }

            FlatButton {
                width: 20
                height: 20
                icon: IconCode.ZOOM_IN
                transparent: true
                toolTipTitle: qsTrc("dorico", "Zoom in")
                onClicked: zoomSlider.value = Math.min(zoomSlider.to, zoomSlider.value * 1.5)
            }
        }
    }
}
