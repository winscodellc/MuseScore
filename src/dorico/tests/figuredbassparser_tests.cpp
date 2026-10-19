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

#include "dorico/popover/parsers/figuredbassparser.h"

using namespace mu::dorico;

namespace {
// Figures as "[prefix]number", the way the spec writes them; a lone accidental has no number; brackets in parentheses.
std::string show(const std::string& entry)
{
    std::string out;
    for (const Figure& f : parseFiguredBass(entry).figures) {
        out += (out.empty() ? "" : " ");
        out += f.accidentalBracketed ? "(" + f.accidental + ")" : f.accidental;
        if (f.number) {
            out += (f.bracketed ? "(" : "") + std::to_string(f.number) + (f.bracketed ? ")" : "");
        }
        if (f.alteration) {
            out += f.alteration;
        }
    }
    return out;
}

ParseStatus status(const std::string& entry)
{
    return parseFiguredBass(entry).status;
}

std::string text(const std::string& entry)
{
    return parseFiguredBass(entry).text;
}
}

TEST(DoricoFiguredBass, Figures)
{
    EXPECT_EQ(show("6"), "6");
    EXPECT_EQ(show("#6"), "#6");
    EXPECT_EQ(show("#"), "#");
    EXPECT_EQ(show("6,4"), "6 4");
    EXPECT_EQ(show("6,#4,2"), "6 #4 2");
    EXPECT_EQ(show("6#42"), "6 #4 2");
    EXPECT_EQ(show("64"), "6 4");
    EXPECT_EQ(show("643"), "6 4 3");
    EXPECT_EQ(show("b7"), "b7");
    EXPECT_EQ(show("n3"), "n3");
    EXPECT_EQ(show("x4"), "##4");
    EXPECT_EQ(show("10"), "10");
    EXPECT_EQ(show("6,10"), "6 10");
    EXPECT_EQ(show("19"), "19");
}

TEST(DoricoFiguredBass, AccidentalSpellings)
{
    EXPECT_EQ(show("s4"), "#4");
    EXPECT_EQ(show("f4"), "b4");
    EXPECT_EQ(show("N4"), "n4");
    EXPECT_EQ(show("##4"), "##4");
    EXPECT_EQ(show("ds4"), "##4");
    EXPECT_EQ(show("bb4"), "bb4");
    EXPECT_EQ(show("db4"), "bb4");
}

TEST(DoricoFiguredBass, Brackets)
{
    EXPECT_EQ(show("(#643)"), "(#)(6) (4) (3)");
    EXPECT_EQ(show("#64(3)"), "#6 4 (3)");
    EXPECT_EQ(show("(#)643"), "(#)6 4 3");
    EXPECT_EQ(status("(#643)"), ParseStatus::Ok);
}

TEST(DoricoFiguredBass, Alterations)
{
    EXPECT_EQ(show("6+"), "6+");
    EXPECT_EQ(show("7d"), "7d");
    EXPECT_EQ(show("7db"), "7 bb"); // "db" is a double flat, not a diminished 7 followed by b
}

TEST(DoricoFiguredBass, ModeFlagIsIgnored)
{
    for (const std::string flag : { "R", "r", "V", "v", "?", "O", "o", "!" }) {
        EXPECT_EQ(show(flag + "6"), "6") << flag;
    }
}

TEST(DoricoFiguredBass, MuseScoreText)
{
    EXPECT_EQ(text("6"), "6");
    EXPECT_EQ(text("6#42"), "6\n#4\n2");
    EXPECT_EQ(text("2,4,6"), "6\n4\n2"); // largest number on top
    EXPECT_EQ(text("n3"), "h3");
    EXPECT_EQ(text("x4"), "##4");
    EXPECT_EQ(text("(#643)"), "(#6)\n(4)\n(3)");
    EXPECT_EQ(text("(#)6"), "(#)6");
    EXPECT_EQ(text("#64(3)"), "#6\n4\n(3)");
    EXPECT_EQ(text("#"), "#");
    EXPECT_EQ(text("#,6"), "6\n#"); // a lone accidental is a third
    EXPECT_EQ(text("7d"), "7/");
    EXPECT_EQ(text("6+"), "6+");
}

TEST(DoricoFiguredBass, Unsupported)
{
    for (const char* entry : { "4->3", "4_3", "4~3", "4-3", "<3>", "{3}", "ts", "tasto", "Em7", "G/B", "r=2", "d=2", "x#", "###", "bbb",
                               "u", "-", "#6+" }) {
        EXPECT_EQ(status(entry), ParseStatus::Unsupported) << entry;
    }
}

TEST(DoricoFiguredBass, IncompleteAndInvalid)
{
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("R"), ParseStatus::Incomplete);
    EXPECT_EQ(status("6,"), ParseStatus::Incomplete);
    EXPECT_EQ(status("(6"), ParseStatus::Incomplete);
    EXPECT_EQ(status("6,,4"), ParseStatus::Invalid);
    EXPECT_EQ(status(",6"), ParseStatus::Invalid);
    EXPECT_EQ(status("20"), ParseStatus::Invalid);
    EXPECT_EQ(status("0"), ParseStatus::Invalid);
    EXPECT_EQ(status("6)"), ParseStatus::Invalid);
    EXPECT_EQ(status("(("), ParseStatus::Invalid);
    EXPECT_EQ(status("()"), ParseStatus::Invalid);
    EXPECT_EQ(status("d"), ParseStatus::Invalid);
    EXPECT_EQ(status("#b4"), ParseStatus::Invalid);
    EXPECT_EQ(status("6 4"), ParseStatus::Invalid);
    EXPECT_EQ(status("64,3"), ParseStatus::Invalid);
}
