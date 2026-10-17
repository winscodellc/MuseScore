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

#include "dorico/popover/parsers/holdsparser.h"

using namespace mu::dorico;
using mu::engraving::SymId;
using Kind = HoldEntry::Kind;

namespace {
void expectHold(const std::string& text, Kind kind, SymId sym)
{
    const HoldParse p = parseHold(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, kind) << text;
    EXPECT_EQ(p.entry.sym, sym) << text;
}

ParseStatus status(const std::string& text)
{
    return parseHold(text).status;
}
}

TEST(DoricoHoldsParser, Fermatas)
{
    for (const char* t : { "fer", "fermata", "pause", " FER ", "Fermata" }) {
        expectHold(t, Kind::Fermata, SymId::fermataAbove);
    }
    for (const char* t : { "verylongfer", "verylongfermata" }) {
        expectHold(t, Kind::Fermata, SymId::fermataVeryLongAbove);
    }
    expectHold("longfer", Kind::Fermata, SymId::fermataLongAbove);
    expectHold("shortfer", Kind::Fermata, SymId::fermataShortAbove);
    expectHold("veryshortfer", Kind::Fermata, SymId::fermataVeryShortAbove);
    expectHold("shorthenzefer", Kind::Fermata, SymId::fermataShortHenzeAbove);
    expectHold("longhenzefermata", Kind::Fermata, SymId::fermataLongHenzeAbove);
    expectHold("curlew", Kind::Fermata, SymId::curlewSign);
}

TEST(DoricoHoldsParser, BreathMarks)
{
    for (const char* t : { "comma", ",", "breathmarkcomma" }) {
        expectHold(t, Kind::Breath, SymId::breathMarkComma);
    }
    expectHold("breathmarktick", Kind::Breath, SymId::breathMarkTick);
    expectHold("breathmarkupbow", Kind::Breath, SymId::breathMarkUpbow);
    expectHold("breathmarksalzedo", Kind::Breath, SymId::breathMarkSalzedo);
}

TEST(DoricoHoldsParser, Caesuras)
{
    for (const char* t : { "caes", "caesura", "//" }) {
        expectHold(t, Kind::Caesura, SymId::caesura);
    }
    for (const char* t : { "thick//", "caesurathick", "thickcaesura" }) {
        expectHold(t, Kind::Caesura, SymId::caesuraThick);
    }
    expectHold("curved//", Kind::Caesura, SymId::caesuraCurved);
    expectHold("short//", Kind::Caesura, SymId::caesuraShort);
    expectHold("single//", Kind::Caesura, SymId::caesuraSingleStroke);
}

TEST(DoricoHoldsParser, IncompleteAndInvalid)
{
    for (const char* t : { "", "fe", "breath", "/", "short", "  " }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "xyz", "fer2", "///", "commas" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoHoldsParser, PrefillText)
{
    EXPECT_EQ(holdText(SymId::fermataAbove), "fer");
    EXPECT_EQ(holdText(SymId::fermataLongAbove), "longfer");
    EXPECT_EQ(holdText(SymId::breathMarkComma), "comma");
    EXPECT_EQ(holdText(SymId::caesuraThick), "thick//");
    EXPECT_EQ(holdText(SymId::chantCaesura), "");
    for (SymId s : { SymId::fermataShortHenzeAbove, SymId::curlewSign, SymId::breathMarkSalzedo, SymId::caesuraSingleStroke }) {
        expectHold(holdText(s), parseHold(holdText(s)).entry.kind, s); // round trip
    }
}
