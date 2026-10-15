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

#pragma once

#include <array>

#include "rcommand/commandtypes.h"

namespace mu::dorico {
inline static const muse::rcommand::Command DORICO_NOOP_COMMAND("command://dorico/noop");

// Letters are in NoteName order (C D E F G A B) so a handler can index by NoteName.
inline static const std::array<muse::rcommand::Command, 7> DORICO_PITCH_COMMANDS = {
    muse::rcommand::Command("command://dorico/pitch-c"),
    muse::rcommand::Command("command://dorico/pitch-d"),
    muse::rcommand::Command("command://dorico/pitch-e"),
    muse::rcommand::Command("command://dorico/pitch-f"),
    muse::rcommand::Command("command://dorico/pitch-g"),
    muse::rcommand::Command("command://dorico/pitch-a"),
    muse::rcommand::Command("command://dorico/pitch-b")
};
inline static const std::array<muse::rcommand::Command, 7> DORICO_PITCH_ABOVE_COMMANDS = {
    muse::rcommand::Command("command://dorico/pitch-above-c"),
    muse::rcommand::Command("command://dorico/pitch-above-d"),
    muse::rcommand::Command("command://dorico/pitch-above-e"),
    muse::rcommand::Command("command://dorico/pitch-above-f"),
    muse::rcommand::Command("command://dorico/pitch-above-g"),
    muse::rcommand::Command("command://dorico/pitch-above-a"),
    muse::rcommand::Command("command://dorico/pitch-above-b")
};
inline static const std::array<muse::rcommand::Command, 7> DORICO_PITCH_BELOW_COMMANDS = {
    muse::rcommand::Command("command://dorico/pitch-below-c"),
    muse::rcommand::Command("command://dorico/pitch-below-d"),
    muse::rcommand::Command("command://dorico/pitch-below-e"),
    muse::rcommand::Command("command://dorico/pitch-below-f"),
    muse::rcommand::Command("command://dorico/pitch-below-g"),
    muse::rcommand::Command("command://dorico/pitch-below-a"),
    muse::rcommand::Command("command://dorico/pitch-below-b")
};

// Digits 1..9 in order (128th ... double whole).
inline static const std::array<muse::rcommand::Command, 9> DORICO_SET_DURATION_COMMANDS = {
    muse::rcommand::Command("command://dorico/set-duration-1"),
    muse::rcommand::Command("command://dorico/set-duration-2"),
    muse::rcommand::Command("command://dorico/set-duration-3"),
    muse::rcommand::Command("command://dorico/set-duration-4"),
    muse::rcommand::Command("command://dorico/set-duration-5"),
    muse::rcommand::Command("command://dorico/set-duration-6"),
    muse::rcommand::Command("command://dorico/set-duration-7"),
    muse::rcommand::Command("command://dorico/set-duration-8"),
    muse::rcommand::Command("command://dorico/set-duration-9")
};

inline static const muse::rcommand::Command DORICO_RETURN_COMMAND("command://dorico/return");
inline static const muse::rcommand::Command DORICO_SPACE_COMMAND("command://dorico/space");
inline static const muse::rcommand::Command DORICO_REST_COMMAND("command://dorico/rest");
inline static const muse::rcommand::Command DORICO_TOGGLE_REST_INPUT_COMMAND("command://dorico/toggle-rest-input");
inline static const muse::rcommand::Command DORICO_TOGGLE_CHORD_MODE_COMMAND("command://dorico/toggle-chord-mode");
inline static const muse::rcommand::Command DORICO_TOGGLE_INSERT_MODE_COMMAND("command://dorico/toggle-insert-mode");
inline static const muse::rcommand::Command DORICO_CYCLE_DOTS_COMMAND("command://dorico/cycle-dots");
inline static const muse::rcommand::Command DORICO_NEXT_VOICE_COMMAND("command://dorico/next-voice");
inline static const muse::rcommand::Command DORICO_NEW_VOICE_COMMAND("command://dorico/new-voice");
inline static const muse::rcommand::Command DORICO_TOGGLE_STRESSED_COMMAND("command://dorico/toggle-articulation-stressed");
inline static const muse::rcommand::Command DORICO_TOGGLE_STACCATISSIMO_COMMAND("command://dorico/toggle-articulation-staccatissimo");
inline static const muse::rcommand::Command DORICO_TOGGLE_UNSTRESSED_COMMAND("command://dorico/toggle-articulation-unstressed");
inline static const muse::rcommand::Command DORICO_TOGGLE_TENUTO_STACCATO_COMMAND("command://dorico/toggle-articulation-tenuto-staccato");

// Project window modes and zones (spec 50). Mode commands are in `Mode` order (Setup, Write, Engrave, Play, Print).
inline static const std::array<muse::rcommand::Command, 5> DORICO_MODE_COMMANDS = {
    muse::rcommand::Command("command://dorico/mode-setup"),
    muse::rcommand::Command("command://dorico/mode-write"),
    muse::rcommand::Command("command://dorico/mode-engrave"),
    muse::rcommand::Command("command://dorico/mode-play"),
    muse::rcommand::Command("command://dorico/mode-print")
};
// In `Zone` order (Left, Lower, Right).
inline static const std::array<muse::rcommand::Command, 3> DORICO_TOGGLE_ZONE_COMMANDS = {
    muse::rcommand::Command("command://dorico/toggle-zone-left"),
    muse::rcommand::Command("command://dorico/toggle-zone-lower"),
    muse::rcommand::Command("command://dorico/toggle-zone-right")
};
inline static const muse::rcommand::Command DORICO_TOGGLE_ALL_ZONES_COMMAND("command://dorico/toggle-all-zones");
inline static const muse::rcommand::Command DORICO_TOGGLE_TOOLBAR_COMMAND("command://dorico/toggle-toolbar");

// Popovers (spec 20). The id is the URI tail; PopoverRegistry has the matching PopoverDef.
inline static const std::array<muse::rcommand::Command, 16> DORICO_POPOVER_COMMANDS = {
    muse::rcommand::Command("command://dorico/popover/bars"),
    muse::rcommand::Command("command://dorico/popover/clef"),
    muse::rcommand::Command("command://dorico/popover/dynamics"),
    muse::rcommand::Command("command://dorico/popover/fingering"),
    muse::rcommand::Command("command://dorico/popover/figuredbass"),
    muse::rcommand::Command("command://dorico/popover/holds"),
    muse::rcommand::Command("command://dorico/popover/notetools"),
    muse::rcommand::Command("command://dorico/popover/keysig"),
    muse::rcommand::Command("command://dorico/popover/lyrics"),
    muse::rcommand::Command("command://dorico/popover/timesig"),
    muse::rcommand::Command("command://dorico/popover/ornaments"),
    muse::rcommand::Command("command://dorico/popover/techniques"),
    muse::rcommand::Command("command://dorico/popover/chordsymbols"),
    muse::rcommand::Command("command://dorico/popover/repeats"),
    muse::rcommand::Command("command://dorico/popover/tempo"),
    muse::rcommand::Command("command://dorico/popover/tuplets")
};
}
