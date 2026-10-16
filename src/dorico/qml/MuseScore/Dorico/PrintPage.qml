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

//! Print mode (spec 58): Layouts panel (left, which layouts to print, System Dialogs footer), Print Options panel
//! (right, Print button at its bottom right), and a read-only preview of the current layout in the centre. No lower
//! zone; the layout selector is disabled in the top bar; the status bar has Page Width / Whole Page.
DoricoPage {
    id: root

    objectName: "DoricoPrint"
    uri: "musescore://dorico/print"

    statusItems: ["summary", "zoomFit", "zoom"]
    readOnlyView: true

    // Shared by both panels; the panels' content components see the page's ids.
    property PrintModel printModel: PrintModel {}
    property DoricoTopBarModel layouts: DoricoTopBarModel {
        Component.onCompleted: load()
    }

    // 0 = printer, then the graphics formats MuseScore can export.
    property int destination: 0
    readonly property var destinations: [
        { "text": qsTrc("dorico", "Printer"), "value": "" },
        { "text": qsTrc("dorico", "PDF"), "value": "pdf" },
        { "text": qsTrc("dorico", "PNG"), "value": "png" },
        { "text": qsTrc("dorico", "SVG"), "value": "svg" }
    ]
    readonly property string suffix: root.destinations[root.destination].value
    // The graphics format remembered while Printer is chosen (index into destinations, 1..3).
    property int graphicsFormat: 1
    property bool filePerLayout: true

    onVisibleChanged: {
        if (visible) {
            root.printModel.load()
        }
    }

    panels: [
        DockPanel {
            id: printLayoutsPanel

            objectName: "printLayoutsPanel"
            title: qsTrc("dorico", "Layouts")

            navigationSection: root.navigationPanelSec(printLayoutsPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Left
            dropDestinations: root.verticalPanelDropDestinations

            Item {
                NavigationPanel {
                    id: layoutsNav
                    name: "DoricoPrintLayouts"
                    section: printLayoutsPanel.navigationSection
                    order: printLayoutsPanel.contentNavigationPanelOrderStart
                    direction: NavigationPanel.Vertical
                }

                StyledListView {
                    id: layoutList

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: systemDialogs.top
                    anchors.bottomMargin: 8

                    spacing: 4
                    model: root.printModel

                    // Dorico-style layout rows: icon and name; the layouts to print are filled blue. Click toggles.
                    delegate: Rectangle {
                        id: card

                        required property int index
                        required property string title
                        required property string info
                        required property bool checked

                        width: ListView.view.width - 16
                        x: 8
                        height: 26
                        radius: 3
                        color: card.checked ? "#45A6F5" : (ui.theme.isDark ? "#3D4A58" : "#DDE2E7")

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6

                            DoricoButton {
                                anchors.verticalCenter: parent.verticalCenter
                                doricoIcon: card.index === 0 ? "svg:file-music" : "svg:file-user"
                                iconBoxSize: 14
                                transparent: true
                                iconColor: card.checked ? "#0C0C0C" : ui.theme.fontPrimaryColor
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: card.title
                                font: ui.theme.bodyFont
                                color: card.checked ? "#0C0C0C" : ui.theme.fontPrimaryColor
                            }
                        }

                        MouseArea {
                            id: cardMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.printModel.setChecked(card.index, !card.checked)
                            onContainsMouseChanged: {
                                if (containsMouse) {
                                    ui.tooltip.show(card, card.info)
                                } else {
                                    ui.tooltip.hide(card)
                                }
                            }
                        }

                        NavigationControl {
                            name: card.title
                            panel: layoutsNav
                            order: card.index
                            accessible.role: MUAccessible.CheckBox
                            accessible.name: card.title
                            accessible.checked: card.checked
                            onTriggered: root.printModel.setChecked(card.index, !card.checked)
                        }
                    }
                }

                // Dorico's "System Dialogs" footer: the page settings dialog and the system print dialog.
                Column {
                    id: systemDialogs

                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    spacing: 6

                    StyledTextLabel { text: qsTrc("dorico", "System Dialogs:") }

                    Row {
                        spacing: 8

                        FlatButton {
                            text: qsTrc("dorico", "Page Setup...")
                            navigation.panel: layoutsNav
                            navigation.order: 1000
                            onClicked: root.layouts.dispatch("command://notation/open-page-settings")
                        }

                        FlatButton {
                            text: qsTrc("dorico", "Print...")
                            enabled: root.printModel.hasChecked
                            navigation.panel: layoutsNav
                            navigation.order: 1001
                            onClicked: root.printModel.print()
                        }
                    }
                }
            }
        },

        DockPanel {
            id: printOptionsPanel

            objectName: "printOptionsPanel"
            title: qsTrc("dorico", "Print Options")

            navigationSection: root.navigationPanelSec(printOptionsPanel.location)

            width: root.verticalPanelDefaultWidth
            minimumWidth: root.verticalPanelDefaultWidth
            maximumWidth: root.verticalPanelDefaultWidth * 2
            minimumHeight: root.panelMinDimension
            maximumHeight: root.panelMaxDimension

            location: Location.Right
            dropDestinations: root.verticalPanelDropDestinations

            // No title bar: Dorico's right zone starts with the Destination section
            floatable: false
            closable: false

            Item {
                NavigationPanel {
                    id: optionsNav
                    name: "DoricoPrintOptions"
                    section: printOptionsPanel.navigationSection
                    order: printOptionsPanel.contentNavigationPanelOrderStart
                    direction: NavigationPanel.Vertical
                }

                StyledFlickable {
                    anchors.fill: parent
                    anchors.bottomMargin: printButton.height + 16
                    contentHeight: options.implicitHeight + 8

                    // Dorico's Print Options: collapsible sections (Duplex Printing and Annotations start closed)
                    Column {
                        id: options

                        width: parent.width
                        spacing: 2

                        InspectorSection {
                            width: options.width
                            title: qsTrc("dorico", "Destination")

                            // Printer | Graphics, then the graphics format (MuseScore exports PDF, PNG and SVG).
                            RowLayout {
                                width: parent.width
                                spacing: 0

                                FlatButton {
                                    Layout.fillWidth: true
                                    text: qsTrc("dorico", "Printer")
                                    accentButton: root.suffix === ""
                                    transparent: root.suffix !== ""
                                    navigation.panel: optionsNav
                                    navigation.order: 1
                                    onClicked: root.destination = 0
                                }

                                FlatButton {
                                    Layout.fillWidth: true
                                    text: qsTrc("dorico", "Graphics")
                                    accentButton: root.suffix !== ""
                                    transparent: root.suffix === ""
                                    navigation.panel: optionsNav
                                    navigation.order: 2
                                    onClicked: root.destination = root.graphicsFormat
                                }
                            }

                            StyledDropdown {
                                width: parent.width
                                visible: root.suffix !== ""
                                model: root.destinations.slice(1)
                                currentIndex: root.graphicsFormat - 1
                                navigation.panel: optionsNav
                                navigation.order: 3
                                onActivated: function(index, value) {
                                    root.graphicsFormat = index + 1
                                    root.destination = root.graphicsFormat
                                }
                            }

                            StyledTextLabel {
                                width: parent.width
                                visible: root.suffix === ""
                                text: qsTrc("dorico", "Copies and printer are chosen in the system print dialog, once per layout.")
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignLeft
                                opacity: 0.7
                            }
                        }

                        InspectorSection {
                            width: options.width
                            title: qsTrc("dorico", "Job Type")

                            StyledTextLabel {
                                width: parent.width
                                text: qsTrc("dorico", "Print range:")
                                horizontalAlignment: Text.AlignLeft
                            }

                            // Only All is real: MuseScore prints or exports whole layouts, and has one flow.
                            RowLayout {
                                width: parent.width
                                spacing: 0

                                FlatButton {
                                    Layout.fillWidth: true
                                    text: qsTrc("dorico", "All")
                                    accentButton: true
                                    navigation.panel: optionsNav
                                    navigation.order: 4
                                }

                                DoricoButton {
                                    Layout.fillWidth: true
                                    text: qsTrc("dorico", "Pages")
                                    available: false
                                    navigation.panel: optionsNav
                                    navigation.order: 5
                                }

                                DoricoButton {
                                    Layout.fillWidth: true
                                    text: qsTrc("dorico", "Flows")
                                    available: false
                                    navigation.panel: optionsNav
                                    navigation.order: 6
                                }
                            }

                            CheckBox {
                                width: parent.width
                                visible: root.suffix === "pdf"
                                text: qsTrc("dorico", "One file per layout")
                                checked: root.filePerLayout
                                navigation.panel: optionsNav
                                navigation.order: 7
                                onClicked: root.filePerLayout = !root.filePerLayout
                            }

                            StyledTextLabel {
                                width: parent.width
                                visible: root.suffix === "png" || root.suffix === "svg"
                                text: qsTrc("dorico", "One file per page")
                                horizontalAlignment: Text.AlignLeft
                                opacity: 0.7
                            }
                        }

                        InspectorSection {
                            width: options.width
                            title: qsTrc("dorico", "Page Setup")

                            StyledTextLabel {
                                width: parent.width
                                text: root.layouts.layoutInfos[root.layouts.currentLayoutIndex] ?? ""
                                horizontalAlignment: Text.AlignLeft
                                wrapMode: Text.WordWrap
                            }

                            FlatButton {
                                text: qsTrc("dorico", "Page Setup...")
                                navigation.panel: optionsNav
                                navigation.order: 8
                                onClicked: root.layouts.dispatch("command://notation/open-page-settings")
                            }
                        }

                        InspectorSection {
                            width: options.width
                            title: qsTrc("dorico", "Duplex Printing")
                            expanded: false

                            DoricoButton {
                                text: qsTrc("dorico", "Two-sided")
                                available: false
                                navigation.panel: optionsNav
                                navigation.order: 9
                            }
                        }

                        InspectorSection {
                            width: options.width
                            title: qsTrc("dorico", "Annotations")
                            expanded: false

                            Row {
                                spacing: 4

                                DoricoButton {
                                    text: qsTrc("dorico", "Crop marks")
                                    available: false
                                    navigation.panel: optionsNav
                                    navigation.order: 10
                                }

                                DoricoButton {
                                    text: qsTrc("dorico", "Border")
                                    available: false
                                    navigation.panel: optionsNav
                                    navigation.order: 11
                                }
                            }
                        }
                    }
                }

                FlatButton {
                    id: printButton

                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 8

                    accentButton: true
                    enabled: root.printModel.hasChecked
                    text: root.suffix === "" ? qsTrc("dorico", "Print") : qsTrc("dorico", "Export")

                    navigation.panel: optionsNav
                    navigation.order: 20

                    onClicked: {
                        if (root.suffix === "") {
                            root.printModel.print()
                        } else {
                            root.printModel.exportTo(root.suffix, root.filePerLayout)
                        }
                    }
                }
            }
        }
    ]
}
