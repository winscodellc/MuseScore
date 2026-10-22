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

#include <gtest/gtest.h>

#include "notationscene/notationscenetypes.h"

#include "dorico/internal/modepolicy.h"

using namespace mu::dorico;
using namespace mu::notation;

namespace {
struct Row {
    const char* command;
    bool setup, write, engrave, play, print;
};
}

// One command per category: view, undo, selection, copy, layout, graphic nudge, rhythmic move, pitch, input, delete,
// paste, popover, text editing, and commands of other modules.
TEST(DoricoModePolicy, CommandsPerMode)
{
    const Row rows[] = {
        // command                                  setup  write  engrave play   print
        { "command://notation/view/zoom-in",         true,  true,  true,  true,  true },
        { "command://dorico/mode-engrave",           true,  true,  true,  true,  true },
        { "command://dorico/toggle-zone-right",      true,  true,  true,  true,  true },
        { "command://notation/undo",                 true,  true,  true,  true,  true },
        { "command://notation/select-all",           false, true,  true,  false, false },
        { "command://dorico/select-more",            false, true,  true,  false, false },
        { "command://dorico/filter/dynamics",        false, true,  true,  false, false },
        { "command://notation/copy",                 false, true,  true,  false, false },
        { "command://notation/copy-paste-swap",      false, true,  false, false, false },
        { "command://notation/toggle-system-break",  false, false, true,  false, false },
        { "command://notation/edittext-toggle-bold", false, true,  true,  false, false },
        { "command://dorico/nudge-left",             false, false, true,  false, false },
        { "command://dorico/move-left",              false, true,  false, false, false },
        { "command://notation/pitch-up-diatonic",    false, true,  false, false, false },
        { "command://dorico/pitch-c",                false, true,  false, false, false },
        { "command://dorico/return",                 false, true,  false, false, false },
        { "command://notation/delete",               false, true,  true,  false, false },
        { "command://notation/paste",                false, true,  false, false, false },
        { "command://dorico/popover/dynamics",       false, true,  false, false, false },
        { "command://project/save",                  true,  true,  true,  true,  true },
        { "command://playback/play",                 true,  true,  true,  true,  true },
    };
    for (const Row& r : rows) {
        EXPECT_EQ(isCommandAllowed(Mode::Setup, r.command), r.setup) << r.command;
        EXPECT_EQ(isCommandAllowed(Mode::Write, r.command), r.write) << r.command;
        EXPECT_EQ(isCommandAllowed(Mode::Engrave, r.command), r.engrave) << r.command;
        EXPECT_EQ(isCommandAllowed(Mode::Play, r.command), r.play) << r.command;
        EXPECT_EQ(isCommandAllowed(Mode::Print, r.command), r.print) << r.command;
    }
}

TEST(DoricoModePolicy, InteractionCaps)
{
    EXPECT_EQ(interactionCaps(Mode::Setup), 0);
    EXPECT_EQ(interactionCaps(Mode::Play), 0);
    EXPECT_EQ(interactionCaps(Mode::Print), 0);

    const int write = interactionCaps(Mode::Write);
    EXPECT_TRUE(write & INTERACTION_SELECT);
    EXPECT_TRUE(write & INTERACTION_DRAG_NOTES);
    EXPECT_TRUE(write & INTERACTION_DROP);
    EXPECT_FALSE(write & INTERACTION_DRAG_ITEMS);
    EXPECT_FALSE(write & INTERACTION_GRIPS);
    EXPECT_TRUE(write & INTERACTION_DORICO_RIGHT_CLICK);
    EXPECT_FALSE(INTERACTION_ALL & INTERACTION_DORICO_RIGHT_CLICK);

    const int engrave = interactionCaps(Mode::Engrave);
    EXPECT_TRUE(engrave & INTERACTION_SELECT);
    EXPECT_TRUE(engrave & INTERACTION_DRAG_ITEMS);
    EXPECT_TRUE(engrave & INTERACTION_GRIPS);
    EXPECT_TRUE(engrave & INTERACTION_DRAG_NOTES);
    EXPECT_TRUE(engrave & INTERACTION_EDIT_TEXT);
    EXPECT_FALSE(engrave & INTERACTION_REANCHOR); // drags change offsets, never the rhythmic position
    EXPECT_FALSE(engrave & INTERACTION_DROP);
}
