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

#include "dorico/internal/jumpbar/gotoparser.h"

using namespace mu::dorico;

TEST(DoricoGoToParser, Bars)
{
    GoToTarget t = parseGoTo("b32");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.bar, 32);
    EXPECT_FALSE(t.barRelative);

    t = parseGoTo(" B 32 ");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.bar, 32);

    t = parseGoTo("b+4");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.bar, 4);
    EXPECT_TRUE(t.barRelative);
}

TEST(DoricoGoToParser, Pages)
{
    GoToTarget t = parseGoTo("p6");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.page, 6);
    EXPECT_FALSE(t.pageRelative);

    t = parseGoTo("p+6");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.page, 6);
    EXPECT_TRUE(t.pageRelative);
}

TEST(DoricoGoToParser, RehearsalMarks)
{
    for (const char* text : { "rK", "rk", "r K" }) {
        const GoToTarget t = parseGoTo(text);
        ASSERT_TRUE(t.valid) << text;
        EXPECT_EQ(t.rehearsal, "k") << text;
    }
}

TEST(DoricoGoToParser, Flows)
{
    GoToTarget t = parseGoTo("f1b20");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.bar, 20);
    EXPECT_FALSE(t.barRelative);

    t = parseGoTo("f1");
    ASSERT_TRUE(t.valid);
    EXPECT_EQ(t.bar, 1);
}

TEST(DoricoGoToParser, Invalid)
{
    // one flow only: f2, fn and fp cannot go anywhere
    for (const char* text : { "", " ", "b", "bx", "b0", "p", "p0", "r", "f", "f2", "f2b3", "fn", "fp", "f1b", "f1b+3", "x5", "b3x",
                              "b-4", "b99999999999" }) {
        EXPECT_FALSE(parseGoTo(text).valid) << text;
    }
}
