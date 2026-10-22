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

//! One preference control (spec 100), chosen by the row's type. `row` is a map from DoricoPreferencesModel.content(),
//! `prefs` the model. The value is re-read whenever prefs.revision changes; an unbacked row is disabled.
Loader {
    id: root

    required property var row
    required property var prefs

    readonly property var current: { root.prefs.revision; return root.prefs.value(root.row.id) }

    function indexOfValue(options, value) {
        for (let i = 0; i < options.length; ++i) {
            if (options[i].value === value) {
                return i
            }
        }
        return -1
    }

    function set(value) {
        root.prefs.setValue(root.row.id, value)
    }

    enabled: root.row.backed
    sourceComponent: {
        switch (root.row.type) {
        case "check": return checkComponent
        case "dropdown": return dropdownComponent
        case "segmented": return segmentedComponent
        case "spinner": return spinnerComponent
        case "text": return textComponent
        case "path": return pathComponent
        case "hex": return hexComponent
        case "slider": return sliderComponent
        case "button": return buttonComponent
        }
        return null
    }

    Component {
        id: checkComponent

        CheckBox {
            checked: root.current === true
            onClicked: root.set(!checked)
        }
    }

    Component {
        id: dropdownComponent

        StyledDropdown {
            width: 220
            model: root.row.options
            currentIndex: root.indexOfValue(root.row.options, root.current)
            onActivated: function(index, value) { root.set(value) }
        }
    }

    Component {
        id: segmentedComponent

        DoricoSegmented {
            model: root.row.options
            currentValue: root.current
            enabled: root.row.backed
            onActivated: function(value) { root.set(value) }
        }
    }

    Component {
        id: spinnerComponent

        IncrementalPropertyControl {
            width: 96
            currentValue: Number(root.current)
            minValue: root.row.min
            maxValue: root.row.max
            step: 1
            decimals: Number.isInteger(root.row.min) && Number.isInteger(Number(root.current)) ? 0 : 1
            measureUnitsSymbol: root.row.unit
            onValueEdited: function(newValue) { root.set(newValue) }
        }
    }

    Component {
        id: textComponent

        TextInputField {
            width: 220
            currentText: String(root.current)
            onTextEditingFinished: function(newValue) { root.set(newValue) }
        }
    }

    Component {
        id: pathComponent

        FilePicker {
            width: 360
            pickerType: FilePicker.PickerType.Directory
            dialogTitle: root.row.label
            path: String(root.current)
            buttonType: FlatButton.TextOnly
            buttonText: qsTrc("dorico", "Choose...")
            buttonOrientation: Qt.Horizontal
            onPathEdited: function(newPath) { root.set(newPath) }
        }
    }

    Component {
        id: hexComponent

        DoricoColorRow {
            value: String(root.current)
            enabled: root.row.backed
            onColorEdited: function(newValue) { root.set(newValue) }
        }
    }

    Component {
        id: sliderComponent

        StyledSlider {
            width: 220
            from: 0
            to: 100
            value: Number(root.current)
            onMoved: root.set(value)
        }
    }

    Component {
        id: buttonComponent

        DoricoButton {
            text: root.row.caption !== "" ? root.row.caption : root.row.label
            available: root.row.backed
        }
    }
}
