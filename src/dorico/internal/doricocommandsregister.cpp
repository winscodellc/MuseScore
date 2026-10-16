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

    const PopoverRegistry popovers = makePopoverRegistry();
    for (const Command& command : DORICO_POPOVER_COMMANDS) {
        const std::string& title = popovers.find(command.pathSegments().back())->title;
        v.push_back(info(command, "Dorico: " + title + " popover", "Opens the " + title + " popover at the selection or caret"));
    }
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
