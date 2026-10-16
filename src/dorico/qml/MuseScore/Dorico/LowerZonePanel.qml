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
import Muse.Dock

import MuseScore.NotationScene
import MuseScore.PropertiesPanel
import MuseScore.Playback

//! Dorico's lower zone (spec 54): ONE dock panel whose content is chosen by a row of tab buttons.
//! Native dock tabs were rejected: the dock library cannot report or set the current tab, so a closed zone
//! could not come back on the tab the user left. The tab is kept in IDoricoModeState (LowerZoneModel), which
//! also lets DoricoActionsController redirect MuseScore's own panel toggles (F3, F8, View menu) to a tab.
//! `tabs` lists the tab ids this mode offers, so Engrave/Play can reuse the panel with fewer tabs.
DockPanel {
    id: root

    objectName: "lowerZonePanel"
    // Fixed: DockBase hands the title to the dock title bar only once, at creation; the selector row names the tabs.
    title: qsTrc("dorico", "Lower zone")

    property var tabs: ["properties", "keyboard", "fretboard", "drumPads", "mixer", "keyEditor", "timeline"]
    property var notationView: null

    // NOTE: DockPanel's default property is ONE lazily loaded Component (contentComponent) with its own id scope:
    // the body below may contain exactly one object (the content Item), and anything the root-level
    // bindings/functions use must live on the root, not inside that Item.
    property LowerZoneModel zoneModel: LowerZoneModel {
        Component.onCompleted: load()
    }
    property var loadersRepeater: null

    readonly property string currentTab: root.tabs.indexOf(root.zoneModel.currentTab) !== -1 ? root.zoneModel.currentTab : root.tabs[0]

    function tabTitle(id) {
        switch (id) {
        case "properties": return qsTrc("dorico", "Properties")
        case "keyboard": return qsTrc("dorico", "Keyboard")
        case "fretboard": return qsTrc("dorico", "Fretboard")
        case "drumPads": return qsTrc("dorico", "Drum Pads")
        case "mixer": return qsTrc("dorico", "Mixer")
        case "keyEditor": return qsTrc("dorico", "Key Editor")
        case "timeline": return qsTrc("dorico", "Timeline")
        }
        return id
    }

    // Applies the current tab's title-bar extras (mixer/percussion toolbars, keyboard context menu) and size to the dock.
    function syncChrome() {
        const loader = root.loadersRepeater?.itemAt(root.tabs.indexOf(root.currentTab))
        if (loader) {
            loader.sync()
        }
    }

    height: 300
    minimumHeight: 100
    maximumHeight: 520
    minimumWidth: 10
    maximumWidth: 7500

    location: Location.Bottom

    //! NOTE: hidden by default
    visible: false

    onPanelShown: root.syncChrome()
    onCurrentTabChanged: root.syncChrome()

    Item {
        id: content

        Component.onCompleted: {
            root.loadersRepeater = tabsRepeater
            root.syncChrome()
        }
        Component.onDestruction: root.loadersRepeater = null

        NavigationPanel {
            name: "LowerZoneTabs"
            section: root.navigationSection
            order: root.contentNavigationPanelOrderStart
            direction: NavigationPanel.Horizontal
            enabled: content.enabled && content.visible

            id: tabsNav
        }

        RowLayout {
            id: selector

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 4

            height: 32
            spacing: 4

            Repeater {
                model: root.tabs

                FlatButton {
                    id: tabButton

                    required property string modelData
                    required property int index

                    Layout.preferredHeight: 28

                    text: root.tabTitle(modelData)
                    transparent: modelData !== root.currentTab
                    accentButton: modelData === root.currentTab

                    navigation.panel: tabsNav
                    navigation.order: tabButton.index

                    onClicked: root.zoneModel.currentTab = tabButton.modelData
                }
            }
        }

        SeparatorLine {
            id: selectorLine
            anchors.top: selector.bottom
        }

        Item {
            id: body

            anchors.top: selectorLine.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            Repeater {
                id: tabsRepeater

                model: root.tabs

                // Loaded on first visit and kept, so the mixer / keyboard keep their state when switching tabs.
                Loader {
                    id: loader

                    required property string modelData

                    readonly property bool isCurrent: modelData === root.currentTab
                    property bool visited: false

                    anchors.fill: parent
                    visible: isCurrent
                    active: isCurrent || visited
                    sourceComponent: content.componentFor(modelData)

                    function sync() {
                        if (!isCurrent || !item) {
                            return
                        }
                        root.contextMenuModel = item.contextMenuModel ?? null
                        root.toolbarComponent = item.toolbarComponent ?? null
                        if (item.resizePanelToContentHeight) {
                            item.resizePanelToContentHeight()
                        }
                    }

                    onIsCurrentChanged: {
                        if (isCurrent) {
                            visited = true
                            sync()
                        }
                    }
                    onLoaded: sync()

                    Connections {
                        target: loader.item
                        ignoreUnknownSignals: true

                        function onResizeRequested(newWidth, newHeight) {
                            if (loader.isCurrent) {
                                root.resize(newWidth, newHeight)
                            }
                        }
                    }
                }
            }
        }

        // NOTE: the tab Components must stay inside this Item. Declared directly in the DockPanel body they would be
        // assigned to its default property (contentComponent), the last one winning, and replace this whole Item.
        function componentFor(id) {
            switch (id) {
            case "properties": return propertiesTab
            case "keyboard": return keyboardTab
            case "drumPads": return drumPadsTab
            case "mixer": return mixerTab
            case "timeline": return timelineTab
            }
            return stubTab
        }

        Component { id: stubTab; DoricoPlaceholder {} }

        Component {
            id: propertiesTab

            PropertiesPanel {
                navigationSection: root.navigationSection
                navigationOrderStart: root.contentNavigationPanelOrderStart + 1
                notationView: root.notationView
            }
        }

        Component {
            id: keyboardTab

            ColumnLayout {
                property alias contextMenuModel: keyboard.contextMenuModel

                spacing: 0

                DoricoToolsModel {
                    id: keyTools

                    // Delete Left needs spec 43; Set Duration (apply the input duration to the selection) has no MuseScore command yet. Listed so the layout matches Dorico.
                    codes: [
                        "command://dorico/space",
                        "command://dorico/delete-left",
                        "command://dorico/set-note-duration",
                        "add-slur"
                    ]

                    Component.onCompleted: keyTools.load()
                }

                RowLayout {
                    Layout.margins: 4
                    spacing: 4

                    Repeater {
                        model: keyTools.items

                        DoricoButton {
                            id: keyButton

                            required property var modelData
                            required property int index

                            readonly property var labels: [
                                qsTrc("dorico", "Advance Caret"),
                                qsTrc("dorico", "Delete Left"),
                                qsTrc("dorico", "Set Duration"),
                                qsTrc("dorico", "Slur")
                            ]

                            readonly property var icons: ["svg:arrow-right", "svg:delete", "\uECA5", "svg:slur"]

                            doricoIcon: icons[index]
                            toolTipTitle: labels[index]
                            navigation.accessible.name: labels[index]
                            available: modelData.enabled

                            onTriggered: keyTools.handleMenuItem(modelData.id)
                        }
                    }
                }

                PianoKeyboardPanel {
                    id: keyboard

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    navigationSection: root.navigationSection
                    contentNavigationPanelOrderStart: root.contentNavigationPanelOrderStart + 1
                }
            }
        }

        Component {
            id: drumPadsTab

            PercussionPanel {
                navigationSection: root.navigationSection
                contentNavigationPanelOrderStart: root.contentNavigationPanelOrderStart + 1
            }
        }

        Component {
            id: mixerTab

            MixerPanel {
                navigationSection: root.navigationSection
                contentNavigationPanelOrderStart: root.contentNavigationPanelOrderStart + 1
            }
        }

        Component {
            id: timelineTab

            Timeline {
                navigationSection: root.navigationSection
                contentNavigationPanelOrderStart: root.contentNavigationPanelOrderStart + 1
            }
        }
    }
}
