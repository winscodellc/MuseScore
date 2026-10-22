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
    readonly property int toolbarHeight: 28
    readonly property int rulerHeight: 24
    // Read-only Tempo and Chords lanes under the ruler (spec 101, P9)
    readonly property int laneHeight: 22
    readonly property int topInset: toolbarHeight + rulerHeight + 2 * laneHeight
    readonly property int trackHeight: 48
    readonly property int zoomBarHeight: 28
    // Horizontal zoom (the slider), relative to fitting the piece to the width
    readonly property real zoom: zoomSlider.value
    // Fit the piece to the width, but never squeeze a quarter note below 8 px
    readonly property real fitPxPerQuarter: overview.length > 0 ? Math.max(8, (flick.width - 16) / overview.length) : 8
    readonly property real pxPerQuarter: root.fitPxPerQuarter * root.zoom

    // Read once per change: each `overview.tracks` read converts the whole list (spec 96)
    readonly property int trackCount: overview.tracks.length

    function trackColor(index) {
        return TrackColor.trackColor(index, root.trackCount)
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

    // Track tools: Dorico's row above the headers. No MuseScore commands behind them yet.
    Row {
        id: trackTools

        x: 6
        height: root.toolbarHeight
        spacing: 4

        Repeater {
            model: [
                { "icon": "svg:arrow-down-wide-narrow", "title": qsTrc("dorico", "Sort tracks") },
                { "icon": "svg:eye", "title": qsTrc("dorico", "Show/hide tracks") },
                { "icon": "svg:fold-vertical", "title": qsTrc("dorico", "Track height") }
            ]

            DoricoButton {
                required property var modelData

                anchors.verticalCenter: parent.verticalCenter
                enabled: false
                transparent: true
                doricoIcon: modelData.icon
                iconBoxSize: 16
                toolTipTitle: modelData.title
                toolTipDescription: qsTrc("dorico", "Not available yet")
            }
        }
    }

    // Ruler: scrolls horizontally with the tracks, stays at the top
    Item {
        id: ruler

        anchors.left: headers.right
        anchors.right: parent.right
        y: root.toolbarHeight
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

    // Tempo and Chords lanes: marks of the score, scrolling with the tracks
    Repeater {
        model: [
            { "title": qsTrc("dorico", "Tempo"), "row": 0, "marks": overview.tempos },
            { "title": qsTrc("dorico", "Chords"), "row": 1, "marks": overview.chords }
        ]

        Item {
            id: lane

            required property var modelData

            y: root.toolbarHeight + root.rulerHeight + lane.modelData.row * root.laneHeight
            width: root.width
            height: root.laneHeight

            StyledTextLabel {
                x: 14
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                text: lane.modelData.title
            }

            Item {
                x: root.headerWidth
                width: parent.width - root.headerWidth
                height: parent.height
                clip: true

                Item {
                    x: -flick.contentX
                    height: parent.height

                    Repeater {
                        model: lane.modelData.marks

                        StyledTextLabel {
                            required property var modelData

                            x: modelData.start * root.pxPerQuarter + 2
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.text
                            font: ui.theme.bodyBoldFont
                        }
                    }
                }
            }

            SeparatorLine { anchors.bottom: parent.bottom }
        }
    }

    // Track headers: scroll vertically with the tracks
    Item {
        id: headers

        anchors.top: parent.top
        anchors.topMargin: root.topInset
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
        anchors.topMargin: root.topInset
        anchors.left: headers.right
        anchors.right: parent.right
        anchors.bottom: zoomBar.top

        contentWidth: overview.length * root.pxPerQuarter + 16
        contentHeight: root.trackCount * root.trackHeight

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

                    // Mini piano roll: one dash per note, higher pitch higher up (Dorico's blocks show the notes)
                    Repeater {
                        model: track.modelData.notes

                        Rectangle {
                            required property var modelData

                            x: modelData[0] * root.pxPerQuarter
                            y: 10 + (1 - (Math.max(21, Math.min(108, modelData[2])) - 21) / 87) * (root.trackHeight - 22)
                            width: Math.max(1.5, modelData[1] * root.pxPerQuarter - 1)
                            height: 1.5
                            color: "#FFFFFF"
                            opacity: 0.85
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
