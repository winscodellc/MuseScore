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

#include "dorico/popover/parsers/clefparser.h"

using namespace mu::dorico;
using mu::engraving::ClefType;
using Kind = ClefEntry::Kind;

namespace {
void expectClef(const std::string& text, ClefType type)
{
    const ClefParse p = parseClef(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, Kind::Clef) << text;
    EXPECT_EQ(p.entry.clef, type) << text;
}

void expectOctaves(const std::string& text, int octaves)
{
    const ClefParse p = parseClef(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, Kind::OctaveLine) << text;
    EXPECT_EQ(p.entry.octaves, octaves) << text;
}

ParseStatus status(const std::string& text)
{
    return parseClef(text).status;
}
}

TEST(DoricoClefParser, Clefs)
{
    for (const char* t : { "treble", "g", "g2", "sol", "G", "Treble", " treble " }) {
        expectClef(t, ClefType::G);
    }
    for (const char* t : { "bass", "f", "f4", "fa" }) {
        expectClef(t, ClefType::F);
    }
    for (const char* t : { "tenor", "ct", "c4", "ut4" }) {
        expectClef(t, ClefType::C4);
    }
    for (const char* t : { "alto", "ca", "c3", "ut3" }) {
        expectClef(t, ClefType::C3);
    }
    for (const char* t : { "g8ba", "g8d", "treble8ba", "treble8d" }) {
        expectClef(t, ClefType::G8_VB);
    }
    expectClef("g15ba", ClefType::G15_MB);
    expectClef("g8va", ClefType::G8_VA);
    expectClef("g15ma", ClefType::G15_MA);
    expectClef("treble15u", ClefType::G15_MA);
    expectClef("ct8d", ClefType::C4_8VB);
    expectClef("f8ba", ClefType::F8_VB);
    expectClef("bass15ba", ClefType::F15_MB);
    expectClef("f8va", ClefType::F_8VA);
    expectClef("f15ma", ClefType::F_15MA);
    expectClef("perc", ClefType::PERC);
    expectClef("tab4", ClefType::TAB4);
    expectClef("tab6", ClefType::TAB);
    expectClef("g1", ClefType::G_1);
    expectClef("soprano", ClefType::C1);
    expectClef("ut1", ClefType::C1);
    expectClef("mezzo", ClefType::C2);
    expectClef("c2", ClefType::C2);
    expectClef("baritone", ClefType::C5);
    expectClef("c5", ClefType::C5);
    expectClef("baritonebass", ClefType::F_B);
    expectClef("subbass", ClefType::F_C);
}

TEST(DoricoClefParser, OctaveShifts)
{
    expectClef("g+1", ClefType::G8_VA);
    expectClef("g+2", ClefType::G15_MA);
    expectClef("treble-1", ClefType::G8_VB);
    expectClef("f-2", ClefType::F15_MB);
    expectClef("bass+1", ClefType::F_8VA);
    expectClef("tenor-1", ClefType::C4_8VB);
    expectClef("g8ba+1", ClefType::G);
    expectClef("g+0", ClefType::G);
    for (const char* t : { "g+3", "f-3", "tenor+1", "alto-1", "perc+1", "treble+1t", "g-1c", "invisible+1" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
}

TEST(DoricoClefParser, OctaveLines)
{
    for (const char* t : { "8va", "8", "8u", "1u", "8VA" }) {
        expectOctaves(t, 1);
    }
    for (const char* t : { "15ma", "15", "15u", "2u" }) {
        expectOctaves(t, 2);
    }
    for (const char* t : { "22ma", "22", "22u", "3u" }) {
        expectOctaves(t, 3);
    }
    for (const char* t : { "8ba", "8vb", "8d", "1d" }) {
        expectOctaves(t, -1);
    }
    for (const char* t : { "15ba", "15vb", "15d", "2d" }) {
        expectOctaves(t, -2);
    }
    for (const char* t : { "22ba", "22vb", "22d", "3d" }) {
        expectOctaves(t, -3);
    }
    for (const char* t : { "|", "stop", "STOP" }) {
        const ClefParse p = parseClef(t);
        EXPECT_EQ(p.status, ParseStatus::Ok) << t;
        EXPECT_EQ(p.entry.kind, Kind::Stop) << t;
    }
}

TEST(DoricoClefParser, UnsupportedIncompleteInvalid)
{
    for (const char* t : { "invisible", "alto8ba", "ca8ba", "ca8d", "alto8d", "loco" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    for (const char* t : { "", " ", "tre", "c", "ba", "1", "2", "3", "8v", "15m", "st", "lo", "g+", "treble-", "u" }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "8x", "xyz", "9va", "trebles", "g+x", "g+1x", "x+1", "||", "4" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoClefParser, TextRoundTrips)
{
    for (int i = 0; i < int(ClefType::MAX); ++i) {
        const ClefType type = ClefType(i);
        const std::string text = clefText(type);
        if (!text.empty()) {
            expectClef(text, type);
        }
    }
    EXPECT_EQ(clefText(ClefType::G), "treble");
    EXPECT_EQ(clefText(ClefType::F8_VB), "bass8ba");
    EXPECT_EQ(clefText(ClefType::TAB_SERIF), "");
    EXPECT_EQ(clefText(ClefType::INVALID), "");
    for (int octaves : { -3, -2, -1, 1, 2, 3 }) {
        expectOctaves(octaveLineText(octaves), octaves);
    }
    EXPECT_EQ(octaveLineText(1), "8va");
    EXPECT_EQ(octaveLineText(-2), "15ba");
}
