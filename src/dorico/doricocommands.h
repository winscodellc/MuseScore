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
#include <string>

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

// Caret and rhythmic grid (spec 12).
inline static const muse::rcommand::Command DORICO_CARET_LEFT_COMMAND("command://dorico/caret-left");
inline static const muse::rcommand::Command DORICO_CARET_RIGHT_COMMAND("command://dorico/caret-right");
inline static const muse::rcommand::Command DORICO_GRID_FINER_COMMAND("command://dorico/grid-finer");
inline static const muse::rcommand::Command DORICO_GRID_COARSER_COMMAND("command://dorico/grid-coarser");
inline static const muse::rcommand::Command DORICO_SELECT_UP_COMMAND("command://dorico/select-up");
inline static const muse::rcommand::Command DORICO_SELECT_DOWN_COMMAND("command://dorico/select-down");
inline static const muse::rcommand::Command DORICO_SELECT_TOP_STAFF_COMMAND("command://dorico/select-top-staff");
inline static const muse::rcommand::Command DORICO_SELECT_BOTTOM_STAFF_COMMAND("command://dorico/select-bottom-staff");
inline static const muse::rcommand::Command DORICO_RESPELL_ABOVE_COMMAND("command://dorico/respell-above");
inline static const muse::rcommand::Command DORICO_RESPELL_BELOW_COMMAND("command://dorico/respell-below");
inline static const muse::rcommand::Command DORICO_STOP_TUPLET_COMMAND("command://dorico/stop-tuplet");

// Duration edits and rhythmic moves (spec 13).
inline static const muse::rcommand::Command DORICO_LENGTHEN_BY_GRID_COMMAND("command://dorico/lengthen-by-grid");
inline static const muse::rcommand::Command DORICO_SHORTEN_BY_GRID_COMMAND("command://dorico/shorten-by-grid");
inline static const muse::rcommand::Command DORICO_DOUBLE_DURATION_COMMAND("command://dorico/double-duration");
inline static const muse::rcommand::Command DORICO_HALVE_DURATION_COMMAND("command://dorico/halve-duration");
inline static const muse::rcommand::Command DORICO_MOVE_RIGHT_COMMAND("command://dorico/move-right");
inline static const muse::rcommand::Command DORICO_MOVE_LEFT_COMMAND("command://dorico/move-left");
inline static const muse::rcommand::Command DORICO_SCISSORS_COMMAND("command://dorico/scissors");

// Engrave mode's Alt-arrow (spec 97): nudges graphically; Up / Down on notes changes their pitch, as in Dorico 6.
inline static const muse::rcommand::Command DORICO_NUDGE_LEFT_COMMAND("command://dorico/nudge-left");
inline static const muse::rcommand::Command DORICO_NUDGE_RIGHT_COMMAND("command://dorico/nudge-right");
inline static const muse::rcommand::Command DORICO_NUDGE_UP_COMMAND("command://dorico/nudge-up");
inline static const muse::rcommand::Command DORICO_NUDGE_DOWN_COMMAND("command://dorico/nudge-down");

// Edit > Paste Special (spec 41). Paste Into Voice commands are in voice order (1-4).
inline static const muse::rcommand::Command DORICO_DUPLICATE_TO_STAFF_ABOVE_COMMAND("command://dorico/duplicate-to-staff-above");
inline static const muse::rcommand::Command DORICO_DUPLICATE_TO_STAFF_BELOW_COMMAND("command://dorico/duplicate-to-staff-below");
inline static const muse::rcommand::Command DORICO_MOVE_TO_STAFF_ABOVE_COMMAND("command://dorico/move-to-staff-above");
inline static const muse::rcommand::Command DORICO_MOVE_TO_STAFF_BELOW_COMMAND("command://dorico/move-to-staff-below");
inline static const std::array<muse::rcommand::Command, 4> DORICO_PASTE_INTO_VOICE_COMMANDS = {
    muse::rcommand::Command("command://dorico/paste-into-voice-1"),
    muse::rcommand::Command("command://dorico/paste-into-voice-2"),
    muse::rcommand::Command("command://dorico/paste-into-voice-3"),
    muse::rcommand::Command("command://dorico/paste-into-voice-4")
};
inline static const muse::rcommand::Command DORICO_PASTE_INTO_NEW_VOICE_COMMAND("command://dorico/paste-into-new-voice");
inline static const muse::rcommand::Command DORICO_SWAP_STAVES_COMMAND("command://dorico/swap-staves");
inline static const muse::rcommand::Command DORICO_PASTE_ARTICULATIONS_COMMAND("command://dorico/paste-articulations");
inline static const muse::rcommand::Command DORICO_EXPLODE_COMMAND("command://dorico/explode");
inline static const muse::rcommand::Command DORICO_REDUCE_COMMAND("command://dorico/reduce");

// Write > Transform (spec 65), in the order of the menu: Pitches, Rhythm, All
inline static const std::array<muse::rcommand::Command, 9> DORICO_TRANSFORM_COMMANDS = {
    muse::rcommand::Command("command://dorico/transform/invert-pitches"),
    muse::rcommand::Command("command://dorico/transform/reverse-pitches"),
    muse::rcommand::Command("command://dorico/transform/reverse-invert-pitches"),
    muse::rcommand::Command("command://dorico/transform/rotate-pitches"),
    muse::rcommand::Command("command://dorico/transform/reverse-rhythms"),
    muse::rcommand::Command("command://dorico/transform/rotate-rhythms"),
    muse::rcommand::Command("command://dorico/transform/reverse-all"),
    muse::rcommand::Command("command://dorico/transform/reverse-invert-all"),
    muse::rcommand::Command("command://dorico/transform/rotate-all")
};

// Delete family (spec 43); Delete itself is `notation/delete`, hooked in DoricoPasteController
inline static const muse::rcommand::Command DORICO_REMOVE_RESTS_COMMAND("command://dorico/remove-rests");
inline static const muse::rcommand::Command DORICO_DELETE_BARS_COMMAND("command://dorico/delete-bars");

// Selection (spec 44). Select All is `notation/select-all`, Shift-arrows stay MuseScore's `add-to-selection-*`.
inline static const muse::rcommand::Command DORICO_EXTEND_TO_BAR_START_COMMAND("command://dorico/extend-to-bar-start");
inline static const muse::rcommand::Command DORICO_EXTEND_TO_BAR_END_COMMAND("command://dorico/extend-to-bar-end");
inline static const muse::rcommand::Command DORICO_SELECT_MORE_COMMAND("command://dorico/select-more");
inline static const muse::rcommand::Command DORICO_DESELECT_ALL_COMMAND("command://dorico/deselect-all");
inline static const muse::rcommand::Command DORICO_NEXT_ITEM_COMMAND("command://dorico/next-item-at-position");
inline static const muse::rcommand::Command DORICO_PREV_ITEM_COMMAND("command://dorico/prev-item-at-position");
inline static const muse::rcommand::Command DORICO_SELECT_SAME_PITCH_ONWARDS_COMMAND("command://dorico/select-same-pitch-onwards");

// Filters (spec 45). One command per category: command://dorico/filter/<FilterDef::id> (see internal/filters.h).
inline static const muse::rcommand::Command DORICO_FILTER_SELECT_ONLY_COMMAND("command://dorico/filter-select-only");
inline static const muse::rcommand::Command DORICO_FILTER_DESELECT_ONLY_COMMAND("command://dorico/filter-deselect-only");
inline static const muse::rcommand::Command DORICO_FILTER_NOTES_BY_PITCH_COMMAND("command://dorico/filter-notes-by-pitch");
inline std::string doricoFilterCommandUri(const std::string& id)
{
    return "command://dorico/filter/" + id;
}
inline muse::rcommand::Command doricoFilterCommand(const std::string& id)
{
    return muse::rcommand::Command(doricoFilterCommandUri(id));
}

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
// Window > Hub (Dorico's start page = MuseScore's Home page) and File > Publish: the top bar has no Home/Publish buttons
inline static const muse::rcommand::Command DORICO_OPEN_HUB_COMMAND("command://dorico/open-hub");
inline static const muse::rcommand::Command DORICO_OPEN_PUBLISH_COMMAND("command://dorico/open-publish");
// Players panel "…" menu (spec 55): the score's stave sharing, a switch in MuseScore's instrument tree
inline static const muse::rcommand::Command DORICO_TOGGLE_STAVE_SHARING_COMMAND("command://dorico/toggle-stave-sharing");

// Lyrics popover keys (spec 33): enabled only while a lyric is being edited, when every other dorico command is off.
inline static const muse::rcommand::Command DORICO_LYRICS_CLOSE_COMMAND("command://dorico/lyrics-close");
inline static const muse::rcommand::Command DORICO_LYRICS_NEXT_LINE_COMMAND("command://dorico/lyrics-next-line");
inline static const muse::rcommand::Command DORICO_LYRICS_ABOVE_COMMAND("command://dorico/lyrics-above");
inline static const muse::rcommand::Command DORICO_LYRICS_BELOW_COMMAND("command://dorico/lyrics-below");

// Jump bar (spec 46)
inline static const muse::rcommand::Command DORICO_JUMP_BAR_COMMAND("command://dorico/jump-bar");
inline static const muse::rcommand::Command DORICO_JUMP_BAR_COMMANDS_COMMAND("command://dorico/jump-bar-commands");
inline static const muse::rcommand::Command DORICO_JUMP_BAR_GOTO_COMMAND("command://dorico/jump-bar-goto");
inline static const muse::rcommand::Command DORICO_GO_TO_BAR_COMMAND("command://dorico/go-to-bar");

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
