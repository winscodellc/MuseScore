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
    internal/doricoperf.h
    internal/doricomodestate.cpp
    internal/doricomodestate.h
    internal/modepolicy.cpp
    internal/modepolicy.h
    internal/prefstable.cpp
    internal/prefstable.h
    internal/doriconoteinputcontroller.cpp
    internal/doriconoteinputcontroller.h
    internal/contextmenu.cpp
    internal/contextmenu.h
    internal/formatcontroller.cpp
    internal/formatcontroller.h
    internal/doricomenu.cpp
    internal/doricomenu.h
    internal/doricomenuarranger.cpp
    internal/doricomenuarranger.h
    internal/doricoshell.h
    internal/notationspanels.cpp
    internal/notationspanels.h
    internal/doricopastecontroller.cpp
    internal/doricopastecontroller.h
    internal/pastespecial.cpp
    internal/pastespecial.h
    internal/pasteplanner.cpp
    internal/pasteplanner.h
    internal/doricoselectioncontroller.cpp
    internal/doricoselectioncontroller.h
    internal/filters.cpp
    internal/filters.h
    internal/filtersettings.h
    internal/selectmore.cpp
    internal/transact.h
    internal/transformcontroller.cpp
    internal/transformcontroller.h
    internal/transform.cpp
    internal/transformapply.cpp
    internal/transformapply.h
    internal/transform.h
    internal/selectmore.h
    internal/selectionsummary.cpp
    internal/selectionsummary.h

    video/timecode.cpp
    video/timecode.h
    video/videodata.cpp
    video/videodata.h
    internal/noteinpututils.h
    internal/partlayouts.h
    internal/printjob.cpp
    internal/printjob.h
    internal/jumpbar/commandcatalog.cpp
    internal/jumpbar/commandcatalog.h
    internal/jumpbar/gotoparser.cpp
    internal/jumpbar/gotoparser.h
    internal/jumpbar/jumpbarcontroller.cpp
    internal/jumpbar/jumpbarcontroller.h
    internal/popovercontroller.cpp
    internal/popovercontroller.h
    popover/handlers/barshandler.cpp
    popover/handlers/barshandler.h
    popover/handlers/chordsymbolshandler.cpp
    popover/handlers/chordsymbolshandler.h
    popover/handlers/clefhandler.cpp
    popover/handlers/clefhandler.h
    popover/handlers/dynamicshandler.cpp
    popover/handlers/dynamicshandler.h
    popover/handlers/figuredbasshandler.cpp
    popover/handlers/figuredbasshandler.h
    popover/handlers/fingeringhandler.cpp
    popover/handlers/fingeringhandler.h
    popover/handlers/holdshandler.cpp
    popover/handlers/holdshandler.h
    popover/handlers/keysighandler.cpp
    popover/handlers/keysighandler.h
    popover/handlers/lyricshandler.cpp
    popover/handlers/lyricshandler.h
    popover/handlers/notetoolshandler.cpp
    popover/handlers/notetoolshandler.h
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
    popover/handlers/tupletshandler.cpp
    popover/handlers/tupletshandler.h
    popover/parsers/barsparser.cpp
    popover/parsers/barsparser.h
    popover/parsers/chordsymbolsparser.cpp
    popover/parsers/chordsymbolsparser.h
    popover/parsers/clefparser.cpp
    popover/parsers/clefparser.h
    popover/parsers/dynamicsparser.cpp
    popover/parsers/dynamicsparser.h
    popover/parsers/figuredbassparser.cpp
    popover/parsers/figuredbassparser.h
    popover/parsers/fingeringparser.cpp
    popover/parsers/fingeringparser.h
    popover/parsers/holdsparser.cpp
    popover/parsers/holdsparser.h
    popover/parsers/keysigparser.cpp
    popover/parsers/keysigparser.h
    popover/parsers/notetoolsparser.cpp
    popover/parsers/notetoolsparser.h
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
    popover/parsers/tupletparser.cpp
    popover/parsers/tupletparser.h
    popover/popovernavigation.cpp
    popover/popovernavigation.h
    popover/popoverregistry.cpp
    popover/popoverregistry.h
    popover/popovertypes.h
    idoricocaret.h
    idoricomenuarranger.h
    idoricomodestate.h
    ijumpbarcontroller.h
    ipopovercontroller.h
)

target_link_libraries(dorico PRIVATE engraving muse_uicomponents_qml)
# Markers in exported MIDI (spec 91): sets ExportMidi's marker hook
# Video playback (spec 91 phase B) needs Qt Multimedia (the picture) and FFmpeg (decoding, the soundtrack); without
# them the build works and the video commands are disabled
find_package(Qt6 QUIET OPTIONAL_COMPONENTS Multimedia)
set(DORICO_VIDEO OFF)
if (TARGET Qt6::Multimedia)
    if (APPLE)
        # Qt ships FFmpeg 7 for its multimedia backend: link that, with muse's FFmpeg 7 headers
        set(DORICO_FFMPEG_LIBS)
        foreach (_lib avformat avcodec avutil swscale swresample)
            find_library(DORICO_LIB_${_lib} NAMES ${_lib} PATHS "${Qt6_DIR}/../.." NO_DEFAULT_PATH)
            if (DORICO_LIB_${_lib})
                list(APPEND DORICO_FFMPEG_LIBS ${DORICO_LIB_${_lib}})
            endif()
        endforeach()
        list(LENGTH DORICO_FFMPEG_LIBS _count)
        if (_count EQUAL 5)
            target_include_directories(dorico PRIVATE ${MUSE_FRAMEWORK_SRC_PATH}/media/thirdparty/ffmpeg/v7)
            target_link_libraries(dorico PRIVATE ${DORICO_FFMPEG_LIBS})
            set(DORICO_VIDEO ON)
        endif()
    else()
        find_package(PkgConfig QUIET)
        if (PKG_CONFIG_FOUND)
            pkg_check_modules(DORICO_FFMPEG QUIET IMPORTED_TARGET libavformat libavcodec libavutil libswscale libswresample)
        endif()
        if (DORICO_FFMPEG_FOUND)
            target_link_libraries(dorico PRIVATE PkgConfig::DORICO_FFMPEG)
            set(DORICO_VIDEO ON)
        endif()
    endif()
endif()
if (DORICO_VIDEO)
    target_sources(dorico PRIVATE video/videodecoder.cpp video/videodecoder.h)
    target_link_libraries(dorico PUBLIC Qt6::Multimedia)
    target_compile_definitions(dorico PUBLIC DORICO_HAS_VIDEO)
else()
    message(STATUS "dorico: no Qt Multimedia or FFmpeg, video playback is off")
endif()

if (TARGET iex_midi)
    target_link_libraries(dorico PRIVATE iex_midi)
    target_compile_definitions(dorico PRIVATE DORICO_HAS_MIDI_EXPORT)
endif()

add_subdirectory(qml/MuseScore/Dorico)

# SetupConfigure.cmake only switches off the upstream *_TESTS options when unit tests are disabled,
# so we gate on MUSE_ENABLE_UNIT_TESTS ourselves (keeps upstream edits minimal).
if (MUE_BUILD_DORICO_TESTS AND MUSE_ENABLE_UNIT_TESTS)
    add_subdirectory(tests)
endif()
