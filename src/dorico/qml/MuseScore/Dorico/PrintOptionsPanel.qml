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

//! Print mode's Print Options panel (right zone, spec 58), with the Print button at its bottom right.
Item {
    id: root

    property PrintModel printModel: null
    property DoricoTopBarModel layouts: null
    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1
    NavigationPanel {
        id: optionsNav
        name: "DoricoPrintOptions"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    StyledFlickable {
        anchors.fill: parent
        anchors.bottomMargin: printButton.height + 16
        contentHeight: options.implicitHeight + 8

        // Dorico's Print Options: collapsible sections (Duplex Printing and Annotations start closed)
        PrintOptions {
            id: options

            width: parent.width
            printModel: root.printModel
            nav: optionsNav
            layoutInfo: root.layouts.layoutInfos[root.layouts.currentLayoutIndex] ?? ""
            onPageSetupRequested: root.layouts.dispatch("command://notation/open-page-settings")
        }
    }

    FlatButton {
        id: printButton

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 8

        accentButton: true
        enabled: root.printModel.hasChecked && root.printModel.pagesValid
        text: root.printModel.buttonLabel

        navigation.panel: optionsNav
        navigation.order: 60

        onClicked: root.printModel.run()
    }
}
