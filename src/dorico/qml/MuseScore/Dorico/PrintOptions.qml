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

//! The sections of Print mode's Print Options panel (spec 62): Destination, Job Type, Page Setup, Duplex Printing,
//! Annotations. Every value is a per-layout setting of the PrintModel (`settings`); with several layouts checked a value
//! they do not share is `undefined` here and shows blank, and editing a control sets it on all of them.
Column {
    id: root

    required property PrintModel printModel
    required property NavigationPanel nav
    //! "A4, portrait, 2 pages" of the current layout
    property string layoutInfo: ""

    signal pageSetupRequested()

    spacing: 2

    readonly property var s: root.printModel.settings
    readonly property bool toPrinter: root.printModel.anyPrinter
    readonly property bool toGraphics: root.printModel.anyGraphics
    // Page ranges and job types are written by our own PDF writer and printer path; PNG and SVG use MuseScore's export.
    readonly property bool rangesAvailable: root.toPrinter || root.s.graphicsFormat === "pdf"

    // Enum values of printjob.h (JobType, RangeKind, PrintOn, Flip, Orientation)
    readonly property var jobTypes: [
        { "text": qsTrc("dorico", "Normal"), "value": 0 },
        { "text": qsTrc("dorico", "Spreads"), "value": 1 },
        { "text": qsTrc("dorico", "2-up"), "value": 2 },
        { "text": qsTrc("dorico", "Booklet"), "value": 3 }
    ]
    readonly property var printOnItems: [
        { "text": qsTrc("dorico", "One side only"), "value": 0 },
        { "text": qsTrc("dorico", "Both sides manually"), "value": 1 },
        { "text": qsTrc("dorico", "Both sides automatically"), "value": 2 }
    ]
    readonly property var flipItems: [
        { "text": qsTrc("dorico", "Flip automatically"), "value": 0 },
        { "text": qsTrc("dorico", "Flip long side"), "value": 1 },
        { "text": qsTrc("dorico", "Flip short side"), "value": 2 }
    ]
    readonly property var formatItems: [
        { "text": qsTrc("dorico", "PDF"), "value": "pdf" },
        { "text": qsTrc("dorico", "PNG"), "value": "png" },
        { "text": qsTrc("dorico", "SVG"), "value": "svg" }
    ]
    readonly property var printerItems: {
        const names = root.printModel.printers
        if (names.length === 0) {
            return [{ "text": qsTrc("dorico", "No printer found"), "value": "" }]
        }
        return names.map(n => ({ "text": n, "value": n }))
    }
    readonly property var paperItems: [{ "text": qsTrc("dorico", "Layout page size"), "value": "" }]
        .concat(root.printModel.paperSizes.map(p => ({ "text": p.name, "value": p.key })))

    // The orientation a button group shows: Auto follows the job type (landscape for everything but Normal), then the layout
    readonly property var shownOrientation: root.s.orientation !== 0 ? root.s.orientation
                                            : (root.s.jobType !== 0 || root.printModel.landscapeLayout ? 2 : 1)
    readonly property var shownOddOrientation: root.s.oddFinalOrientation === 0 ? 1 : root.s.oddFinalOrientation

    function set(key, value) {
        root.printModel.setSetting(key, value)
    }

    function indexOfValue(items, value) {
        for (let i = 0; i < items.length; ++i) {
            if (items[i].value === value) {
                return i
            }
        }
        return -1
    }

    // A row of buttons, one per item, the current one highlighted (Dorico's Printer | Graphics, All | Pages | Flows ...)
    component Segmented: RowLayout {
        id: seg

        property var items: []
        property var current: undefined
        property int navOrder: 0
        property bool segEnabled: true

        signal picked(var value)

        width: parent.width
        spacing: 0

        Repeater {
            model: seg.items

            DoricoButton {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                text: modelData.text
                available: modelData.available !== false
                enabled: seg.segEnabled
                accentButton: seg.current === modelData.value
                transparent: seg.current !== modelData.value
                navigation.panel: root.nav
                navigation.order: seg.navOrder + index
                onTriggered: seg.picked(modelData.value)
            }
        }
    }

    component Caption: StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
    }

    InspectorSection {
        width: root.width
        title: qsTrc("dorico", "Destination")

        Segmented {
            items: [
                { "text": qsTrc("dorico", "Printer"), "value": false },
                { "text": qsTrc("dorico", "Graphics"), "value": true }
            ]
            current: root.s.graphics
            navOrder: 1
            onPicked: function(value) { root.set("graphics", value) }
        }

        // ---- Printer ----
        StyledDropdown {
            width: parent.width
            visible: root.toPrinter
            model: root.printerItems
            currentIndex: root.s.printerName === undefined
                          ? -1 : root.indexOfValue(root.printerItems, root.s.printerName || root.printModel.defaultPrinter)
            navigation.panel: root.nav
            navigation.order: 3
            onActivated: function(index, value) { root.set("printerName", value) }
        }

        RowLayout {
            width: parent.width
            visible: root.toPrinter
            spacing: 8

            StyledTextLabel {
                text: qsTrc("dorico", "Copies")
            }

            IncrementalPropertyControl {
                Layout.fillWidth: true
                currentValue: root.s.copies ?? 1
                isIndeterminate: root.s.copies === undefined
                minValue: 1
                maxValue: 999
                step: 1
                decimals: 0
                navigation.panel: root.nav
                navigation.order: 4
                onValueEdited: function(newValue) { root.set("copies", newValue) }
            }
        }

        CheckBox {
            width: parent.width
            visible: root.toPrinter
            text: qsTrc("dorico", "Collate copies")
            checked: root.s.collate === true
            navigation.panel: root.nav
            navigation.order: 5
            onClicked: root.set("collate", !checked)
        }

        // ---- Graphics ----
        StyledDropdown {
            width: parent.width
            visible: root.toGraphics
            model: root.formatItems
            currentIndex: root.indexOfValue(root.formatItems, root.s.graphicsFormat)
            navigation.panel: root.nav
            navigation.order: 6
            onActivated: function(index, value) { root.set("graphicsFormat", value) }
        }

        Segmented {
            visible: root.toGraphics
            items: [
                { "text": qsTrc("dorico", "Mono"), "value": false },
                { "text": qsTrc("dorico", "Color"), "value": true }
            ]
            current: root.s.colour
            segEnabled: root.s.graphicsFormat === "pdf"
            navOrder: 7
            onPicked: function(value) { root.set("colour", value) }
        }

        Caption {
            visible: root.toGraphics && root.s.graphicsFormat !== "pdf"
            text: qsTrc("dorico", "PNG and SVG use MuseScore's image export settings, one file per page.")
            opacity: 0.7
        }
    }

    InspectorSection {
        width: root.width
        title: qsTrc("dorico", "Job Type")

        StyledDropdown {
            width: parent.width
            enabled: root.rangesAvailable
            model: root.jobTypes
            currentIndex: root.indexOfValue(root.jobTypes, root.s.jobType)
            navigation.panel: root.nav
            navigation.order: 10
            onActivated: function(index, value) { root.set("jobType", value) }
        }

        Caption {
            text: qsTrc("dorico", "Print range:")
        }

        Segmented {
            items: [
                { "text": qsTrc("dorico", "All"), "value": 0 },
                { "text": qsTrc("dorico", "Pages"), "value": 1 },
                // MuseScore has one flow per project
                { "text": qsTrc("dorico", "Flows"), "value": 2, "available": false }
            ]
            current: root.s.range
            segEnabled: root.rangesAvailable && root.s.jobType !== 3
            navOrder: 11
            onPicked: function(value) { root.set("range", value) }
        }

        TextInputField {
            width: parent.width
            visible: root.s.range === 1 && root.s.jobType !== 3
            currentText: root.s.pagesText ?? ""
            hint: qsTrc("dorico", "e.g. 1-4, 6, 8-")
            navigation.panel: root.nav
            navigation.order: 14
            onTextEditingFinished: function(newTextValue) { root.set("pagesText", newTextValue) }
        }

        Caption {
            visible: root.s.range === 1 && !root.printModel.pagesValid
            text: qsTrc("dorico", "Not a page range of this layout. Use numbers like 1-4, 6, 8-.")
            color: "#E54D4D"
        }

        Caption {
            visible: root.s.jobType === 3
            text: qsTrc("dorico", "A booklet always uses the full range.")
            opacity: 0.7
        }

        CheckBox {
            width: parent.width
            visible: root.toGraphics && root.s.graphicsFormat === "pdf"
            text: qsTrc("dorico", "One file per layout")
            checked: root.s.filePerLayout === true
            navigation.panel: root.nav
            navigation.order: 15
            onClicked: root.set("filePerLayout", !checked)
        }

        Caption {
            visible: !root.rangesAvailable
            text: qsTrc("dorico", "Job types and page ranges apply to printing and PDF files.")
            opacity: 0.7
        }
    }

    InspectorSection {
        width: root.width
        title: qsTrc("dorico", "Page Setup")

        Caption {
            text: root.layoutInfo
        }

        // A graphics file keeps the layout's page size: only its orientation can change.
        StyledDropdown {
            width: parent.width
            visible: root.toPrinter
            model: root.paperItems
            currentIndex: root.indexOfValue(root.paperItems, root.s.paperSize)
            navigation.panel: root.nav
            navigation.order: 20
            onActivated: function(index, value) { root.set("paperSize", value) }
        }

        Segmented {
            items: [
                { "text": qsTrc("dorico", "Portrait"), "value": 1 },
                { "text": qsTrc("dorico", "Landscape"), "value": 2 }
            ]
            current: root.shownOrientation
            navOrder: 21
            onPicked: function(value) { root.set("orientation", value) }
        }

        Segmented {
            visible: root.toPrinter
            items: [
                { "text": qsTrc("dorico", "Fit to Paper"), "value": true },
                { "text": qsTrc("dorico", "Custom Scale"), "value": false }
            ]
            current: root.s.fitToPaper
            navOrder: 23
            onPicked: function(value) { root.set("fitToPaper", value) }
        }

        RowLayout {
            width: parent.width
            visible: root.toPrinter && root.s.fitToPaper === false
            spacing: 8

            StyledTextLabel {
                text: qsTrc("dorico", "Scale factor")
            }

            IncrementalPropertyControl {
                Layout.fillWidth: true
                currentValue: root.s.scalePercent ?? 100
                isIndeterminate: root.s.scalePercent === undefined
                measureUnitsSymbol: "%"
                minValue: 1
                maxValue: 1000
                step: 5
                decimals: 0
                navigation.panel: root.nav
                navigation.order: 25
                onValueEdited: function(newValue) { root.set("scalePercent", newValue) }
            }
        }

        // Spreads and 2-up: the unpaired last page can go on other paper
        CheckBox {
            width: parent.width
            visible: root.toPrinter && (root.s.jobType === 1 || root.s.jobType === 2)
            text: qsTrc("dorico", "Print odd final page on")
            checked: root.s.oddFinalOn === true
            navigation.panel: root.nav
            navigation.order: 26
            onClicked: root.set("oddFinalOn", !checked)
        }

        StyledDropdown {
            width: parent.width
            visible: root.toPrinter && root.s.oddFinalOn === true && (root.s.jobType === 1 || root.s.jobType === 2)
            model: root.paperItems
            currentIndex: root.indexOfValue(root.paperItems, root.s.oddFinalSize)
            navigation.panel: root.nav
            navigation.order: 27
            onActivated: function(index, value) { root.set("oddFinalSize", value) }
        }

        Segmented {
            visible: root.toPrinter && root.s.oddFinalOn === true && (root.s.jobType === 1 || root.s.jobType === 2)
            items: [
                { "text": qsTrc("dorico", "Portrait"), "value": 1 },
                { "text": qsTrc("dorico", "Landscape"), "value": 2 }
            ]
            current: root.shownOddOrientation
            navOrder: 28
            onPicked: function(value) { root.set("oddFinalOrientation", value) }
        }

        FlatButton {
            text: qsTrc("dorico", "Page Setup...")
            navigation.panel: root.nav
            navigation.order: 30
            onClicked: root.pageSetupRequested()
        }
    }

    InspectorSection {
        width: root.width
        title: qsTrc("dorico", "Duplex Printing")
        visible: root.toPrinter
        expanded: false

        StyledDropdown {
            width: parent.width
            // "Both sides automatically" only for a printer that can do it (and when it is already chosen)
            model: root.printModel.autoDuplexAvailable || root.s.printOn === 2 ? root.printOnItems : root.printOnItems.slice(0, 2)
            currentIndex: root.indexOfValue(root.printOnItems, root.s.printOn)
            navigation.panel: root.nav
            navigation.order: 40
            onActivated: function(index, value) { root.set("printOn", value) }
        }

        StyledTextLabel {
            visible: root.s.printOn === 2
            text: qsTrc("dorico", "Flip image (portrait)")
            horizontalAlignment: Text.AlignLeft
        }

        StyledDropdown {
            width: parent.width
            visible: root.s.printOn === 2
            model: root.flipItems
            currentIndex: root.indexOfValue(root.flipItems, root.s.flipPortrait)
            navigation.panel: root.nav
            navigation.order: 41
            onActivated: function(index, value) { root.set("flipPortrait", value) }
        }

        StyledTextLabel {
            visible: root.s.printOn === 2
            text: qsTrc("dorico", "Flip image (landscape)")
            horizontalAlignment: Text.AlignLeft
        }

        StyledDropdown {
            width: parent.width
            visible: root.s.printOn === 2
            model: root.flipItems
            currentIndex: root.indexOfValue(root.flipItems, root.s.flipLandscape)
            navigation.panel: root.nav
            navigation.order: 42
            onActivated: function(index, value) { root.set("flipLandscape", value) }
        }

        CheckBox {
            width: parent.width
            visible: root.s.printOn === 1
            text: qsTrc("dorico", "Reverse order of outward pages")
            checked: root.s.reverseOutward === true
            navigation.panel: root.nav
            navigation.order: 43
            onClicked: root.set("reverseOutward", !checked)
        }
    }

    InspectorSection {
        width: root.width
        title: qsTrc("dorico", "Annotations")
        expanded: false

        CheckBox {
            width: parent.width
            text: qsTrc("dorico", "Crop marks")
            checked: root.s.cropMarks === true
            navigation.panel: root.nav
            navigation.order: 50
            onClicked: root.set("cropMarks", !checked)
        }

        CheckBox {
            width: parent.width
            text: qsTrc("dorico", "Border")
            checked: root.s.border === true
            navigation.panel: root.nav
            navigation.order: 51
            onClicked: root.set("border", !checked)
        }

        CheckBox {
            width: parent.width
            text: qsTrc("dorico", "Date and time")
            checked: root.s.dateTime === true
            navigation.panel: root.nav
            navigation.order: 52
            onClicked: root.set("dateTime", !checked)
        }

        CheckBox {
            width: parent.width
            text: qsTrc("dorico", "Watermark")
            checked: root.s.watermark === true
            navigation.panel: root.nav
            navigation.order: 53
            onClicked: root.set("watermark", !checked)
        }

        TextInputField {
            width: parent.width
            enabled: root.s.watermark === true
            currentText: root.s.watermarkText ?? ""
            hint: qsTrc("dorico", "Watermark text")
            navigation.panel: root.nav
            navigation.order: 54
            onTextEditingFinished: function(newTextValue) { root.set("watermarkText", newTextValue) }
        }

        Caption {
            text: qsTrc("dorico", "Crop marks and border only print where the page is smaller than the paper.")
            opacity: 0.7
        }
    }
}
