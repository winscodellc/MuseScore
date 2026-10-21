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
//! `tabs` lists the tab ids the current mode offers (spec 95: one lower zone for all modes). The loaders are made
//! for `allTabs`, so a mode switch only hides tab buttons and keeps the mixer etc. alive. Setup's only tab is "flows",
//! shown without the tab row.
DockPanel {
    id: root

    objectName: "lowerZonePanel"
    // Fixed: DockBase hands the title to the dock title bar only once, at creation; the selector row names the tabs.
    title: qsTrc("dorico", "Lower zone")

    readonly property var allTabs: ["properties", "keyboard", "fretboard", "drumPads", "mixer", "keyEditor", "timeline", "flows"]
    property var tabs: ["properties", "keyboard", "fretboard", "drumPads", "mixer", "keyEditor", "timeline"]
    property var notationView: null
    // As Dorico (all modes): icon tabs, with the current tab's toolbar (`headerComponent`) beside them, and Dorico's
    // own mixer (DoricoMixer) instead of MuseScore's. Off restores text tabs / MuseScore's mixer.
    property bool iconTabs: true
    property bool doricoMixer: true

    // NOTE: DockPanel's default property is ONE lazily loaded Component (contentComponent) with its own id scope:
    // the body below may contain exactly one object (the ZoneHolder), and the content lives on the root (zoneBody).
    property LowerZoneModel zoneModel: LowerZoneModel {
        Component.onCompleted: load()
    }
    property var loadersRepeater: null

    readonly property string currentTab: root.tabs.indexOf(root.zoneModel.currentTab) !== -1 ? root.zoneModel.currentTab : (root.tabs[0] ?? "")

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

    function tabIcon(id) {
        switch (id) {
        case "properties": return "svg:list"
        case "keyboard": return "svg:keyboard-music"
        case "fretboard": return "svg:fretboard"
        case "drumPads": return "svg:layout-grid"
        case "mixer": return "svg:sliders-vertical"
        case "keyEditor": return "svg:list-music"
        case "timeline": return "svg:move-horizontal"
        }
        return IconCode.NONE
    }

    // Applies the current tab's title-bar extras (mixer/percussion toolbars, keyboard context menu) and size to the dock.
    function syncChrome() {
        const loader = root.loadersRepeater?.itemAt(root.allTabs.indexOf(root.currentTab))
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

    // No title bar, as in Dorico: a panel that can neither float nor close gets none (DockFrameModel)
    floatable: false
    closable: false

    //! NOTE: hidden by default
    visible: false

    onPanelShown: root.syncChrome()
    onCurrentTabChanged: root.syncChrome()

    // The dock only borrows the body (ZoneHolder, spec 95): closing the lower zone (Cmd-8, or a mode without one)
    // keeps the tabs that were built, e.g. the mixer.
    ZoneHolder {
        item: root.zoneBody
    }

    property Item parkingItem: Item {
        visible: false
    }

    property Item zoneBody: Item {
        id: content

        parent: root.parkingItem

        Component.onCompleted: {
            root.loadersRepeater = tabsRepeater
            root.syncChrome()
        }

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

            height: visible ? 32 : 0
            visible: root.tabs.length > 1
            spacing: 4

            Repeater {
                model: root.allTabs

                DoricoButton {
                    id: tabButton

                    required property string modelData
                    required property int index

                    Layout.preferredHeight: 28
                    visible: root.tabs.indexOf(modelData) !== -1

                    doricoIcon: root.iconTabs ? root.tabIcon(modelData) : IconCode.NONE
                    iconBoxSize: 18
                    text: root.iconTabs ? "" : root.tabTitle(modelData)
                    toolTipTitle: root.iconTabs ? root.tabTitle(modelData) : ""
                    transparent: modelData !== root.currentTab
                    accentButton: modelData === root.currentTab

                    navigation.panel: tabsNav
                    navigation.order: tabButton.index

                    onTriggered: root.zoneModel.currentTab = tabButton.modelData
                }
            }

            // The current tab's toolbar, if it has one (DoricoMixer's Mixer toolbar)
            Loader {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.preferredHeight: 28

                sourceComponent: {
                    const loader = tabsRepeater.count > 0 ? tabsRepeater.itemAt(root.allTabs.indexOf(root.currentTab)) : null
                    return loader && loader.item ? (loader.item.headerComponent ?? null) : null
                }
            }
        }

        SeparatorLine {
            id: selectorLine
            anchors.top: selector.bottom
            visible: selector.visible
        }

        Item {
            id: body

            anchors.top: selector.visible ? selectorLine.bottom : parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            Repeater {
                id: tabsRepeater

                model: root.allTabs

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
            case "mixer": return root.doricoMixer ? doricoMixerTab : mixerTab
            case "timeline": return timelineTab
            case "flows": return flowsTab
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
            id: doricoMixerTab

            DoricoMixer {
                navigationSection: root.navigationSection
                contentNavigationPanelOrderStart: root.contentNavigationPanelOrderStart + 1
            }
        }

        Component {
            id: flowsTab

            SetupFlowsPanel {
                navigationSection: root.navigationSection
                navigationOrderStart: root.contentNavigationPanelOrderStart + 1
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
