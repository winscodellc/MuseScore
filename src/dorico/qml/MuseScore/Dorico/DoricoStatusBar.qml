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
import MuseScore.AppShell
import MuseScore.NotationScene
import MuseScore.Playback

//! Dorico status bar (spec 51). `items` lists which controls this mode shows; the order below is Dorico's order.
//! Names: grid, pitch, order, summary, zoneArrow, follow, tools, viewType, arrangement, zoom, midi.
Item {
    id: root

    property var items: []

    function shows(name) {
        return root.items.indexOf(name) !== -1
    }

    readonly property string notAvailable: qsTrc("dorico", "Not available yet")

    property NavigationSection navigationSection: NavigationSection {
        name: "DoricoStatusBar"
        enabled: root.enabled && root.visible
        order: 8
    }

    NotationStatusBarModel {
        id: model
    }

    DoricoStatusModel {
        id: status

        Component.onCompleted: status.load()
    }

    DoricoToolsModel {
        id: tools

        codes: [
            "command://dorico/toggle-zone-lower",
            "command://playback/pan-toggle"
        ]

        Component.onCompleted: tools.load()
    }

    NavigationPanel {
        id: navPanel
        name: "DoricoStatusBar"
        enabled: root.enabled && root.visible
        order: 0
        direction: NavigationPanel.Horizontal
        section: root.navigationSection
    }

    // A control Dorico has but MuseScore cannot back yet: shown disabled so the layout matches.
    component StubButton: DoricoButton {
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredHeight: 28

        transparent: true
        enabled: false
        toolTipDescription: root.notAvailable
    }

    // FlatButton has no `checked`: a highlighted (accent) button is the "on" state.
    component ToggleButton: DoricoButton {
        property bool active: false

        Layout.alignment: Qt.AlignVCenter
        Layout.preferredHeight: 28

        transparent: !active
        accentButton: active
    }

    RowLayout {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 4

        height: parent.height

        spacing: 4

        // ---- left group ----

        StubButton {
            visible: root.shows("grid")
            text: qsTrc("dorico", "Grid: 1/4")
            toolTipTitle: qsTrc("dorico", "Rhythmic grid")
        }

        SeparatorLine { orientation: Qt.Vertical; visible: root.shows("grid") }

        ToggleButton {

            visible: root.shows("pitch")
            active: model.concertPitchItem.checked
            enabled: model.concertPitchItem.enabled
            text: qsTrc("dorico", "Concert")

            navigation.panel: navPanel
            navigation.order: 1

            onClicked: if (!active) model.toggleConcertPitch()
        }

        ToggleButton {

            visible: root.shows("pitch")
            active: !model.concertPitchItem.checked
            enabled: model.concertPitchItem.enabled
            text: qsTrc("dorico", "Transposed")

            navigation.panel: navPanel
            navigation.order: 2

            onClicked: if (!active) model.toggleConcertPitch()
        }

        SeparatorLine { orientation: Qt.Vertical; visible: root.shows("pitch") }

        StubButton {
            visible: root.shows("order")
            text: qsTrc("dorico", "Order")
            toolTipTitle: qsTrc("dorico", "Instrument order")
        }

        StyledTextLabel {
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true

            visible: root.shows("summary")
            text: status.summary
            horizontalAlignment: Text.AlignLeft
        }

        Item {
            Layout.fillWidth: true
            visible: !root.shows("summary")
        }

        PlaybackLoadingInfo {
            id: playbackLoadingInfo

            Layout.fillWidth: false

            onStarted: visible = true
            onFinished: visible = false
        }

        OnlineSoundsStatusView {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 28

            navigationPanel: navPanel
            navigationOrder: 3
        }

        // ---- right group ----

        FlatButton {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 28

            visible: root.shows("zoneArrow")
            transparent: true
            icon: IconCode.SMALL_ARROW_UP
            toolTipTitle: qsTrc("dorico", "Show/hide lower zone")

            navigation.panel: navPanel
            navigation.order: 4

            onClicked: tools.handleMenuItem(tools.items[0].id)
        }

        ToggleButton {

            visible: root.shows("follow")
            active: tools.items.length > 1 && tools.items[1].checked
            doricoIcon: "svg:locate-fixed"
            toolTipTitle: qsTrc("dorico", "Follow playhead")

            navigation.panel: navPanel
            navigation.order: 5

            onClicked: tools.handleMenuItem(tools.items[1].id)
        }

        StubButton {
            visible: root.shows("tools")
            doricoIcon: "svg:square-dashed"
            toolTipTitle: qsTrc("dorico", "Marquee tool")
        }

        SeparatorLine { orientation: Qt.Vertical; visible: root.shows("viewType") }

        // Page / Galley / Fill -> MuseScore page / continuous / float view (approximation, see spec 51)
        Repeater {
            model: root.shows("viewType") ? [
                { "title": qsTrc("dorico", "Page"), "mode": "command://notation/view/mode-page" },
                { "title": qsTrc("dorico", "Galley"), "mode": "command://notation/view/mode-continuous" },
                { "title": qsTrc("dorico", "Fill"), "mode": "command://notation/view/mode-float" }
            ] : []

            ToggleButton {
                id: viewButton

                required property int index
                required property var modelData

                readonly property string currentCode: model.currentViewMode ? model.currentViewMode.code : ""
                active: currentCode === modelData.mode
                         || (modelData.mode.endsWith("continuous") && currentCode.endsWith("single"))
                text: modelData.title

                navigation.panel: navPanel
                navigation.order: 6 + index

                onClicked: model.setCurrentViewMode(modelData.mode)
            }
        }

        StubButton {
            visible: root.shows("arrangement")
            doricoIcon: "svg:book-open"
            toolTipTitle: qsTrc("dorico", "Page arrangement")
        }

        SeparatorLine { orientation: Qt.Vertical; visible: root.shows("zoom") }

        ZoomControl {
            id: zoomControl

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 28

            visible: root.shows("zoom")
            enabled: model.zoomEnabled
            currentZoomPercentage: model.currentZoomPercentage
            minZoomPercentage: model.minZoomPercentage()
            maxZoomPercentage: model.maxZoomPercentage()
            availableZoomList: model.availableZoomList

            navigationPanel: navPanel
            navigationOrderMin: 10

            onChangeZoomPercentageRequested: function(newZoomPercentage) {
                model.currentZoomPercentage = newZoomPercentage
            }

            onChangeZoomRequested: function(zoomId) {
                model.setCurrentZoom(zoomId)
            }

            onZoomInRequested: model.zoomIn()
            onZoomOutRequested: model.zoomOut()
        }

        // MIDI activity light: no MIDI input indicator in MuseScore, so a grey dot.
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10

            visible: root.shows("midi")
            radius: 5
            color: ui.theme.strokeColor

            HoverHandler {
                id: midiHover

                onHoveredChanged: {
                    if (hovered) {
                        ui.tooltip.show(parent, qsTrc("dorico", "MIDI input: not available yet"))
                    } else {
                        ui.tooltip.hide(parent)
                    }
                }
            }
        }
    }
}
