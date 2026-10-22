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
import MuseScore.AppShell

import MuseScore.NotationScene
import MuseScore.Palette
import MuseScore.PropertiesPanel
import MuseScore.Playback

//! All five Dorico modes in ONE page (spec 95), so a mode switch is not a page load. Each zone is one dock whose
//! content is a StackLayout with a page per mode (like a Java CardLayout); `mode` picks the page. A zone that the
//! current mode does not have is closed by DoricoActionsController, which also remembers each mode's zone
//! visibility and size. The score view is created once and shared by Setup, Write, Engrave and Print.
DockPage {
    id: root

    objectName: "DoricoShell"
    uri: "musescore://notation"

    required property NavigationSection topToolbarKeyNavSec

    // Same order as mu::dorico::Mode
    readonly property int modeSetup: 0
    readonly property int modeWrite: 1
    readonly property int modeEngrave: 2
    readonly property int modePlay: 3
    readonly property int modePrint: 4

    // The mode, Print's layout list and a command dispatcher for the pages
    property DoricoTopBarModel shellModel: DoricoTopBarModel {
        Component.onCompleted: load()
    }
    readonly property int mode: shellModel.currentMode

    property NotationPageModel pageModel: NotationPageModel {}

    // The Dorico panel shown in Write's right zone (spec 53); the Notations toolbox chooses it.
    property DoricoPanelModel notationsModel: DoricoPanelModel {}

    // Engrave's tool (spec 56): "formatting" (Graphic Editing), "frames" or "slices"
    property string engraveTool: "formatting"

    // Play (spec 57): the Track Inspector publishes its track, the track overview asks for another one.
    property string inspectedPartId: ""
    signal trackPicked(string partId)

    property PrintModel printModel: PrintModel {}

    // One mixer model and one channel-names model for the Mixer tab and the Play inspector (spec 96). The mixer model
    // subscribes to every track, so it is only built once one of them is shown (they set mixerModelWanted).
    property bool mixerModelWanted: false
    readonly property MixerPanelModel mixerModel: root.mixerModelInstantiator.object as MixerPanelModel
    property Instantiator mixerModelInstantiator: Instantiator {
        active: root.mixerModelWanted
        delegate: MixerPanelModel {
            navigationSection: root.keynavBottomPanelSec
        }
    }
    property DoricoMixerModel mixerNames: DoricoMixerModel {
        Component.onCompleted: root.mixerNames.load()
    }

    property var notationView: null

    onInited: {
        Qt.callLater(pageModel.init)
        // Cmd-9 before any toolbox click shows the first panel (the setter builds it)
        if (root.notationsModel.panelId === "") {
            root.notationsModel.panelId = "clefs"
        }
    }

    // Zone sizes per mode (spec 95). sizeMode is the mode whose sizes the zone docks have; the docks' minimum and
    // maximum sizes follow it, not `mode`, so the leaving mode's sizes are saved before the new limits clamp them.
    property int sizeMode: root.mode

    function zoneDock(zone) {
        // mu::dorico::Zone order: left, lower, right
        return zone === 0 ? leftZone : (zone === 1 ? lowerZonePanel : rightZone)
    }

    function saveZoneSizes() {
        root.sizeMode = root.mode // also ends the initial binding: from now on only these handlers set it
        for (let zone = 0; zone < 3; ++zone) {
            const dock = root.zoneDock(zone)
            if (dock.visible) {
                root.shellModel.setZoneSize(root.mode, zone, zone === 1 ? dock.height : dock.width)
            }
        }
    }

    function restoreZoneSizes() {
        for (let zone = 0; zone < 3; ++zone) {
            const dock = root.zoneDock(zone)
            if (dock.visible) {
                const size = root.shellModel.zoneSize(root.mode, zone)
                if (zone === 1) {
                    dock.resize(dock.width, size)
                } else {
                    dock.resize(size, dock.height)
                }
            }
        }
    }

    Connections {
        target: root.shellModel
        function onModeAboutToChange() { root.saveZoneSizes() }
    }

    // When the page is shown (startup, back from the Hub) DoricoActionsController first puts the current mode's
    // zones back (one event-loop turn later); then their sizes. ponytail: fixed delay, a "zones applied" signal if it
    // ever races.
    Timer {
        id: restoreSizesLater
        interval: 100
        onTriggered: root.restoreZoneSizes()
    }

    onVisibleChanged: {
        if (root.visible) {
            restoreSizesLater.restart()
        }
    }

    onModeChanged: {
        // DoricoActionsController has already opened and closed the docks for the new mode (same notification, earlier).
        root.sizeMode = root.mode
        Qt.callLater(root.restoreZoneSizes)
        // Again once the dock library has settled: a resize while a toolbox dock is still opening or closing lands
        // tens of pixels short (measured: 503 asked, 461 got).
        restoreSizesLater.restart()

        if (root.mode === root.modePrint) {
            root.printModel.load()
            // Dorico opens Print on one whole page. DoricoActionsController restores the zoom on the way out.
            Qt.callLater(root.shellModel.dispatch, "command://notation/view/zoom-to-whole-page")
        }
    }

    function showNotationsPanel(panelId) {
        // Clicking the active button closes the zone, like Dorico.
        if (root.notationsModel.panelId === panelId && root.notationsModel.isPanelOpen()) {
            root.notationsModel.setPanelOpen(false)
            return
        }
        root.notationsModel.panelId = panelId
        root.notationsModel.setPanelOpen(true)
    }

    property NavigationSection noteInputKeyNavSec: NavigationSection {
        name: "NoteInputSection"
        order: 2
    }

    property NavigationSection keynavTopPanelSec: NavigationSection {
        name: "NavigationTopPanel"
        enabled: root.visible
        order: 3
    }

    property NavigationSection keynavLeftPanelSec: NavigationSection {
        name: "NavigationLeftPanel"
        enabled: root.visible
        order: 4
    }

    property NavigationSection keynavRightPanelSec: NavigationSection {
        name: "NavigationRightPanel"
        enabled: root.visible
        order: 6
    }

    property NavigationSection keynavBottomPanelSec: NavigationSection {
        name: "NavigationBottomPanel"
        enabled: root.visible
        order: 7
    }

    function navigationPanelSec(location) {
        switch(location) {
        case Location.Top: return keynavTopPanelSec
        case Location.Left: return keynavLeftPanelSec
        case Location.Right: return keynavRightPanelSec
        case Location.Bottom: return keynavBottomPanelSec
        }

        return null
    }

    readonly property int verticalPanelDefaultWidth: 300
    readonly property int panelMinDimension: 10
    readonly property int panelMaxDimension: 7500 //! NOTE: Value found experimentally - see issue #27770

    readonly property var verticalPanelDropDestinations: [
        { "dock": root.centralDock, "dropLocation": Location.Left, "dropDistance": root.verticalPanelDefaultWidth },
        { "dock": root.centralDock, "dropLocation": Location.Right, "dropDistance": root.verticalPanelDefaultWidth }
    ]

    readonly property var horizontalPanelDropDestinations: [
        root.panelTopDropDestination,
        root.panelBottomDropDestination
    ]

    readonly property string rightPanelsGroup: "RIGHT_PANELS"

    // Per mode, in Mode order. Zone sizes: the sizes a mode starts with are in doricoshell.h (zoneDefaultSize).
    readonly property var statusItemsByMode: [
        ["order", "summary", "zoom", "midi"],
        ["grid", "pitch", "summary", "zoneArrow", "follow", "tools", "viewType", "arrangement", "zoom", "midi"],
        ["grid", "pitch", "summary", "zoneArrow", "follow", "tools", "arrangement", "zoom", "midi"], // Engrave: always page view
        ["pitch", "summary", "zoneArrow", "follow", "zoom", "midi"],
        ["summary", "zoomFit", "zoom"]
    ]
    readonly property var leftZoneMinWidth: [300, 88, 200, 200, 300]
    readonly property var leftZoneMaxWidth: [600, 300, 600, 600, 600]
    readonly property var rightZoneMinWidth: [300, 200, 200, 200, 300]
    readonly property var lowerZoneTabs: [
        ["flows"],
        ["properties", "keyboard", "fretboard", "drumPads", "mixer", "keyEditor", "timeline"],
        ["properties", "mixer"],
        ["mixer", "keyEditor"],
        []
    ]

    // The contents of the zones and toolboxes (see ZoneHolder): built once, shown by their dock while it is open.
    Item {
        id: parking

        visible: false

        Item {
            id: leftToolboxContent

            implicitWidth: root.mode === root.modeEngrave ? engraveTools.implicitWidth : notesTools.implicitWidth
            implicitHeight: root.mode === root.modeEngrave ? engraveTools.implicitHeight : notesTools.implicitHeight

            // Toolbox strips are one shade lighter than the panels (spec 101, P1)
            Rectangle {
                anchors.fill: parent
                color: ui.theme.backgroundSecondaryColor
            }

            NotesToolbox {
                id: notesTools

                visible: root.mode !== root.modeEngrave
                navigationPanel.section: leftToolbox.navigationSection
                navigationPanel.order: 1
            }

            EngraveToolbox {
                id: engraveTools

                visible: root.mode === root.modeEngrave
                tool: root.engraveTool
                navigationSection: leftToolbox.navigationSection
                onToolSelected: function(toolId) { root.engraveTool = toolId }
            }
        }

        NotationsToolbox {
            id: notationsToolboxContent

            availableHeight: notationsToolbox.height
            navigationPanel.section: notationsToolbox.navigationSection
            navigationPanel.order: 2

            Rectangle {
                anchors.fill: parent
                z: -1
                color: ui.theme.backgroundSecondaryColor
            }

            // Highlight only while the zone shows it (a DockPanel's visible follows its open state).
            currentPanelId: rightZone.visible && root.mode === root.modeWrite ? root.notationsModel.panelId : ""

            onPanelClicked: function(panelId) {
                root.showNotationsPanel(panelId)
            }
        }

        StackLayout {
            id: leftZoneContent

            currentIndex: root.mode

            LazyPane {
                sourceComponent: SetupPlayersPanel {
                    navigationSection: leftZone.navigationSection
                    navigationOrderStart: leftZone.contentNavigationPanelOrderStart
                }
            }

            LazyPane {
                sourceComponent: NotesPanel {
                    navigationSection: leftZone.navigationSection
                    navigationOrderStart: leftZone.contentNavigationPanelOrderStart
                }
            }

            LazyPane {
                sourceComponent: EngraveFormattingPanel {
                    tool: root.engraveTool
                    navigationSection: leftZone.navigationSection
                    navigationOrderStart: leftZone.contentNavigationPanelOrderStart
                }
            }

            LazyPane {
                sourceComponent: PlayInspectorPanel {
                    navigationSection: leftZone.navigationSection
                    navigationOrderStart: leftZone.contentNavigationPanelOrderStart
                    trackSource: root
                    mixerModel: root.mixerModel
                    mixerNames: root.mixerNames
                    Component.onCompleted: root.mixerModelWanted = true
                    onPartInspected: function(partId) { root.inspectedPartId = partId }
                }
            }

            LazyPane {
                sourceComponent: PrintLayoutsPanel {
                    printModel: root.printModel
                    layouts: root.shellModel
                    navigationSection: leftZone.navigationSection
                    navigationOrderStart: leftZone.contentNavigationPanelOrderStart
                }
            }
        }

        StackLayout {
            id: rightZoneContent

            currentIndex: root.mode

            LazyPane {
                sourceComponent: ColumnLayout {
                    spacing: 0

                    ZoneHeader {
                        Layout.fillWidth: true
                        title: qsTrc("dorico", "Layouts")
                    }

                    LayoutsPanel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        navigationSection: rightZone.navigationSection
                        navigationOrderStart: rightZone.contentNavigationPanelOrderStart
                    }
                }
            }

            LazyPane {
                sourceComponent: NotationsPanel {
                    panelModel: root.notationsModel
                    navigationSection: rightZone.navigationSection
                    navigationOrderStart: rightZone.contentNavigationPanelOrderStart
                }
            }

            LazyPane {
                sourceComponent: EngravePagesPanel {}
            }

            Item {} // Play has no right zone

            LazyPane {
                sourceComponent: PrintOptionsPanel {
                    printModel: root.printModel
                    layouts: root.shellModel
                    navigationSection: rightZone.navigationSection
                    navigationOrderStart: rightZone.contentNavigationPanelOrderStart
                }
            }
        }
    }

    mainToolBars: [
        DockToolBar {
            id: doricoTopBar

            objectName: "doricoTopBar"
            thickness: 37
            title: qsTrc("dorico", "Top bar")

            floatable: false
            closable: false
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Left
            contentBottomPadding: 0

            navigationSection: root.topToolbarKeyNavSec

            // Dorico's toolbar is one shade lighter than the panels (spec 101, T2); DockFrame paints the panel colour
            Rectangle {
                color: ui.theme.backgroundSecondaryColor
                implicitWidth: topBarContent.implicitWidth
                implicitHeight: 37

                DoricoTopBar {
                    id: topBarContent

                    anchors.verticalCenter: parent.verticalCenter

                    navigationPanel.section: doricoTopBar.navigationSection
                    navigationPanel.order: 2
                }
            }
        },

        DockToolBar {
            id: playbackToolBar

            objectName: "playbackToolBar"
            thickness: 37
            title: qsTrc("appshell", "Playback controls")

            separatorsVisible: false
            alignment: DockToolBarAlignment.Center
            resizable: !floating

            contentBottomPadding: floating ? 8 : 0
            contentTopPadding: floating ? 8 : 0

            navigationSection: root.topToolbarKeyNavSec

            // Dorico's transport group (spec 101, T5): video, mixer, audio engine, the playback controls, record and
            // volume, in one dark pill. Video, audio engine, record and volume have no MuseScore command yet.
            Rectangle {
                id: transport

                readonly property bool docked: !playbackToolBar.floating
                readonly property string notAvailable: qsTrc("dorico", "Not available yet")

                color: docked ? ui.theme.backgroundSecondaryColor : "transparent"
                implicitWidth: transportRow.implicitWidth + (docked ? 20 : 0)
                implicitHeight: docked ? 37 : transportRow.implicitHeight

                Rectangle {
                    anchors.centerIn: parent
                    width: transportRow.implicitWidth + 12
                    height: 31
                    radius: 4
                    color: ui.theme.backgroundPrimaryColor
                    visible: transport.docked
                }

                Row {
                    id: transportRow

                    anchors.centerIn: parent
                    spacing: 4

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: transport.docked
                        enabled: false
                        doricoIcon: "svg:video"
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Video")
                        toolTipDescription: transport.notAvailable
                    }

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: transport.docked
                        doricoIcon: "svg:sliders-vertical"
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Mixer")
                        onTriggered: root.shellModel.dispatch("command://app/dock/toggle-mixer")
                    }

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: transport.docked
                        enabled: false
                        doricoIcon: "svg:power"
                        iconColor: "#40ACFF"
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Audio engine")
                        toolTipDescription: transport.notAvailable
                    }

                    PlaybackToolBar {
                        id: playbackContent

                        anchors.verticalCenter: parent.verticalCenter

                        navigationPanelSection: playbackToolBar.navigationSection
                        navigationPanelOrder: 3

                        floating: playbackToolBar.floating
                    }

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: transport.docked
                        enabled: false
                        doricoIcon: "svg:circle"
                        iconColor: "#E5484D"
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Record")
                        toolTipDescription: transport.notAvailable
                    }

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: transport.docked
                        enabled: false
                        doricoIcon: "svg:volume-2"
                        transparent: true
                        toolTipTitle: qsTrc("dorico", "Master volume")
                        toolTipDescription: transport.notAvailable
                    }
                }
            }
        },

        DockToolBar {
            id: undoRedoToolBar

            objectName: "undoRedoToolBar"
            thickness: 37
            title: qsTrc("appshell", "Undo/redo")

            floatable: false
            closable: false
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Right
            contentBottomPadding: 0

            navigationSection: root.topToolbarKeyNavSec

            Rectangle {
                color: ui.theme.backgroundSecondaryColor
                implicitWidth: undoRedoRow.implicitWidth
                implicitHeight: 37

                Row {
                    id: undoRedoRow

                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    UndoRedoToolBar {
                        anchors.verticalCenter: parent.verticalCenter

                        navigationPanel.section: undoRedoToolBar.navigationSection
                        navigationPanel.order: 5
                    }

                    DoricoButton {
                        anchors.verticalCenter: parent.verticalCenter

                        doricoIcon: "svg:circle-help"
                        toolTipTitle: qsTrc("dorico", "Help")

                        navigation.panel: NavigationPanel {
                            name: "DoricoHelp"
                            section: undoRedoToolBar.navigationSection
                            order: 6
                            accessible.name: qsTrc("dorico", "Help")
                        }

                        onClicked: root.shellModel.dispatch("command://app/online-handbook")
                    }
                }
            }
        }
    ]

    toolBars: [
        // Write: Notes toolbox (spec 52). Engrave: Engrave toolbox (spec 56). Closed in the other modes.
        DockToolBar {
            id: leftToolbox

            objectName: "doricoLeftToolbox"
            title: qsTrc("dorico", "Left toolbox")

            // A non-floatable toolbar keeps the orientation it is given (DockToolBarView::canChangeOrientation).
            location: Location.Left
            orientation: Qt.Vertical
            // Restore default layout forces every toolbar horizontal (DockToolBarView::resetToDefault)
            onOrientationChanged: function(o) { if (o !== Qt.Vertical) orientation = Qt.Vertical }
            floatable: false
            closable: false
            resizable: false
            thickness: 48

            navigationSection: root.noteInputKeyNavSec

            ZoneHolder { item: leftToolboxContent }
        },

        // Write only: Notations toolbox (spec 53)
        DockToolBar {
            id: notationsToolbox

            objectName: "doricoRightToolbox"
            title: qsTrc("dorico", "Notations toolbox")

            location: Location.Right
            orientation: Qt.Vertical
            // Restore default layout forces every toolbar horizontal (DockToolBarView::resetToDefault)
            onOrientationChanged: function(o) { if (o !== Qt.Vertical) orientation = Qt.Vertical }
            floatable: false
            closable: false
            resizable: false
            thickness: 48

            navigationSection: root.noteInputKeyNavSec

            ZoneHolder { item: notationsToolboxContent }
        }
    ]

    panels: [
        DockPanel {
            id: leftZone

            objectName: "doricoLeftZone"
            title: qsTrc("dorico", "Left zone")

            navigationSection: root.navigationPanelSec(leftZone.location)

            width: 132
            minimumWidth: root.leftZoneMinWidth[root.sizeMode]
            maximumWidth: root.leftZoneMaxWidth[root.sizeMode]
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left

            // No title bar, as in Dorico (a panel that can neither float nor close gets none); Setup and Print draw
            // their own (ZoneHeader).
            floatable: false
            closable: false
            dropDestinations: root.verticalPanelDropDestinations

            ZoneHolder { item: leftZoneContent }
        },

        DockPanel {
            id: rightZone

            objectName: "doricoRightZone"
            title: qsTrc("dorico", "Right zone")

            navigationSection: root.navigationPanelSec(rightZone.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.rightZoneMinWidth[root.sizeMode]
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            // No title bar (see the left zone)
            floatable: false
            closable: false

            //! NOTE: closed in Write until a Notations toolbox button opens it
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            ZoneHolder { item: rightZoneContent }
        },

        // MuseScore's own right-side panels, Write mode only (the View menu toggles them by these names)
        DockPanel {
            id: palettesPanel

            objectName: root.pageModel.palettesPanelName()
            title: qsTrc("appshell", "Palettes")

            navigationSection: root.navigationPanelSec(palettesPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default; the Notations panel is the Dorico way in
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            PalettesPanel {
                navigationSection: palettesPanel.navigationSection
                navigationOrderStart: palettesPanel.contentNavigationPanelOrderStart

                Component.onCompleted: {
                    palettesPanel.contextMenuModel = contextMenuModel
                }
            }
        },

        DockPanel {
            id: selectionFilterPanel

            objectName: root.pageModel.selectionFiltersPanelName()
            title: qsTrc("appshell", "Selection filter")

            navigationSection: root.navigationPanelSec(selectionFilterPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            SelectionFilterPanel {
                navigationSection: selectionFilterPanel.navigationSection
                navigationOrderStart: selectionFilterPanel.contentNavigationPanelOrderStart
            }
        },

        DockPanel {
            id: undoHistoryPanel

            objectName: root.pageModel.undoHistoryPanelName()
            title: qsTrc("notation", "History")

            navigationSection: root.navigationPanelSec(undoHistoryPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth

            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            groupName: root.rightPanelsGroup
            location: Location.Right

            //! NOTE: hidden by default
            visible: false

            dropDestinations: root.verticalPanelDropDestinations

            UndoHistoryPanel {
                navigationSection: undoHistoryPanel.navigationSection
                navigationOrderStart: undoHistoryPanel.contentNavigationPanelOrderStart
            }
        },

        // Write, Engrave and Play: the tabbed lower zone (spec 54). Setup: the Flows panel. Print: closed.
        LowerZonePanel {
            id: lowerZonePanel

            tabs: root.lowerZoneTabs[root.mode]
            maximumHeight: root.sizeMode === root.modeSetup ? 300 : 520
            notationView: root.notationView
            mixerHost: root

            navigationSection: root.navigationPanelSec(lowerZonePanel.location)

            dropDestinations: root.horizontalPanelDropDestinations
        }
    ]

    // Setup, Write, Engrave and Print share one score view; Play shows the track overview instead (spec 57).
    central: Item {
        readonly property var navigationSection: notationView.navigationSection

        StackLayout {
            anchors.fill: parent
            currentIndex: root.mode === root.modePlay ? 1 : 0

            NotationView {
                id: notationView
                name: "MainNotationView"

                isNavigatorVisible: root.pageModel.isNavigatorVisible
                isBraillePanelVisible: root.pageModel.isBraillePanelVisible
                isMainView: true
                // Print mode shows the score as a non-editable preview.
                readOnly: root.mode === root.modePrint
                showNotationTabs: false
                // Right-click shows Dorico's menu (spec 98)
                contextMenuModelOverride: DoricoContextMenuModel {}

                Component.onCompleted: {
                    root.notationView = notationView.paintView

                    root.setDefaultNavigationControl(notationView.defaultNavigationControl)
                }

                Component.onDestruction: {
                    root.setDefaultNavigationControl(null)
                }

                // Dorico shows Print mode's preview on grey (measured #AAAAAE), without touching the stored setting.
                // Each mode allows its own mouse edits (spec 97): none in Setup, pitch drags in Write, offsets in Engrave.
                Binding {
                    target: notationView.paintView
                    property: "interactionCaps"
                    value: root.shellModel.interactionCaps
                }

                Binding {
                    target: notationView.paintView
                    property: "backgroundOverride"
                    value: root.mode === root.modePrint ? "#AAAAAE" : "transparent" // transparent: the gradient below shows
                }

                // The music area behind the pages (spec 101, M1): Dorico's navy gradient (sampled from Dorico Pro 6),
                // a flat slate in Engrave. A negative z draws it under the paint view's own painting.
                Rectangle {
                    parent: notationView.paintView
                    anchors.fill: parent
                    z: -1
                    visible: root.mode !== root.modePrint
                    color: "#485E6E"
                    gradient: root.mode === root.modeEngrave ? null : writeGradient

                    Gradient {
                        id: writeGradient

                        GradientStop { position: 0.0; color: "#09203F" }
                        GradientStop { position: 0.6; color: "#1D3756" }
                        GradientStop { position: 1.0; color: "#3A5977" }
                    }
                }

                // Zone collapse tabs at the window edges (spec 101, S3): the left and right zones, like Ctrl-7 / Ctrl-9
                Repeater {
                    model: [
                        { "icon": "svg:chevron-left", "command": "command://dorico/toggle-zone-left", "left": true },
                        { "icon": "svg:chevron-right", "command": "command://dorico/toggle-zone-right", "left": false }
                    ]

                    DoricoButton {
                        required property var modelData

                        parent: notationView.paintView
                        z: 10
                        anchors.left: modelData.left ? parent.left : undefined
                        anchors.right: modelData.left ? undefined : parent.right
                        anchors.verticalCenter: parent.verticalCenter

                        visible: root.mode !== root.modePrint
                        doricoIcon: modelData.icon
                        iconBoxSize: 12
                        toolTipTitle: modelData.left ? qsTrc("dorico", "Show/hide left zone") : qsTrc("dorico", "Show/hide right zone")

                        onTriggered: root.shellModel.dispatch(modelData.command)
                    }
                }

                // Popovers (spec 20) open in Write mode only. On the paint view, so the popover works in view pixels.
                Loader {
                    parent: notationView.paintView
                    anchors.fill: parent
                    active: root.mode === root.modeWrite
                    sourceComponent: Popover {
                        view: notationView.paintView
                    }
                }

                // The jump bar (spec 46): same host as the popovers.
                Loader {
                    parent: notationView.paintView
                    anchors.fill: parent
                    active: root.mode === root.modeWrite
                    sourceComponent: JumpBar {
                        view: notationView.paintView
                    }
                }
            }

            LazyPane {
                sourceComponent: TrackOverview {
                    navigationSection: root.keynavTopPanelSec
                    contentNavigationPanelOrderStart: 1

                    selectedPartId: root.inspectedPartId
                    onTrackClicked: function(partId) { root.trackPicked(partId) }
                }
            }
        }
    }

    statusBar: DockStatusBar {
        objectName: "notationStatusBar"

        navigationSection: content.navigationSection

        DoricoStatusBar {
            id: content

            items: root.statusItemsByMode[root.mode] ?? []
        }
    }
}
