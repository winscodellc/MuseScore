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

#include "dorico/popover/parsers/barsparser.h"

using namespace mu::dorico;
using mu::engraving::BarLineType;
using Kind = BarsEntry::Kind;

namespace {
void expectBars(const std::string& text, Kind kind, int count = 0)
{
    const BarsParse p = parseBars(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, kind) << text;
    EXPECT_EQ(p.entry.count, count) << text;
}

void expectBarline(const std::string& text, BarLineType type)
{
    const BarsParse p = parseBars(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.kind, Kind::Barline) << text;
    EXPECT_EQ(p.entry.barline, type) << text;
}

ParseStatus status(const std::string& text)
{
    return parseBars(text).status;
}
}

TEST(DoricoBarsParser, Bars)
{
    expectBars("+2", Kind::AddBars, 2);
    expectBars("2", Kind::AddBars, 2);
    expectBars("+12", Kind::AddBars, 12);
    expectBars(" +4 ", Kind::AddBars, 4);
    expectBars("-6", Kind::DeleteBars, 6);
    expectBars("rest", Kind::BarRest);
    expectBars("REST", Kind::BarRest);
    expectBars("trim", Kind::Trim);
}

TEST(DoricoBarsParser, BeatsAreUnsupported)
{
    for (const char* t : { "+2q", "2q", "+2-6", "+2 6", "+2/4", "-2q", "-2-6", "-2 6", "-2/4", "+1e.", "+3/16", "-1w" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    for (const char* t : { "+2-", "+2/", "+2/3" }) { // the start of +2-6, +2/4, +2/32
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "+2 1", "+2/5", "+2k", "+2q-" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoBarsParser, Barlines)
{
    for (const char* t : { "|", "single", "normal" }) {
        expectBarline(t, BarLineType::NORMAL);
    }
    for (const char* t : { "||", "double" }) {
        expectBarline(t, BarLineType::DOUBLE);
    }
    for (const char* t : { ":", "dash", "dashed" }) {
        expectBarline(t, BarLineType::BROKEN);
    }
    for (const char* t : { "|]", "fin", "final" }) {
        expectBarline(t, BarLineType::END);
    }
    expectBarline("thick", BarLineType::HEAVY);
    for (const char* t : { "doublethick", "thickdouble" }) {
        expectBarline(t, BarLineType::DOUBLE_HEAVY);
    }
    for (const char* t : { "|:", "start" }) {
        expectBarline(t, BarLineType::START_REPEAT);
    }
    for (const char* t : { ":|", "end" }) {
        expectBarline(t, BarLineType::END_REPEAT);
    }
    for (const char* t : { ":|:", ":||:", "end-start", "endstart" }) {
        expectBarline(t, BarLineType::END_START_REPEAT);
    }
}

TEST(DoricoBarsParser, ProBarlinesAreUnsupported)
{
    for (const char* t : { "|||", "triple", "'", "tick", "tickbottom", ",", "short", "shorttop" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
}

TEST(DoricoBarsParser, IncompleteAndInvalid)
{
    for (const char* t : { "", "+", "-", "r", "tri", "end-", "doub", ":||", "thickd" }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "foo", "+x", "0", "+0", "-0", "1000", "||||", "x" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoBarsParser, PrefillRoundTrips)
{
    for (BarLineType t : { BarLineType::NORMAL, BarLineType::DOUBLE, BarLineType::BROKEN, BarLineType::END, BarLineType::HEAVY,
                           BarLineType::DOUBLE_HEAVY, BarLineType::START_REPEAT, BarLineType::END_REPEAT,
                           BarLineType::END_START_REPEAT }) {
        expectBarline(barlineText(t), t);
    }
    EXPECT_EQ(barlineText(BarLineType::DOTTED), "");
    EXPECT_EQ(barlineText(BarLineType::REVERSE_END), "");
}
