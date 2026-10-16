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

#include "dorico/popover/parsers/keysigparser.h"

using namespace mu::dorico;
using Kind = KeySigEntry::Kind;

namespace {
void expectKey(const std::string& text, int fifths, bool minor)
{
    const KeySigParse p = parseKeySig(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, Kind::Tonal) << text;
    EXPECT_EQ(p.entry.fifths, fifths) << text;
    EXPECT_EQ(p.entry.minor, minor) << text;
}

ParseStatus status(const std::string& text)
{
    return parseKeySig(text).status;
}
}

TEST(DoricoKeySigParser, Names)
{
    expectKey("C", 0, false);
    expectKey("c", -3, true);
    expectKey("a", 0, true);
    expectKey("A", 3, false);
    expectKey("Eb", -3, false);
    expectKey("Ef", -3, false);
    expectKey("f#", 3, true);
    expectKey("Fs", 6, false);
    expectKey("bb", -5, true);
    expectKey("Bb", -2, false);
    expectKey("b", 2, true);
    expectKey("Cb", -7, false);
    expectKey("C#", 7, false);
    expectKey("a#", 7, true);
    expectKey("ab", -7, true);
    expectKey(" G ", 1, false);
}

TEST(DoricoKeySigParser, Counts)
{
    expectKey("3#", 3, false);
    expectKey("3s", 3, false);
    expectKey("4b", -4, false);
    expectKey("4f", -4, false);
    expectKey("7#", 7, false);
    expectKey("1b", -1, false);
}

TEST(DoricoKeySigParser, Open)
{
    for (const char* text : { "open", "atonal", "ATONAL", "Open" }) {
        const KeySigParse p = parseKeySig(text);
        EXPECT_EQ(p.status, ParseStatus::Ok) << text;
        EXPECT_EQ(p.entry.kind, Kind::Open) << text;
    }
}

TEST(DoricoKeySigParser, IncompleteAndInvalid)
{
    for (const char* text : { "", " ", "o", "ope", "at", "ato", "atona", "O", "3", "7" }) {
        EXPECT_EQ(status(text), ParseStatus::Incomplete) << text;
    }
    for (const char* text : { "Es", "D#", "ff", "8#", "0b", "0", "8", "3x", "33#", "xyz", "Ebb", "H", "s", "#", "opens" }) {
        EXPECT_EQ(status(text), ParseStatus::Invalid) << text;
    }
}

TEST(DoricoKeySigParser, TextRoundTrips)
{
    for (int fifths = -7; fifths <= 7; ++fifths) {
        for (bool minor : { false, true }) {
            const std::string text = keySigText({ Kind::Tonal, fifths, minor });
            expectKey(text, fifths, minor);
        }
    }
    EXPECT_EQ(keySigText({ Kind::Tonal, -3, false }), "Eb");
    EXPECT_EQ(keySigText({ Kind::Tonal, 3, true }), "f#");
    EXPECT_EQ(keySigText({ Kind::Tonal, 0, true }), "a");
    EXPECT_EQ(keySigText({ Kind::Open, 0, false }), "open");
}
