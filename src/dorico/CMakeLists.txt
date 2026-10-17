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

muse_create_module(dorico)

target_sources(dorico PRIVATE
    doricomodule.cpp
    doricomodule.h
    doricocommands.h

    internal/doricocommandsregister.cpp
    internal/doricocommandsregister.h
    internal/doricocommandsstate.cpp
    internal/doricocommandsstate.h
    internal/doricoactionscontroller.cpp
    internal/doricoactionscontroller.h
    internal/doricocaret.cpp
    internal/doricocaret.h
    internal/doricodurationscontroller.cpp
    internal/doricodurationscontroller.h
    internal/doricomodestate.cpp
    internal/doricomodestate.h
    internal/doriconoteinputcontroller.cpp
    internal/doriconoteinputcontroller.h
    internal/doricomenu.cpp
    internal/doricomenu.h
    internal/doricomenuarranger.cpp
    internal/doricomenuarranger.h
    internal/doricoshell.h
    internal/notationspanels.cpp
    internal/notationspanels.h
    internal/selectionsummary.cpp
    internal/selectionsummary.h
    internal/noteinpututils.h
    internal/partlayouts.h
    internal/popovercontroller.cpp
    internal/popovercontroller.h
    popover/handlers/barshandler.cpp
    popover/handlers/barshandler.h
    popover/handlers/clefhandler.cpp
    popover/handlers/clefhandler.h
    popover/handlers/dynamicshandler.cpp
    popover/handlers/dynamicshandler.h
    popover/handlers/holdshandler.cpp
    popover/handlers/holdshandler.h
    popover/handlers/keysighandler.cpp
    popover/handlers/keysighandler.h
    popover/handlers/ornamentshandler.cpp
    popover/handlers/ornamentshandler.h
    popover/handlers/pendingatcaret.cpp
    popover/handlers/pendingatcaret.h
    popover/handlers/repeatshandler.cpp
    popover/handlers/repeatshandler.h
    popover/handlers/techniqueshandler.cpp
    popover/handlers/techniqueshandler.h
    popover/handlers/tempohandler.cpp
    popover/handlers/tempohandler.h
    popover/handlers/timesighandler.cpp
    popover/handlers/timesighandler.h
    popover/parsers/barsparser.cpp
    popover/parsers/barsparser.h
    popover/parsers/clefparser.cpp
    popover/parsers/clefparser.h
    popover/parsers/dynamicsparser.cpp
    popover/parsers/dynamicsparser.h
    popover/parsers/holdsparser.cpp
    popover/parsers/holdsparser.h
    popover/parsers/keysigparser.cpp
    popover/parsers/keysigparser.h
    popover/parsers/ornamentsparser.cpp
    popover/parsers/ornamentsparser.h
    popover/parsers/repeatsparser.cpp
    popover/parsers/repeatsparser.h
    popover/parsers/techniquesparser.cpp
    popover/parsers/techniquesparser.h
    popover/parsers/tempoparser.cpp
    popover/parsers/tempoparser.h
    popover/parsers/timesigparser.cpp
    popover/parsers/timesigparser.h
    popover/popoverregistry.cpp
    popover/popoverregistry.h
    popover/popovertypes.h
    idoricocaret.h
    idoricomenuarranger.h
    idoricomodestate.h
    ipopovercontroller.h
)

target_link_libraries(dorico PRIVATE engraving muse_uicomponents_qml)

if (MUE_BUILD_DORICO_QML)
    add_subdirectory(qml/MuseScore/Dorico)
endif()

# SetupConfigure.cmake only switches off the upstream *_TESTS options when unit tests are disabled,
# so we gate on MUSE_ENABLE_UNIT_TESTS ourselves (keeps upstream edits minimal).
if (MUE_BUILD_DORICO_TESTS AND MUSE_ENABLE_UNIT_TESTS)
    add_subdirectory(tests)
endif()
