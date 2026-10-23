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
        videomodel.cpp
        videomodel.h
        doricopanelmodel.cpp
        doricopanelmodel.h
        doricomixermodel.cpp
        doricomixermodel.h
        doricopreferencesmodel.cpp
        doricopreferencesmodel.h
        doricoprinter.cpp
        doricoprinter.h
        doricotopbarmodel.cpp
        doricotopbarmodel.h
        doricostatusmodel.cpp
        doricostatusmodel.h
        doricotoolsmodel.cpp
        doricotoolsmodel.h
        doricocontextmenumodel.cpp
        doricocontextmenumodel.h
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
        AddMarkerDialog.qml
        VideoPanel.qml
        VideoPropertiesDialog.qml
        DoricoTimeDisplay.qml
        DoricoTimecodeField.qml
        DoricoButton.qml
        DoricoShellPage.qml
        EngraveFormattingPanel.qml
        EngravePagesPanel.qml
        EngraveToolbox.qml
        LazyPane.qml
        PlayInspectorPanel.qml
        PrintLayoutsPanel.qml
        PrintOptionsPanel.qml
        SetupFlowsPanel.qml
        SetupPlayersPanel.qml
        ZoneHeader.qml
        ZoneHolder.qml
        DoricoPlaceholder.qml
        DoricoStatusBar.qml
        DoricoTopBar.qml
        LayoutsPanel.qml
        PlayersList.qml
        LowerZonePanel.qml
        NotationsPanel.qml
        NotationsToolbox.qml
        NotesPanel.qml
        NotesToolbox.qml
        PaletteSections.qml
        Popover.qml
        PrintOptions.qml
        ToolSection.qml
        TrackOverview.qml
        DoricoChannelStrip.qml
        DoricoChannelStripSections.qml
        DoricoMixer.qml
        InspectorSection.qml
        FilterNotesByPitchDialog.qml
        InvertPitchesDialog.qml
        RotateDialog.qml
        DoricoPreferencesDialog.qml
        prefs/DoricoCard.qml
        prefs/DoricoColorRow.qml
        prefs/DoricoEmbeddedPage.qml
        prefs/DoricoHelpIcon.qml
        prefs/DoricoPrefControl.qml
        prefs/DoricoPrefRow.qml
        prefs/DoricoSectionBand.qml
        prefs/DoricoSegmented.qml
        prefs/prefscolors.js
        JumpBar.qml
        ResourceDropdown.qml
        mixervalues.js
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
        MuseScore.Preferences
)

# DoricoPanelModel (spec 53) builds palette trees; palette/internal headers include their module headers without a path.
target_include_directories(dorico_qml PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../../palette)
target_link_libraries(dorico_qml PRIVATE palette)
# DoricoPrinter (spec 62) prints without the system dialog.
target_link_libraries(dorico_qml PRIVATE Qt::PrintSupport Qt::Widgets)
# TrackInspectorModel (spec 57) reads the mixer's channel items; their header includes playback headers without a path.
target_include_directories(dorico_qml PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../../playback
                                              ${CMAKE_CURRENT_LIST_DIR}/../../../../playback/qml/MuseScore/Playback)
target_link_libraries(dorico_qml PRIVATE playback_qml)

fixup_qml_module_dependencies(dorico_qml)
