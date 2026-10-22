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

#include "dorico/internal/contextmenu.h"

using namespace mu::dorico;

namespace {
// Titles of the top-level rows; a separator is "---", a submenu has " >" appended.
QStringList rows(const std::vector<MenuSpec>& specs)
{
    QStringList out;
    for (const MenuSpec& s : specs) {
        out << (s.isSeparator() ? QString("---") : s.isSubmenu() ? s.title + " >" : s.title);
    }
    return out;
}

void collectIds(const std::vector<MenuSpec>& specs, QStringList& out)
{
    for (const MenuSpec& s : specs) {
        out << s.id;
        collectIds(s.children, out);
    }
}

ContextMenuInput note(Mode mode = Mode::Write)
{
    ContextMenuInput in;
    in.mode = mode;
    in.hasSelection = true;
    in.notes = true;
    return in;
}
}

TEST(DoricoContextMenu, NoteInWriteHasDoricosRows)
{
    EXPECT_EQ(rows(scoreContextMenu(note())),
              QStringList({ "Cut", "Copy", "Paste Special >", "---", "Filter >", "---", "Bar Numbers >", "Beaming >",
                            "Chord Symbols and Diagrams >", "Fingering >", "Notehead >", "Octave Line >", "Pedal Lines >",
                            "Staff Size >", "Staff >", "Stem >", "Voices >" }));
}

TEST(DoricoContextMenu, PasteRowOnlyWithMusicOnTheClipboard)
{
    ContextMenuInput in = note();
    EXPECT_EQ(rows(scoreContextMenu(in)).mid(0, 3), QStringList({ "Cut", "Copy", "Paste Special >" }));
    in.clipboardHasMusic = true;
    EXPECT_EQ(rows(scoreContextMenu(in)).mid(0, 4), QStringList({ "Cut", "Copy", "Paste", "Paste Special >" }));
}

TEST(DoricoContextMenu, DynamicTempoAndSlurVariants)
{
    ContextMenuInput dyn;
    dyn.hasSelection = dyn.dynamics = true;
    EXPECT_EQ(rows(scoreContextMenu(dyn)),
              QStringList({ "Cut", "Copy", "Paste Special >", "---", "Filter >", "---", "Edit Font...", "---", "Bar Numbers >",
                            "Dynamics >", "Staff Size >", "Staff >" }));

    ContextMenuInput tempo;
    tempo.hasSelection = tempo.text = true;
    EXPECT_EQ(rows(scoreContextMenu(tempo)).mid(6),
              QStringList({ "Edit Font...", "---", "Bar Numbers >", "Staff Size >", "Staff >" }));

    ContextMenuInput slur;
    slur.hasSelection = slur.slurs = true;
    EXPECT_EQ(rows(scoreContextMenu(slur)).mid(4),
              QStringList({ "Filter >", "---", "Bar Numbers >", "Slurs >", "Staff Size >", "Staff >" }));
}

TEST(DoricoContextMenu, RangeIsTheUnionOfNoteDynamicAndText)
{
    ContextMenuInput range = note();
    range.dynamics = range.text = true;
    const QStringList r = rows(scoreContextMenu(range));
    EXPECT_TRUE(r.contains("Edit Font..."));
    EXPECT_TRUE(r.contains("Dynamics >"));
    EXPECT_TRUE(r.contains("Voices >"));
    EXPECT_LT(r.indexOf("Beaming >"), r.indexOf("Dynamics >"));
    EXPECT_LT(r.indexOf("Dynamics >"), r.indexOf("Fingering >"));
}

TEST(DoricoContextMenu, NothingSelectedIsFilterBarNumbersStaffSize)
{
    ContextMenuInput none;
    EXPECT_EQ(rows(scoreContextMenu(none)), QStringList({ "Filter >", "---", "Bar Numbers >", "Staff Size >" }));
    none.mode = Mode::Engrave;
    EXPECT_EQ(rows(scoreContextMenu(none)), QStringList({ "Filter >", "---", "Bar Numbers >", "Staff Size >" }));
}

TEST(DoricoContextMenu, EngraveHasNoEditingRows)
{
    EXPECT_EQ(rows(scoreContextMenu(note(Mode::Engrave))),
              QStringList({ "Filter >", "---", "Bar Numbers >", "Staff Size >", "Staff >" }));

    ContextMenuInput dyn;
    dyn.mode = Mode::Engrave;
    dyn.hasSelection = dyn.dynamics = true;
    EXPECT_EQ(rows(scoreContextMenu(dyn)),
              QStringList({ "Filter >", "---", "Edit Font...", "---", "Bar Numbers >", "Dynamics >", "Staff Size >", "Staff >" }));
}

TEST(DoricoContextMenu, NoScoreMenuInSetupPlayOrPrint)
{
    for (Mode mode : { Mode::Setup, Mode::Play, Mode::Print }) {
        EXPECT_TRUE(scoreContextMenu(note(mode)).empty());
    }
}

TEST(DoricoContextMenu, NeverDeleteEditElementOrStyle)
{
    ContextMenuInput all = note();
    all.dynamics = all.slurs = all.text = all.clipboardHasMusic = true;
    QStringList ids;
    collectIds(scoreContextMenu(all), ids);
    for (const char* banned : { "command://notation/delete", "command://notation/edit-element", "command://notation/open-edit-style" }) {
        EXPECT_FALSE(ids.contains(banned)) << banned;
    }
}

TEST(DoricoContextMenu, SetupListsMatchDorico)
{
    EXPECT_EQ(rows(playerContextMenu()).first(), "Soloist");
    EXPECT_EQ(rows(playerContextMenu()).last(), "Import Kit...");
    EXPECT_EQ(rows(flowContextMenu()).first(), "Rename");
    EXPECT_EQ(rows(flowContextMenu()).last(), "Delete Flow");
    EXPECT_EQ(rows(layoutContextMenu()).first(), "Rename");
    EXPECT_EQ(rows(layoutContextMenu()).last(), "Duplicate Layout");
}
