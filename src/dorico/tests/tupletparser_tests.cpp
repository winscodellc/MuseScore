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

#include "dorico/popover/parsers/tupletparser.h"

using namespace mu::dorico;
using mu::engraving::DurationType;

namespace {
void expectTuplet(const std::string& text, int actual, int normal)
{
    const TupletParse p = parseTuplet(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.actual, actual) << text;
    EXPECT_EQ(p.entry.normal, normal) << text;
    EXPECT_FALSE(p.entry.unit.has_value()) << text;
}

void expectUnit(const std::string& text, int actual, int normal, DurationType type, int dots)
{
    const TupletParse p = parseTuplet(text);
    ASSERT_EQ(p.status, ParseStatus::Ok) << text;
    EXPECT_EQ(p.entry.actual, actual) << text;
    EXPECT_EQ(p.entry.normal, normal) << text;
    ASSERT_TRUE(p.entry.unit.has_value()) << text;
    EXPECT_EQ(p.entry.unit->type, type) << text;
    EXPECT_EQ(p.entry.unit->dots, dots) << text;
}
}

TEST(TupletParser, Ratios)
{
    expectTuplet("3", 3, 2);
    expectTuplet("3:2", 3, 2);
    expectTuplet("3/2", 3, 2);
    expectTuplet("3 2", 3, 2);
    expectTuplet(" 3:2 ", 3, 2);
    expectTuplet("3:4", 3, 4);
    expectTuplet("5:6", 5, 6);
    expectTuplet("9/8", 9, 8);
    expectTuplet("2 3", 2, 3);
}

TEST(TupletParser, DefaultNormal)
{
    expectTuplet("2", 2, 3);
    expectTuplet("4", 4, 3);
    expectTuplet("5", 5, 4);
    expectTuplet("6", 6, 4);
    expectTuplet("7", 7, 4);
    expectTuplet("8", 8, 6);
    expectTuplet("9", 9, 8);
    expectTuplet("11", 11, 8);
    expectTuplet("17", 17, 16);
}

TEST(TupletParser, BeatUnits)
{
    expectUnit("3:2e", 3, 2, DurationType::V_EIGHTH, 0);
    expectUnit("5:4x", 5, 4, DurationType::V_16TH, 0);
    expectUnit("3e", 3, 2, DurationType::V_EIGHTH, 0);
    expectUnit("3:2 e", 3, 2, DurationType::V_EIGHTH, 0);
    expectUnit("5:4q.", 5, 4, DurationType::V_QUARTER, 1);
    expectUnit("5/4q.", 5, 4, DurationType::V_QUARTER, 1);
    expectUnit("5 4q.", 5, 4, DurationType::V_QUARTER, 1);
    expectUnit("5:4-6.", 5, 4, DurationType::V_QUARTER, 1);
    expectUnit("5:4 6.", 5, 4, DurationType::V_QUARTER, 1);
    expectUnit("3:2 2w", 3, 2, DurationType::V_BREVE, 0);
    expectUnit("3:2-9", 3, 2, DurationType::V_BREVE, 0);
}

TEST(TupletParser, Incomplete)
{
    EXPECT_EQ(parseTuplet("").status, ParseStatus::Incomplete);
    EXPECT_EQ(parseTuplet("3:").status, ParseStatus::Incomplete);
    EXPECT_EQ(parseTuplet("5:4-").status, ParseStatus::Incomplete);
}

TEST(TupletParser, Invalid)
{
    for (const char* text : { "abc", "x", "0:2", "3:0", "0", ":2", "3:2k", "3:2.", "3:2-6x", "3:2-1", "1234:2" }) {
        EXPECT_EQ(parseTuplet(text).status, ParseStatus::Invalid) << text;
    }
}

TEST(TupletParser, Text)
{
    EXPECT_EQ(tupletText({ 3, 2, std::nullopt }), "3:2");
    EXPECT_EQ(tupletText({ 5, 4, BeatUnit { DurationType::V_QUARTER, 1 } }), "5:4q.");
}
