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
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui

import "prefscolors.js" as PC

//! One row of a card (spec 100): label, control, optional second control, italic help text. A row without a MuseScore
//! setting is dimmed, and says why in a tooltip. `matchState` highlights page-search matches (1 match, 2 current).
Item {
    id: root

    required property var row
    required property var prefs

    property int matchState: 0

    readonly property bool isButton: root.row.type === "button"
    readonly property bool isDark: ui.theme.isDark
    readonly property bool hasExtra: root.row.extra !== undefined
    readonly property bool checkOn: { root.prefs.revision; return root.prefs.value(root.row.id) === true }

    implicitHeight: Math.max(36, line.implicitHeight + (help.visible ? help.implicitHeight + 6 : 0) + 10)

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: -8
        anchors.rightMargin: -8
        radius: 6
        color: root.matchState === 2 ? PC.currentHighlight() : root.matchState === 1 ? PC.highlight() : "transparent"
    }

    RowLayout {
        id: line

        x: 0
        y: 5
        width: parent.width
        spacing: 12
        opacity: root.row.backed ? 1.0 : 0.5

        Text {
            // a check box carries its label to its right; a button with no separate label is its own caption
            Layout.preferredWidth: 340
            visible: root.row.type !== "check" && !(root.isButton && root.row.caption === "")
            text: root.row.label
            color: PC.text(root.isDark)
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            elide: Text.ElideNone
        }

        DoricoPrefControl {
            row: root.row
            prefs: root.prefs
        }

        Text {
            visible: root.row.type === "check"
            Layout.fillWidth: true
            text: root.row.label
            color: PC.text(root.isDark)
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }

        Text {
            // the extra's own label, unless its unit already says it ("minutes")
            visible: root.hasExtra && root.row.extra.label !== "" && root.row.extra.label !== root.row.extra.unit
            text: root.hasExtra ? root.row.extra.label : ""
            color: PC.text(root.isDark)
            font.pixelSize: 13
        }

        DoricoPrefControl {
            visible: root.hasExtra
            // an extra is only live while its check box is on
            enabled: root.hasExtra && root.row.extra.backed && (root.row.type !== "check" || root.checkOn)
            row: root.hasExtra ? root.row.extra : root.row
            prefs: root.prefs
        }

        Item {
            visible: root.row.type !== "check"
            Layout.fillWidth: true
        }
    }

    Text {
        id: help

        visible: root.row.help !== ""
        anchors.top: line.bottom
        anchors.topMargin: 2
        x: root.row.type === "check" ? 32 : 0
        width: parent.width - x
        text: root.row.help
        color: PC.dimText(root.isDark)
        font.pixelSize: 12
        font.italic: true
        wrapMode: Text.WordWrap
    }

    HoverHandler { id: hover }

    ToolTip.visible: hover.hovered && !root.row.backed
    ToolTip.delay: 600
    ToolTip.text: qsTrc("dorico", "Not available yet")
}
