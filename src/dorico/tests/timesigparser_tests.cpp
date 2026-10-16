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

#include "dorico/popover/parsers/timesigparser.h"

using namespace mu::dorico;
using mu::engraving::Fraction;
using Kind = TimeSigEntry::Kind;

namespace {
TimeSigEntry ok(const std::string& text)
{
    const TimeSigParse p = parseTimeSig(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

ParseStatus status(const std::string& text)
{
    return parseTimeSig(text).status;
}
}

TEST(DoricoTimeSigParser, Simple)
{
    TimeSigEntry e = ok("3/4");
    EXPECT_EQ(e.kind, Kind::Normal);
    EXPECT_EQ(e.groups, std::vector<int>({ 3 }));
    EXPECT_EQ(e.denominator, 4);
    EXPECT_TRUE(e.pickup.isZero());

    EXPECT_TRUE(ok("6/8").sig().identical(Fraction(6, 8)));
    EXPECT_TRUE(ok("2/2").sig().identical(Fraction(2, 2)));
    EXPECT_TRUE(ok(" 5/4 ").sig().identical(Fraction(5, 4)));
    EXPECT_TRUE(ok("12/16").sig().identical(Fraction(12, 16)));
}

TEST(DoricoTimeSigParser, CommonAndCut)
{
    TimeSigEntry e = ok("c");
    EXPECT_EQ(e.kind, Kind::Common);
    EXPECT_TRUE(e.sig().identical(Fraction(4, 4)));
    for (const char* t : { "cut", "cutc", "\xC2\xA2" }) {
        e = ok(t);
        EXPECT_EQ(e.kind, Kind::Cut) << t;
        EXPECT_TRUE(e.sig().identical(Fraction(2, 2))) << t;
    }
    EXPECT_EQ(status("cu"), ParseStatus::Incomplete);
}

TEST(DoricoTimeSigParser, Additive)
{
    TimeSigEntry e = ok("3+2+2/8");
    EXPECT_EQ(e.groups, std::vector<int>({ 3, 2, 2 }));
    EXPECT_EQ(e.numerator(), 7);
    EXPECT_TRUE(e.groupingShown);

    e = ok("[2+3+2]/8");
    EXPECT_EQ(e.groups, std::vector<int>({ 2, 3, 2 }));
    EXPECT_FALSE(e.groupingShown);
    EXPECT_TRUE(e.sig().identical(Fraction(7, 8)));
}

TEST(DoricoTimeSigParser, OpenAndCompoundAreUnsupported)
{
    for (const char* t : { "x", "X", "open", "6/8 + 3/4", "2/4|6/8", "2/4:6/8", "2/4 (6/8)", "2/4 = 6/8", "2/4 / 6/8",
                           "2/4 \xE2\x80\x93 6/8" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    EXPECT_EQ(status("6/8 +"), ParseStatus::Incomplete);
    EXPECT_EQ(status("6/8 + 3/"), ParseStatus::Incomplete);
    EXPECT_EQ(status("6/8 + abc"), ParseStatus::Invalid);
}

TEST(DoricoTimeSigParser, PickUp)
{
    TimeSigEntry e = ok("4/4,1");
    EXPECT_TRUE(e.sig().identical(Fraction(4, 4)));
    EXPECT_EQ(e.pickup, Fraction(1, 4));
    EXPECT_EQ(ok("4/4,1.5").pickup, Fraction(3, 8));
    EXPECT_EQ(ok("4/4,0.25").pickup, Fraction(1, 16));
    EXPECT_EQ(ok("3/4,0.75").pickup, Fraction(3, 16));
    EXPECT_EQ(ok("6/8,2").pickup, Fraction(2, 8));
    EXPECT_EQ(ok("2/2,0.5").pickup, Fraction(1, 4));
    EXPECT_EQ(ok("c,1").pickup, Fraction(1, 4));

    EXPECT_EQ(status("4/4,"), ParseStatus::Incomplete);
    EXPECT_EQ(status("4/4,1."), ParseStatus::Incomplete);
    EXPECT_EQ(status("4/4,4"), ParseStatus::Invalid);   // a whole bar is no pick-up
    EXPECT_EQ(status("4/4,0"), ParseStatus::Invalid);
    EXPECT_EQ(status("4/4,0.3"), ParseStatus::Invalid); // not a note value
    EXPECT_EQ(status("4/4,a"), ParseStatus::Invalid);
}

TEST(DoricoTimeSigParser, IncompleteAndInvalid)
{
    for (const char* t : { "", "3", "3+", "3/", "[2+3", "[2+3]", "3/3", "3/6", "o", "ope" }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "3/0", "3/5", "0/4", "abc", "3+/8", "+3/4", "100/4", "[2+3/8", "3/4/", "3/,1" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}
