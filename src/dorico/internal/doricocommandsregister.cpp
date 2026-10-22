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

#include "doricocommandsregister.h"

#include "../doricocommands.h"
#include "filters.h"
#include "../popover/popoverregistry.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::rcommand;
using namespace mu::dorico;

static CommandInfo info(const Command& command, const std::string& title, const std::string& description)
{
    return CommandInfo{
        command,
        TranslatableString::untranslatable(title.c_str()),
        TranslatableString::untranslatable(description.c_str()),
        InputSchema(),
        Decoration()
    };
}

static std::vector<CommandInfo> makeCommandInfos()
{
    std::vector<CommandInfo> v;
    v.push_back(CommandInfo{
        DORICO_NOOP_COMMAND,
        TranslatableString("dorico", "Dorico no-op"),
        TranslatableString("dorico", "Does nothing; proves the dorico module is wired"),
        InputSchema(),
        Decoration()
    });

    static const char* LETTERS = "CDEFGAB";
    for (int i = 0; i < 7; ++i) {
        const std::string l(1, LETTERS[i]);
        v.push_back(info(DORICO_PITCH_COMMANDS[i], "Dorico: input " + l, "Enters pitch " + l + " during note input"));
        v.push_back(info(DORICO_PITCH_ABOVE_COMMANDS[i], "Dorico: input " + l + " above",
                         "Enters pitch " + l + " in the octave above the previous note"));
        v.push_back(info(DORICO_PITCH_BELOW_COMMANDS[i], "Dorico: input " + l + " below",
                         "Enters pitch " + l + " in the octave below the previous note"));
    }
    for (int i = 0; i < 9; ++i) {
        v.push_back(info(DORICO_SET_DURATION_COMMANDS[i], "Dorico: duration " + std::to_string(i + 1),
                         "Sets the duration; pressing the same digit twice quickly toggles a dot"));
    }

    v.push_back(info(DORICO_RETURN_COMMAND, "Dorico: Return", "Starts or stops note input"));
    v.push_back(info(DORICO_SPACE_COMMAND, "Dorico: Space", "Advances the caret during note input, otherwise plays/stops"));
    v.push_back(info(DORICO_REST_COMMAND, "Dorico: input rest", "Enters a rest during note input"));
    v.push_back(info(DORICO_TOGGLE_REST_INPUT_COMMAND, "Dorico: rest input", "Toggles rest input: letters then enter rests"));
    v.push_back(info(DORICO_TOGGLE_CHORD_MODE_COMMAND, "Dorico: chord mode", "Toggles chord mode"));
    v.push_back(info(DORICO_TOGGLE_INSERT_MODE_COMMAND, "Dorico: insert mode", "Toggles insert mode"));
    v.push_back(info(DORICO_CYCLE_DOTS_COMMAND, "Dorico: cycle dots", "Cycles 0-4 rhythm dots"));
    v.push_back(info(DORICO_NEXT_VOICE_COMMAND, "Dorico: next voice", "Moves to the next existing voice"));
    v.push_back(info(DORICO_NEW_VOICE_COMMAND, "Dorico: new voice", "Moves to the first unused voice"));
    v.push_back(info(DORICO_TOGGLE_STRESSED_COMMAND, "Dorico: stressed", "Toggles the stressed articulation"));
    v.push_back(info(DORICO_TOGGLE_STACCATISSIMO_COMMAND, "Dorico: staccatissimo", "Toggles the staccatissimo articulation"));
    v.push_back(info(DORICO_TOGGLE_UNSTRESSED_COMMAND, "Dorico: unstressed", "Toggles the unstressed articulation"));
    v.push_back(info(DORICO_TOGGLE_TENUTO_STACCATO_COMMAND, "Dorico: tenuto-staccato", "Toggles the tenuto-staccato articulation"));
    v.push_back(info(DORICO_CARET_LEFT_COMMAND, "Dorico: caret left",
                     "Note input: moves the caret back by the rhythmic grid; otherwise selects the previous item"));
    v.push_back(info(DORICO_CARET_RIGHT_COMMAND, "Dorico: caret right",
                     "Note input: moves the caret on by the rhythmic grid; otherwise selects the next item"));
    v.push_back(info(DORICO_GRID_FINER_COMMAND, "Dorico: finer rhythmic grid", "Halves the rhythmic grid (eighth -> 16th)"));
    v.push_back(info(DORICO_GRID_COARSER_COMMAND, "Dorico: coarser rhythmic grid", "Doubles the rhythmic grid (eighth -> quarter)"));
    v.push_back(info(DORICO_SELECT_UP_COMMAND, "Dorico: up", "Selects the note above, crossing staves; note input: caret to the staff above"));
    v.push_back(info(DORICO_SELECT_DOWN_COMMAND, "Dorico: down", "Selects the note below, crossing staves; note input: caret to the staff below"));
    v.push_back(info(DORICO_SELECT_TOP_STAFF_COMMAND, "Dorico: top staff", "Goes to the top staff at the same position"));
    v.push_back(info(DORICO_SELECT_BOTTOM_STAFF_COMMAND, "Dorico: bottom staff", "Goes to the bottom staff at the same position"));
    v.push_back(info(DORICO_RESPELL_ABOVE_COMMAND, "Dorico: respell using note name above", "Respells the selected notes with the next note name up"));
    v.push_back(info(DORICO_RESPELL_BELOW_COMMAND, "Dorico: respell using note name below", "Respells the selected notes with the next note name down"));
    v.push_back(info(DORICO_STOP_TUPLET_COMMAND, "Dorico: stop tuplet input", "Note input: stops the innermost tuplet started from the tuplets popover"));
    v.push_back(info(DORICO_LENGTHEN_BY_GRID_COMMAND, "Dorico: lengthen by grid",
                     "Lengthens the selected notes by the rhythmic grid; lines go to the next grid position or note"));
    v.push_back(info(DORICO_SHORTEN_BY_GRID_COMMAND, "Dorico: shorten by grid",
                     "Shortens the selected notes by the rhythmic grid; lines go to the previous grid position or note"));
    v.push_back(info(DORICO_DOUBLE_DURATION_COMMAND, "Dorico: double duration", "Doubles the selected notes; lines extend to the next note"));
    v.push_back(info(DORICO_HALVE_DURATION_COMMAND, "Dorico: halve duration", "Halves the selected notes; lines shorten to the previous note"));
    v.push_back(info(DORICO_MOVE_RIGHT_COMMAND, "Dorico: move right",
                     "Moves the selected note past the next one, or the selected dynamic or text to the next note"));
    v.push_back(info(DORICO_MOVE_LEFT_COMMAND, "Dorico: move left",
                     "Moves the selected note before the previous one, or the selected dynamic or text to the previous note"));
    v.push_back(info(DORICO_NUDGE_LEFT_COMMAND, "Dorico: nudge left", "Engrave mode: moves the selected items left graphically"));
    v.push_back(info(DORICO_NUDGE_RIGHT_COMMAND, "Dorico: nudge right", "Engrave mode: moves the selected items right graphically"));
    v.push_back(info(DORICO_NUDGE_UP_COMMAND, "Dorico: nudge up",
                     "Engrave mode: moves the selected items up graphically, or raises the selected notes"));
    v.push_back(info(DORICO_NUDGE_DOWN_COMMAND, "Dorico: nudge down",
                     "Engrave mode: moves the selected items down graphically, or lowers the selected notes"));
    v.push_back(info(DORICO_SCISSORS_COMMAND, "Dorico: scissors", "Removes the ties of the selected tie chains"));

    v.push_back(info(DORICO_DUPLICATE_TO_STAFF_ABOVE_COMMAND, "Duplicate to Staff Above", "Copies the selection to the staff above"));
    v.push_back(info(DORICO_DUPLICATE_TO_STAFF_BELOW_COMMAND, "Duplicate to Staff Below", "Copies the selection to the staff below"));
    v.push_back(info(DORICO_MOVE_TO_STAFF_ABOVE_COMMAND, "Move to Staff Above", "Cuts the selection and pastes it onto the staff above"));
    v.push_back(info(DORICO_MOVE_TO_STAFF_BELOW_COMMAND, "Move to Staff Below", "Cuts the selection and pastes it onto the staff below"));
    for (int i = 0; i < 4; ++i) {
        v.push_back(info(DORICO_PASTE_INTO_VOICE_COMMANDS[i], "Voice " + std::to_string(i + 1),
                         "Pastes the copied music into this voice of the selected staff"));
    }
    v.push_back(info(DORICO_PASTE_INTO_NEW_VOICE_COMMAND, "New Voice", "Pastes the copied music into the first voice that is empty there"));
    v.push_back(info(DORICO_SWAP_STAVES_COMMAND, "Swap", "Swaps the contents of the two selected staves for the selected range"));
    v.push_back(info(DORICO_PASTE_ARTICULATIONS_COMMAND, "Paste Articulations",
                     "Applies the copied articulations and slurs to the notes from the selected one on"));
    v.push_back(info(DORICO_EXPLODE_COMMAND, "Explode", "Spreads the notes of the copied chords over the selected staves, top-down"));
    v.push_back(info(DORICO_REDUCE_COMMAND, "Reduce", "Merges the copied staves onto the selected staves, top-down"));
    static const char* TRANSFORM_TITLES[9] = {
        "Invert Pitches...", "Reverse Pitches", "Reverse and Invert Pitches...", "Rotate Pitches...", "Reverse Rhythms",
        "Rotate Rhythms...", "Reverse Pitches and Rhythms", "Reverse and Invert Pitches and Reverse Rhythms...",
        "Rotate Pitches and Rhythms..."
    };
    for (size_t i = 0; i < DORICO_TRANSFORM_COMMANDS.size(); ++i) {
        v.push_back(info(DORICO_TRANSFORM_COMMANDS[i], TRANSFORM_TITLES[i],
                         "Inverts, reverses or rotates the pitches and/or rhythms of the selected notes"));
    }
    v.push_back(info(DORICO_REMOVE_RESTS_COMMAND, "Remove Rests", "Removes the selected rests (voice 1 rests are hidden)"));
    v.push_back(info(DORICO_DELETE_BARS_COMMAND, "Delete Bars", "Deletes the selected bars; the music after them moves left"));
    v.push_back(info(DORICO_EXTEND_TO_BAR_START_COMMAND, "Extend Selection to Bar Start", "Extends the selection back to the start of its bar"));
    v.push_back(info(DORICO_EXTEND_TO_BAR_END_COMMAND, "Extend Selection to Bar End", "Extends the selection on to the end of its bar"));
    v.push_back(info(DORICO_SELECT_MORE_COMMAND, "Select More",
                     "Selects more items of the same types and voices: its bar, then its system, then the whole flow"));
    v.push_back(info(DORICO_DESELECT_ALL_COMMAND, "Select None", "Deselects everything"));
    v.push_back(info(DORICO_NEXT_ITEM_COMMAND, "Next Item at Position", "Selects the next item at the same rhythmic position"));
    v.push_back(info(DORICO_PREV_ITEM_COMMAND, "Previous Item at Position", "Selects the previous item at the same rhythmic position"));
    v.push_back(info(DORICO_SELECT_SAME_PITCH_ONWARDS_COMMAND, "Select Same Pitch Onwards",
                     "Selects the note and every later note of the same pitch in its voice"));

    CommandInfo selectOnly = info(DORICO_FILTER_SELECT_ONLY_COMMAND, "Select Only", "Filters keep only the chosen category of the selection");
    selectOnly.decoration.checkable = Checkable::Yes;
    v.push_back(selectOnly);
    CommandInfo deselectOnly = info(DORICO_FILTER_DESELECT_ONLY_COMMAND, "Deselect Only",
                                    "Filters remove the chosen category from the selection");
    deselectOnly.decoration.checkable = Checkable::Yes;
    v.push_back(deselectOnly);
    v.push_back(info(DORICO_FILTER_NOTES_BY_PITCH_COMMAND, "Filter: Notes > Notes by Pitch",
                     "Filters the selected notes by the pitches you choose in a dialog"));
    for (const FilterDef& def : filterDefs()) {
        v.push_back(info(doricoFilterCommand(def.id), "Filter: " + (def.group.empty() ? "" : def.group + " > ") + def.title,
                         "Filters the selection down to (or without) these items, as set by Select Only / Deselect Only"));
    }

    static const char* MODES[] = { "Setup", "Write", "Engrave", "Play", "Print" };
    for (int i = 0; i < 5; ++i) {
        v.push_back(info(DORICO_MODE_COMMANDS[i], std::string("Dorico: ") + MODES[i] + " mode", "Switches the project window to this mode"));
    }
    static const char* ZONES[] = { "left", "lower", "right" };
    for (int i = 0; i < 3; ++i) {
        v.push_back(info(DORICO_TOGGLE_ZONE_COMMANDS[i], std::string("Dorico: toggle ") + ZONES[i] + " zone",
                         "Shows or hides the panels of this zone in the current mode"));
    }
    v.push_back(info(DORICO_TOGGLE_ALL_ZONES_COMMAND, "Dorico: hide/restore zones", "Hides all open zones, or restores them"));
    v.push_back(info(DORICO_TOGGLE_TOOLBAR_COMMAND, "Dorico: toggle toolbar", "Shows or hides the top toolbars"));
    v.push_back(info(DORICO_OPEN_HUB_COMMAND, "Dorico: Hub", "Opens the Hub: recent scores, new score, plugins and learning"));
    CommandInfo staveSharing = info(DORICO_TOGGLE_STAVE_SHARING_COMMAND, "Enable stave sharing",
                                    "Lets instruments share a staff in this score");
    staveSharing.decoration.checkable = Checkable::Yes; // the Players "…" menu shows a tick while it is on
    v.push_back(staveSharing);
    v.push_back(info(DORICO_OPEN_PUBLISH_COMMAND, "Dorico: Publish", "Opens MuseScore's Publish page (upload and share)"));

    v.push_back(info(DORICO_JUMP_BAR_COMMAND, "Jump Bar", "Opens the jump bar in the mode it was last used in"));
    v.push_back(info(DORICO_JUMP_BAR_COMMANDS_COMMAND, "Jump Bar: Commands", "Opens the jump bar to find and run a command"));
    v.push_back(info(DORICO_JUMP_BAR_GOTO_COMMAND, "Jump Bar: Go To", "Opens the jump bar to go to a bar, rehearsal mark or page"));
    v.push_back(info(DORICO_GO_TO_BAR_COMMAND, "Go To Bar", "Opens the jump bar to go to a bar"));

    const PopoverRegistry popovers = makePopoverRegistry();
    for (const Command& command : DORICO_POPOVER_COMMANDS) {
        const std::string& title = popovers.find(command.pathSegments().back())->title;
        v.push_back(info(command, "Dorico: " + title + " popover", "Opens the " + title + " popover at the selection or caret"));
    }
    v.push_back(info(DORICO_LYRICS_CLOSE_COMMAND, "Dorico: close lyrics", "Lyrics entry: stops editing lyrics"));
    v.push_back(info(DORICO_LYRICS_NEXT_LINE_COMMAND, "Dorico: next lyric line", "Lyrics entry: goes to the next line, adding it if needed"));
    v.push_back(info(DORICO_LYRICS_ABOVE_COMMAND, "Dorico: lyrics above", "Lyrics entry: puts the lyric above the staff"));
    v.push_back(info(DORICO_LYRICS_BELOW_COMMAND, "Dorico: lyrics below", "Lyrics entry: puts the lyric below the staff"));

    v.push_back(info(DORICO_STEM_UP_COMMAND, "Force Stem Up", "Forces the stems of the selected notes up"));
    v.push_back(info(DORICO_STEM_DOWN_COMMAND, "Force Stem Down", "Forces the stems of the selected notes down"));
    v.push_back(info(DORICO_STEM_AUTO_COMMAND, "Remove Forced Stem", "Gives the selected notes' stems their automatic direction"));
    v.push_back(info(DORICO_NOTEHEAD_DEFAULT_COMMAND, "Default Noteheads", "Gives the selected notes the default notehead"));
    v.push_back(info(DORICO_NOTEHEAD_CROSS_COMMAND, "Crosses", "Gives the selected notes cross noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_TRIANGLE_COMMAND, "Triangular", "Gives the selected notes triangle noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_DIAMOND_COMMAND, "Diamonds", "Gives the selected notes diamond noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_SLASH_COMMAND, "Slashes", "Gives the selected notes slash noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_SLASHED1_COMMAND, "Slashed Noteheads (Bottom Left to Top Right)", "Gives the selected notes slashed noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_SLASHED2_COMMAND, "Slashed Noteheads (Top Left to Bottom Right)", "Gives the selected notes slashed noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_CIRCLED_COMMAND, "Circled Noteheads", "Gives the selected notes circled noteheads"));
    v.push_back(info(DORICO_NOTEHEAD_CIRCLED_LARGE_COMMAND, "Large Circled Noteheads", "Gives the selected notes large circled noteheads"));
    return v;
}

static const std::vector<CommandInfo> s_commandInfos = makeCommandInfos();

std::string DoricoCommandsRegister::moduleName() const
{
    return "dorico";
}

const std::vector<Command>& DoricoCommandsRegister::commandList() const
{
    static std::vector<Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& DoricoCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
