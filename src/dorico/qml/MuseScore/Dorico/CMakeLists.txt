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

qt_add_qml_module(dorico_qml
    URI MuseScore.Dorico
    VERSION 1.0
    SOURCES
        doricotopbarmodel.cpp
        doricotopbarmodel.h
        doricostatusmodel.cpp
        doricostatusmodel.h
        doricotoolsmodel.cpp
        doricotoolsmodel.h
        doricoviewmenumodel.cpp
        doricoviewmenumodel.h
    QML_FILES
        DoricoPage.qml
        DoricoPlaceholder.qml
        DoricoStatusBar.qml
        DoricoTopBar.qml
        EngravePage.qml
        PlayPage.qml
        PrintPage.qml
        SetupPage.qml
        WritePage.qml
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

fixup_qml_module_dependencies(dorico_qml)
