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

#include "dorico/popover/parsers/repeatsparser.h"

using namespace mu::dorico;
using mu::engraving::JumpType;
using mu::engraving::MarkerType;
using mu::engraving::TremoloType;
using Kind = RepeatsEntry::Kind;

namespace {
RepeatsEntry ok(const std::string& text)
{
    const RepeatsParse p = parseRepeats(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

void expectMarker(const std::string& text, MarkerType t)
{
    const RepeatsEntry e = ok(text);
    EXPECT_EQ(e.kind, Kind::Marker) << text;
    EXPECT_EQ(e.marker, t) << text;
}

void expectJump(const std::string& text, JumpType t)
{
    const RepeatsEntry e = ok(text);
    EXPECT_EQ(e.kind, Kind::Jump) << text;
    EXPECT_EQ(e.jump, t) << text;
}

void expectTremolo(const std::string& text, TremoloType t)
{
    const RepeatsEntry e = ok(text);
    EXPECT_EQ(e.kind, Kind::Tremolo) << text;
    EXPECT_EQ(e.tremolo, t) << text;
}

ParseStatus status(const std::string& text)
{
    return parseRepeats(text).status;
}
}

TEST(DoricoRepeatsParser, Markers)
{
    for (const char* t : { "segno", "seg", "s", "$", " SEGNO " }) {
        expectMarker(t, MarkerType::SEGNO);
    }
    for (const char* t : { "to coda", "toc", "tc", "To Coda" }) {
        expectMarker(t, MarkerType::TOCODA);
    }
    for (const char* t : { "fine", "fin", "f" }) {
        expectMarker(t, MarkerType::FINE);
    }
    for (const char* t : { "coda", "co", "c" }) {
        expectMarker(t, MarkerType::CODA);
    }
}

TEST(DoricoRepeatsParser, Jumps)
{
    for (const char* t : { "da capo", "dc", "D.C." }) {
        expectJump(t, JumpType::DC); // dc is not read as the start of dcalf
    }
    for (const char* t : { "DC al Fine", "dcalf", "D.C. al Fine" }) {
        expectJump(t, JumpType::DC_AL_FINE);
    }
    for (const char* t : { "DC al Coda", "dcalc", "D.C. al Coda" }) {
        expectJump(t, JumpType::DC_AL_CODA);
    }
    for (const char* t : { "dal segno", "ds", "D.S." }) {
        expectJump(t, JumpType::DS);
    }
    for (const char* t : { "DS al Fine", "dsalf", "D.S. al Fine" }) {
        expectJump(t, JumpType::DS_AL_FINE);
    }
    for (const char* t : { "DS al Coda", "dsalc", "D.S. al Coda" }) {
        expectJump(t, JumpType::DS_AL_CODA);
    }
}

TEST(DoricoRepeatsParser, Endings)
{
    for (const char* t : { "end", "ending" }) {
        const RepeatsEntry e = ok(t);
        EXPECT_EQ(e.kind, Kind::Ending);
        EXPECT_FALSE(e.add);
    }
    const RepeatsEntry e = ok("add");
    EXPECT_EQ(e.kind, Kind::Ending);
    EXPECT_TRUE(e.add);
}

TEST(DoricoRepeatsParser, Tremolos)
{
    for (const char* t : { "/", "\\", "1" }) {
        expectTremolo(t, TremoloType::R8);
    }
    for (const char* t : { "//", "\\\\", "2" }) {
        expectTremolo(t, TremoloType::R16);
    }
    expectTremolo("///", TremoloType::R32);
    expectTremolo("\\\\\\\\", TremoloType::R64);
    for (const char* t : { "z", "zonstem" }) {
        expectTremolo(t, TremoloType::BUZZ_ROLL);
    }
    expectTremolo("/2", TremoloType::C8);
    expectTremolo("22", TremoloType::C16);
    expectTremolo("\\\\\\2", TremoloType::C32);
    for (const char* t : { "////2", "42" }) {
        expectTremolo(t, TremoloType::C64);
    }
    for (const char* t : { "0", "clear" }) {
        EXPECT_EQ(ok(t).kind, Kind::ClearTremolo) << t;
    }
}

TEST(DoricoRepeatsParser, Regions)
{
    for (const char* t : { "%", "%1" }) {
        const RepeatsEntry e = ok(t);
        EXPECT_EQ(e.kind, Kind::MeasureRepeat);
        EXPECT_EQ(e.measures, 1);
    }
    EXPECT_EQ(ok("%2").measures, 2);
    EXPECT_EQ(ok("%4").measures, 4);
    EXPECT_EQ(ok("slash").kind, Kind::Slash);
}

TEST(DoricoRepeatsParser, UnsupportedAndInvalid)
{
    for (const char* t : { "%1,2", "%1,4", "%2,4", "%3", "nb", "number bars", "//rel", "2a" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    for (const char* t : { "", " ", "d", "D.C. al", "%1,", "zon" }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t; // a valid start: neutral border
    }
    for (const char* t : { "banana", "xyz", "%x", "5" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoRepeatsParser, TextRoundTrips)
{
    for (const char* t : { "segno", "to coda", "D.C.", "D.C. al Coda", "D.S. al Fine", "/", "//", "z", "////2", "%1", "%4",
                           "end" }) {
        EXPECT_EQ(repeatsText(ok(t)), t);
    }
}
