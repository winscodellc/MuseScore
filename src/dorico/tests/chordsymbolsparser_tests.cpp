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

#include "dorico/popover/parsers/chordsymbolsparser.h"

using namespace mu::dorico;

namespace {
void expectChord(const std::string& text, const std::string& normalized)
{
    const ChordSymbolParse p = parseChordSymbol(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.normalized, normalized) << text;
}

ParseStatus status(const std::string& text)
{
    return parseChordSymbol(text).status;
}
}

TEST(DoricoChordSymbolsParser, Qualities)
{
    expectChord("C", "C");
    expectChord("Cmaj7", "Cmaj7");
    expectChord("C^7", "Cmaj7");
    expectChord("C^", "Cmaj7");
    expectChord("C^9", "Cmaj9");
    expectChord("C9maj7", "Cmaj9");
    expectChord("Cma7", "Cmaj7");
    expectChord("CM", "C");
    expectChord("CM7", "Cmaj7");
    expectChord("Cm", "Cm");
    expectChord("Cmin7", "Cm7");
    expectChord("Cmi7", "Cm7");
    expectChord("Cdim", "Cdim");
    expectChord("Cdi", "Cdim");
    expectChord("Co", "Cdim");
    expectChord("Co7", "Cdim7");
    expectChord("Chd", "Cm7b5");
    expectChord("Chalf-dim", "Cm7b5");
    expectChord("Chalfdim7", "Cm7b5");
    expectChord("Caug", "Caug");
    expectChord("Cau", "Caug");
    expectChord("Cag", "Caug");
    expectChord("C+", "Caug");
    expectChord("C6/9", "C6/9");
    expectChord("C69", "C6/9");
    expectChord("C%", "C6/9");
}

TEST(DoricoChordSymbolsParser, Alterations)
{
    expectChord("C7#9", "C7#9");
    expectChord("C7+9", "C7#9");
    expectChord("C7-5", "C7b5");
    expectChord("C7b5", "C7b5");
    expectChord("Cadd+11", "Cadd#11");
    expectChord("Caddb9", "Caddb9");
    expectChord("Csus4", "Csus4");
    expectChord("C7sus4", "C7sus4");
    expectChord("Comit3", "Comit3");
    expectChord("Cno3", "Comit3");
}

TEST(DoricoChordSymbolsParser, RootsAndBass)
{
    expectChord("Dm7b5/G", "Dm7b5/G");
    expectChord("Cmajb5/Eb", "Cmajb5/Eb");
    expectChord("Cmajb5,Eb", "Cmajb5/Eb");
    expectChord("F#dim", "F#dim");
    expectChord("Fx", "F##");
    expectChord("F##", "F##");
    expectChord("Dbb", "Dbb");
    expectChord("H", "B");
    expectChord("Des", "Db");
    expectChord("Fis7", "F#7");
    expectChord("Es", "Eb");
    expectChord("Esus4", "Esus4");
    expectChord("As", "Ab");
    expectChord("do", "C");
    expectChord("reb", "Db");
    expectChord("fa#m", "F#m");
    expectChord("C/H", "C/B");
}

TEST(DoricoChordSymbolsParser, NoChordPolychordParentheses)
{
    expectChord("N.C.", "N.C.");
    expectChord("NC", "N.C.");
    expectChord("none", "N.C.");
    expectChord("no chord", "N.C.");
    expectChord("Cmaj7|D", "Cmaj7|D");
    expectChord("Cmaj7;D", "Cmaj7|D");
    expectChord("(Cmaj7)", "(Cmaj7)");
}

TEST(DoricoChordSymbolsParser, InvalidIncompleteUnsupported)
{
    EXPECT_EQ(status("xyz!"), ParseStatus::Invalid);
    EXPECT_EQ(status("Cxyz"), ParseStatus::Invalid);
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("Chalf-d"), ParseStatus::Incomplete);
    EXPECT_EQ(status("Cmaj7|"), ParseStatus::Incomplete);
    EXPECT_EQ(status("(Cmaj7"), ParseStatus::Incomplete);
    EXPECT_EQ(status("Cdorian"), ParseStatus::Unsupported);
    EXPECT_EQ(status("Cmelodic minor"), ParseStatus::Unsupported);
    EXPECT_EQ(status("CaddF#"), ParseStatus::Unsupported);
    EXPECT_EQ(status("Cx#"), ParseStatus::Unsupported);
    EXPECT_EQ(status("4#"), ParseStatus::Unsupported);
    EXPECT_EQ(status("reg"), ParseStatus::Unsupported);
}
