/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

import Muse.Dock
import Muse.Interactive
import Muse.Ui
import Muse.UiComponents
import MuseScore.AppShell
import MuseScore.Dorico // Dorico fork: mode pages

import "./HomePage"
import "./NotationPage"
import "./NotationReviewPage"
import "./PublishPage"
import "./DevTools"

DockWindow {
    id: root

    objectName: "WindowContent"

    onPageLoaded: {
        console.log("WindowContent::onPageLoaded")
        interactiveProvider.onPageOpened()

        root.updateMainToolBar()
    }

    // Dorico fork: the Dorico pages carry their own top bar, so the Home/Score/Publish bar is hidden there
    function updateMainToolBar() {
        var isDoricoPage = root.currentPageUri === "musescore://notation" || root.currentPageUri.startsWith("musescore://dorico/")
        if (isDoricoPage && mainToolBar.isOpen()) {
            mainToolBar.close()
        } else if (!isDoricoPage && !mainToolBar.isOpen()) {
            mainToolBar.open()
        }
    }

    InteractiveProvider {
        id: interactiveProvider
        topParent: root

        onRequestedDockPage: function(uri, params) {
            root.loadPage(uri, params)
        }
    }

    NavigationSection {
        id: topToolbarKeyNavSec
        name: "TopTool"
        order: 1
    }

    toolBars: [
        DockToolBar {
            id: mainToolBar

            objectName: "mainToolBar"
            title: qsTrc("appshell", "Main toolbar")

            floatable: false
            closable: false

            navigationSection: topToolbarKeyNavSec

            // Dorico fork: Restore default layout reopens this bar and reloads the page without pageLoaded
            onVisibleChanged: if (visible) Qt.callLater(root.updateMainToolBar)

            MainToolBar {
                id: toolBar
                navigation.section: mainToolBar.navigationSection
                navigation.order: 1

                currentUri: root.currentPageUri

                navigation.onActiveChanged: {
                    if (navigation.active) {
                        mainToolBar.forceActiveFocus()
                    }
                }

                onSelected: function(uri) {
                    root.openPage(uri)
                }

                Component.onCompleted: {
                    toolBar.focusOnFirst()
                }
            }
        }
    ]

    pages: [
        HomePage {
            window: root.window
        },

        // Dorico fork: WritePage keeps the uri musescore://notation (was NotationPage)
        WritePage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        SetupPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        EngravePage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        PlayPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        PrintPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        NotationReviewPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        PublishPage {
            topToolbarKeyNavSec: topToolbarKeyNavSec
        },

        DevToolsPage {}
    ]
}
