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
import Muse.Dock
import MuseScore.AppShell

import MuseScore.NotationScene
import MuseScore.Playback

//! Base of the five mode pages (spec 50): Dorico top bar, playback and undo/redo toolbars, the score and the
//! status bar. A mode page only adds its own toolbars and panels. Think of it as an abstract base class in Java:
//! the subclass sets objectName/uri and overrides `toolBars` and `panels`.
DockPage {
    id: root

    required property NavigationSection topToolbarKeyNavSec

    // Only WritePage calls pageModel.init(): the model subscribes to the dock toggle requests, and a second
    // initialised copy would handle every request twice.
    property NotationPageModel pageModel: NotationPageModel {}

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

    // Docks every mode page has need a distinct name per page: KDDockWidgets looks docks up by name when it restores
    // a page layout, and a shared name lets one page grab another page's dock. Write keeps MuseScore's names (the
    // View menu toggles them). Must match pageDockName() in doricoshell.h.
    function dockName(name) {
        return root.objectName === "DoricoWrite" ? name : root.objectName + "_" + name
    }

    property var notationView: null

    // Print mode shows the score as a non-editable preview.
    property bool readOnlyView: false

    // Shared by the mode pages' zone panels.
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

    // Which status bar controls this mode shows (see DoricoStatusBar.qml); a mode page overrides it.
    property var statusItems: ["grid", "pitch", "summary", "zoneArrow", "follow", "tools", "viewType", "arrangement", "zoom", "midi"]

    mainToolBars: [
        DockToolBar {
            id: doricoTopBar

            objectName: root.dockName("doricoTopBar")
            title: qsTrc("dorico", "Top bar")

            floatable: false
            closable: false
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Left
            contentBottomPadding: 2

            navigationSection: root.topToolbarKeyNavSec

            DoricoTopBar {
                navigationPanel.section: doricoTopBar.navigationSection
                navigationPanel.order: 2
            }
        },

        DockToolBar {
            id: playbackToolBar

            objectName: root.dockName("playbackToolBar")
            title: qsTrc("appshell", "Playback controls")

            separatorsVisible: false
            alignment: DockToolBarAlignment.Center
            resizable: !floating

            contentBottomPadding: floating ? 8 : 2
            contentTopPadding: floating ? 8 : 0

            navigationSection: root.topToolbarKeyNavSec

            PlaybackToolBar {
                navigationPanelSection: playbackToolBar.navigationSection
                navigationPanelOrder: 3

                floating: playbackToolBar.floating
            }
        },

        DockToolBar {
            id: undoRedoToolBar

            objectName: root.dockName("undoRedoToolBar")
            title: qsTrc("appshell", "Undo/redo")

            floatable: false
            closable: false
            resizable: false
            separatorsVisible: false

            alignment: DockToolBarAlignment.Right
            contentBottomPadding: 2

            navigationSection: root.topToolbarKeyNavSec

            Row {
                spacing: 4

                DoricoTopBarModel {
                    id: helpModel
                }

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

                    onClicked: helpModel.dispatch("command://app/online-handbook")
                }
            }
        }
    ]

    central: NotationView {
        id: notationView
        name: "MainNotationView"

        isNavigatorVisible: root.pageModel.isNavigatorVisible
        isBraillePanelVisible: root.pageModel.isBraillePanelVisible
        isMainView: true
        readOnly: root.readOnlyView
        showNotationTabs: false

        Component.onCompleted: {
            root.notationView = notationView.paintView

            root.setDefaultNavigationControl(notationView.defaultNavigationControl)
        }

        Component.onDestruction: {
            root.setDefaultNavigationControl(null)
        }

        // Popovers (spec 20) open in Write mode only. On the paint view, so the popover works in view pixels.
        Loader {
            parent: notationView.paintView
            anchors.fill: parent
            active: root.uri === "musescore://notation"
            sourceComponent: Popover {
                view: notationView.paintView
            }
        }
    }

    statusBar: DockStatusBar {
        objectName: root.dockName("notationStatusBar")

        navigationSection: content.navigationSection

        DoricoStatusBar {
            id: content

            items: root.statusItems
        }
    }
}
