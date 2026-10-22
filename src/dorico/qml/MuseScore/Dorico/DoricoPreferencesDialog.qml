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
import Muse.UiComponents

import "prefs/prefscolors.js" as PC

//! Cmd-, (spec 100): Dorico's Preferences dialog. A category list with a search field on the left, one scrolling page of
//! cards on the right, Reset to Factory / Apply / Close at the bottom. Edits are staged by the settings transaction
//! (DoricoPreferencesModel): Apply commits it, Close (or Escape) throws it away.
StyledDialogView {
    id: root

    title: qsTrc("dorico", "Preferences")

    contentWidth: 1130
    contentHeight: 677 // 708 with the title bar, like Dorico's window
    resizable: true
    closeOnEscape: false // Escape closes the page search first (see the key handling on `content`)

    readonly property bool isDark: ui.theme.isDark

    property string currentCategory: "general"
    property var pages: []
    property var anchorItems: ({}) // anchor -> item, for the left list's sub-rows and the page search
    property var rowItems: ({}) // row id -> item
    property var embeddedItems: []

    property bool searchOpen: false
    property var matchIds: []
    property int matchIndex: 0

    function reloadPages() {
        root.anchorItems = ({})
        root.rowItems = ({})
        root.embeddedItems = []
        root.pages = prefs.content(root.currentCategory)
        flick.contentY = 0
        root.refreshMatches()
    }

    function selectCategory(id) {
        if (root.currentCategory === id) {
            return
        }
        root.currentCategory = id
        root.reloadPages()
    }

    function scrollToItem(item) {
        if (!item) {
            return
        }
        const y = item.mapToItem(pagesColumn, 0, 0).y
        flick.contentY = Math.max(0, Math.min(y - 8, flick.contentHeight - flick.height))
    }

    function scrollToAnchor(anchor) {
        root.scrollToItem(root.anchorItems[anchor])
    }

    function stateOf(rowId) {
        if (!root.searchOpen) {
            return 0
        }
        const i = root.matchIds.indexOf(rowId)
        return i < 0 ? 0 : (i === root.matchIndex ? 2 : 1)
    }

    function refreshMatches() {
        root.matchIds = root.searchOpen ? prefs.matches(root.currentCategory, pageSearch.searchText) : []
        root.matchIndex = 0
        root.revealCurrentMatch()
    }

    function revealCurrentMatch() {
        if (root.matchIds.length > 0) {
            const id = root.matchIds[root.matchIndex]
            root.scrollToItem(root.rowItems[id] || root.anchorItems[id])
        }
    }

    function stepMatch(delta) {
        if (root.matchIds.length === 0) {
            return
        }
        root.matchIndex = (root.matchIndex + delta + root.matchIds.length) % root.matchIds.length
        root.revealCurrentMatch()
    }

    function openPageSearch() {
        root.searchOpen = true
        pageSearch.inputField.forceActiveFocus()
        pageSearch.inputField.selectAll()
        root.refreshMatches()
    }

    function closePageSearch() {
        root.searchOpen = false
        root.matchIds = []
    }

    function apply() {
        prefs.apply()
        for (const e of root.embeddedItems) {
            e.applyPage()
        }
    }

    // Close / Escape: unapplied changes ask first (Dorico: "apply them now?")
    function requestClose() {
        prefs.requestClose()
    }

    function closeNow() {
        prefs.discard()
        root.reject()
    }

    DoricoPreferencesModel {
        id: prefs

        onLoaded: root.reloadPages()
        onCloseRequested: function(apply) {
            if (apply) {
                root.apply()
            }
            root.closeNow()
        }
        onResetConfirmed: {
            for (const e of root.embeddedItems) {
                e.resetPage()
            }
            Qt.callLater(prefs.resetToFactory)
        }
    }

    Component.onCompleted: {
        prefs.load()
        categorySearch.inputField.forceActiveFocus()
    }

    onClosed: prefs.discard()

    // Apply is enabled when a setting differs from the last Apply; MuseScore's own pages write settings directly
    Timer {
        interval: 400
        repeat: true
        running: root.isOpened
        onTriggered: prefs.refreshDirty()
    }

    Rectangle {
        id: content

        anchors.fill: parent
        color: PC.pane(root.isDark)

        // Keys are handled here, not with Shortcut items: the framework's shortcut system claims key events first, and a
        // text field hands focus away on Escape. Whatever has focus below bubbles up to this item.
        focus: true

        function isFind(event) { return (event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_F }
        function isCategorySearch(event) { return (event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_L }
        function isNextMatch(event) { return (event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_G }

        // A click on something that takes no focus leaves the window with no focus item, and its keys would go to the main
        // window. Whenever focus ends up outside this item, take it back.
        Connections {
            target: content.Window.window

            function onActiveFocusItemChanged() {
                const window = content.Window.window
                if (!window || !window.active) {
                    return
                }
                let item = window.activeFocusItem
                while (item && item !== content) {
                    item = item.parent
                }
                if (!item) {
                    Qt.callLater(content.forceActiveFocus)
                }
            }
        }

        // Every key that reaches this item (a text field has already had its say) stays in the dialog: otherwise a
        // plain `z` would zoom the score behind it. Only Cmd-Q still quits the app.
        Keys.onShortcutOverride: function(event) {
            if (event.key === Qt.Key_Q && (event.modifiers & Qt.ControlModifier)) {
                return
            }
            content.forceActiveFocus() // the key press goes to this item, even if a text field just dropped focus
            event.accepted = true
        }

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                root.searchOpen ? root.closePageSearch() : root.requestClose()
            } else if (content.isFind(event)) {
                root.openPageSearch()
            } else if (content.isCategorySearch(event)) {
                categorySearch.inputField.forceActiveFocus()
            } else if (content.isNextMatch(event)) {
                root.stepMatch((event.modifiers & Qt.ShiftModifier) ? -1 : 1)
            } else {
                return
            }
            event.accepted = true
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // ---- category list ----
                Rectangle {
                    Layout.preferredWidth: 203
                    Layout.fillHeight: true
                    color: PC.sidebar(root.isDark)

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        SearchField {
                            id: categorySearch

                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.leftMargin: 8
                            Layout.rightMargin: 8

                            hint: qsTrc("dorico", "Search categories...")
                            onTextChanged: function(newTextValue) { prefs.categorySearch = newTextValue }
                            // the field drops focus on Escape; the key itself bubbles to `content`, which acts on it
                            onEscaped: content.forceActiveFocus()
                        }

                        ListView {
                            id: categoryList

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: prefs.categories

                            delegate: Column {
                                id: categoryItem

                                required property var modelData

                                readonly property bool selected: categoryItem.modelData.id === root.currentCategory

                                width: categoryList.width

                                Item {
                                    width: parent.width
                                    height: 27 // a 25 pt row and a 2 pt gap, as in Dorico

                                    Rectangle {
                                        x: 2
                                        width: parent.width - 4
                                        height: 25
                                        color: categoryItem.selected ? PC.sidebarSelected(root.isDark) : PC.sidebarRow(root.isDark)

                                        Rectangle {
                                            visible: categoryItem.selected
                                            width: 3
                                            height: parent.height
                                            color: PC.accent()
                                        }
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: 16
                                        text: categoryItem.modelData.title
                                        color: PC.text(root.isDark)
                                        font.pixelSize: 13
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: { content.forceActiveFocus(); root.selectCategory(categoryItem.modelData.id) }
                                    }
                                }

                                Repeater {
                                    model: categoryItem.selected ? categoryItem.modelData.children : []

                                    delegate: Item {
                                        id: child

                                        required property var modelData

                                        width: categoryItem.width
                                        height: 25

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            x: 32
                                            text: child.modelData.title
                                            color: PC.dimText(root.isDark)
                                            font.pixelSize: 12
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: { content.forceActiveFocus(); root.scrollToAnchor(child.modelData.anchor) }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ---- page ----
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Rectangle {
                        visible: root.searchOpen
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        color: PC.band(root.isDark)

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 8

                            SearchField {
                                id: pageSearch

                                Layout.preferredWidth: 180

                                hint: qsTrc("dorico", "Search page...")
                                onTextChanged: function(newTextValue) { root.refreshMatches() }
                                onAccepted: root.stepMatch(1)
                                onEscaped: content.forceActiveFocus()
                            }

                            FlatButton {
                                Layout.preferredWidth: 32
                                minWidth: 32
                                icon: IconCode.SMALL_ARROW_UP
                                toolTipTitle: qsTrc("dorico", "Previous match")
                                onClicked: root.stepMatch(-1)
                            }

                            FlatButton {
                                Layout.preferredWidth: 32
                                minWidth: 32
                                icon: IconCode.SMALL_ARROW_DOWN
                                toolTipTitle: qsTrc("dorico", "Next match")
                                onClicked: root.stepMatch(1)
                            }

                            Text {
                                text: root.matchIds.length === 0 ? (pageSearch.searchText !== "" ? qsTrc("dorico", "No matches") : "")
                                                                 : (root.matchIndex + 1) + " / " + root.matchIds.length
                                color: PC.dimText(root.isDark)
                                font.pixelSize: 12
                            }

                            Item { Layout.fillWidth: true }

                            FlatButton {
                                Layout.preferredWidth: 32
                                minWidth: 32
                                icon: IconCode.CLOSE_X_ROUNDED
                                toolTipTitle: qsTrc("dorico", "Close")
                                onClicked: root.closePageSearch()
                            }
                        }
                    }

                    Flickable {
                        id: flick

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: width
                        contentHeight: pagesColumn.height
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: StyledScrollBar {}

                        Column {
                            id: pagesColumn

                            width: flick.width - 14

                            Repeater {
                                model: root.pages

                                delegate: Column {
                                    id: pageItem

                                    required property var modelData

                                    width: pagesColumn.width

                                    Component.onCompleted: root.anchorItems[pageItem.modelData.anchor] = pageItem

                                    DoricoSectionBand {
                                        width: parent.width
                                        page: true
                                        title: pageItem.modelData.title
                                    }

                                    Repeater {
                                        model: pageItem.modelData.sections

                                        delegate: Column {
                                            id: sectionItem

                                            required property var modelData

                                            width: pageItem.width

                                            Component.onCompleted: root.anchorItems[sectionItem.modelData.anchor] = sectionItem

                                            DoricoSectionBand {
                                                visible: sectionItem.modelData.title !== ""
                                                width: parent.width
                                                title: sectionItem.modelData.title
                                            }

                                            Item { width: 1; height: 12 }

                                            Item {
                                                x: 16
                                                width: parent.width - 32
                                                height: card.height + 14

                                                DoricoCard {
                                                    id: card

                                                    width: parent.width

                                                    Repeater {
                                                        model: sectionItem.modelData.rows

                                                        delegate: DoricoPrefRow {
                                                            id: prefRow

                                                            required property var modelData

                                                            width: card.width - 32
                                                            row: prefRow.modelData
                                                            prefs: prefs
                                                            matchState: root.stateOf(prefRow.modelData.id)

                                                            Component.onCompleted: root.rowItems[prefRow.modelData.id] = prefRow
                                                        }
                                                    }

                                                    Repeater {
                                                        model: sectionItem.modelData.embedded

                                                        delegate: DoricoEmbeddedPage {
                                                            id: embedded

                                                            required property var modelData

                                                            width: card.width - 32
                                                            page: embedded.modelData.page
                                                            title: embedded.modelData.title
                                                            navigationSection: root.navigationSection
                                                            onPageLoaded: root.embeddedItems.push(embedded)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---- bottom bar ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: PC.sidebar(root.isDark)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    FlatButton {
                        text: qsTrc("dorico", "Reset to Factory")
                        onClicked: prefs.askForReset()
                    }

                    Item { Layout.fillWidth: true }

                    FlatButton {
                        text: qsTrc("dorico", "Apply")
                        enabled: prefs.dirty
                        onClicked: root.apply()
                    }

                    FlatButton {
                        text: qsTrc("dorico", "Close")
                        accentButton: true
                        onClicked: root.requestClose()
                    }
                }
            }
        }
    }
}
