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

set(MODULE_TEST dorico_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/barsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/chordsymbolsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fingeringparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/clefparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/doricomodule_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/doricoshell_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/doricomenu_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dynamicsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/holdsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/keysigparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/noteinput_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/notationspanels_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ornamentsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/popoverregistry_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/selectionsummary_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/repeatsparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/techniquesparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tempoparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/timesigparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tupletparser_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/trackblocks_tests.cpp
)

set(MODULE_TEST_LINK
    dorico
)

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})

include(SetupGTest)
