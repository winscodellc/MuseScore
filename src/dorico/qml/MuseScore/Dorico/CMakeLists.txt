# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2026 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

muse_create_qml_module(dorico_qml FOR dorico)

# Lucide icons (ISC, see icons/LICENSE-lucide.txt), used by DoricoButton (spec 60).
file(GLOB DORICO_ICONS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} icons/*.svg)

qt_add_qml_module(dorico_qml
    URI MuseScore.Dorico
    VERSION 1.0
    SOURCES
        doricopanelmodel.cpp
        doricopanelmodel.h
        doricotopbarmodel.cpp
        doricotopbarmodel.h
        doricostatusmodel.cpp
        doricostatusmodel.h
        doricotoolsmodel.cpp
        doricotoolsmodel.h
        doricoviewmenumodel.cpp
        doricoviewmenumodel.h
        jumpbarmodel.cpp
        jumpbarmodel.h
        lowerzonemodel.cpp
        lowerzonemodel.h
        popovermodel.cpp
        popovermodel.h
        printmodel.cpp
        printmodel.h
        trackinspectormodel.cpp
        trackinspectormodel.h
        trackoverviewmodel.cpp
        trackoverviewmodel.h
    QML_FILES
        DoricoButton.qml
        DoricoPage.qml
        DoricoPlaceholder.qml
        DoricoStatusBar.qml
        DoricoTopBar.qml
        EngravePage.qml
        LayoutsPanel.qml
        PlayersList.qml
        LowerZonePanel.qml
        NotationsPanel.qml
        NotationsToolbox.qml
        NotesPanel.qml
        NotesToolbox.qml
        PaletteSections.qml
        PlayPage.qml
        Popover.qml
        PrintPage.qml
        SetupPage.qml
        ToolSection.qml
        TrackOverview.qml
        WritePage.qml
        DoricoChannelStrip.qml
        DoricoMixer.qml
        InspectorSection.qml
        JumpBar.qml
        ResourceDropdown.qml
        trackcolor.js
    RESOURCES
        ${DORICO_ICONS}
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
        TARGET muse_dockwindow_qml
        # By URI: these modules are added after dorico in src/CMakeLists.txt, so their targets do not exist yet.
        MuseScore.AppShell
        MuseScore.NotationScene
        MuseScore.Playback
        MuseScore.InstrumentsScene
        MuseScore.Palette
        MuseScore.PropertiesPanel
)

# DoricoPanelModel (spec 53) builds palette trees; palette/internal headers include their module headers without a path.
target_include_directories(dorico_qml PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../../palette)
target_link_libraries(dorico_qml PRIVATE palette)
# TrackInspectorModel (spec 57) reads the mixer's channel items; their header includes playback headers without a path.
target_include_directories(dorico_qml PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../../playback
                                              ${CMAKE_CURRENT_LIST_DIR}/../../../../playback/qml/MuseScore/Playback)
target_link_libraries(dorico_qml PRIVATE playback_qml)

fixup_qml_module_dependencies(dorico_qml)
