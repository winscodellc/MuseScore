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

import QtQuick

import Muse.Ui
import Muse.UiComponents

//! A collapsible Track Inspector section (spec 57): a title bar with a chevron on the right (down = open,
//! left = closed), as in Dorico, and the content below it.
Column {
    id: root

    property string title: ""
    property bool expanded: true
    default property alias content: body.data

    spacing: 0

    Rectangle {
        width: root.width
        height: 30
        color: ui.theme.backgroundSecondaryColor

        StyledTextLabel {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            font: ui.theme.bodyBoldFont
        }

        DoricoButton {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            doricoIcon: root.expanded ? "svg:chevron-down" : "svg:chevron-left"
            iconBoxSize: 16
            transparent: true
            toolTipTitle: root.expanded ? qsTrc("dorico", "Collapse") : qsTrc("dorico", "Expand")
            onTriggered: root.expanded = !root.expanded
        }

        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: 30
            onClicked: root.expanded = !root.expanded
        }
    }

    Column {
        id: body

        x: 8
        width: root.width - 16
        topPadding: 8
        bottomPadding: 10
        spacing: 8
        visible: root.expanded
    }
}
