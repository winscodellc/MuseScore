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

#include "dorico/internal/selectmore.h"

using namespace mu::dorico;

namespace {
// 6 bars of 100 ticks; bars 0-2 are system 1, bars 3-5 system 2
SelectMoreContext score6Bars(bool galley = false)
{
    SelectMoreContext ctx;
    ctx.bar = [](int tick) { return SelectMoreRange { tick / 100 * 100, tick / 100 * 100 + 100 }; };
    ctx.system = [](int tick) { return tick < 300 ? SelectMoreRange { 0, 300 } : SelectMoreRange { 300, 600 }; };
    ctx.flow = { 0, 600 };
    ctx.galley = galley;
    return ctx;
}

SelectMoreItem dyn(int tick, int staff = 0, int voice = 0)
{
    return { SelectKey { 10, staff, voice, 0 }, tick };
}

using Indices = std::vector<size_t>;
}

TEST(DoricoSelectMore, OneItemExpandsToItsBarThenSystemThenFlow)
{
    const std::vector<SelectMoreItem> all = { dyn(110), dyn(150), dyn(0), dyn(250), dyn(320), dyn(550) };
    SelectMoreResult r = selectMore(all, { 0 }, 0, score6Bars());
    EXPECT_EQ(r.stage, 1);
    EXPECT_EQ(r.indices, (Indices { 0, 1 }));
    r = selectMore(all, r.indices, r.stage, score6Bars());
    EXPECT_EQ(r.stage, 2);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 2, 3 }));
    r = selectMore(all, r.indices, r.stage, score6Bars());
    EXPECT_EQ(r.stage, 3);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 2, 3, 4, 5 }));
}

TEST(DoricoSelectMore, AloneInItsBarSkipsToTheSystem)
{
    const std::vector<SelectMoreItem> all = { dyn(110), dyn(0), dyn(250), dyn(320) };
    const SelectMoreResult r = selectMore(all, { 0 }, 0, score6Bars());
    EXPECT_EQ(r.stage, 2);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 2 }));
}

TEST(DoricoSelectMore, TwoItemsTakeEverythingBetween)
{
    const std::vector<SelectMoreItem> all = { dyn(10), dyn(120), dyn(230), dyn(330), dyn(450) };
    const SelectMoreResult r = selectMore(all, { 1, 3 }, 0, score6Bars());
    EXPECT_EQ(r.stage, 1);
    EXPECT_EQ(r.indices, (Indices { 1, 2, 3 }));
}

TEST(DoricoSelectMore, TwoAdjacentItemsSkipToTheSystem)
{
    const std::vector<SelectMoreItem> all = { dyn(10), dyn(120), dyn(230), dyn(330) };
    const SelectMoreResult r = selectMore(all, { 1, 2 }, 0, score6Bars());
    EXPECT_EQ(r.stage, 2);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 2 }));
}

TEST(DoricoSelectMore, OtherVoicesAndStavesAreLeftAlone)
{
    const std::vector<SelectMoreItem> all = { dyn(110), dyn(150, 0, 1), dyn(160, 1, 0), dyn(170) };
    const SelectMoreResult r = selectMore(all, { 0 }, 0, score6Bars());
    EXPECT_EQ(r.indices, (Indices { 0, 3 }));
}

TEST(DoricoSelectMore, EachSelectedTypeExpandsByItself)
{
    const SelectMoreItem note { SelectKey { 20, 0, 0, 0 }, 110 };
    const SelectMoreItem otherNote { SelectKey { 20, 0, 0, 0 }, 190 };
    const SelectMoreItem farNote { SelectKey { 20, 0, 0, 0 }, 210 };
    const std::vector<SelectMoreItem> all = { dyn(120), dyn(180), dyn(220), note, otherNote, farNote };
    // one dynamic in bar 1 and one note in bar 1: both take their bar, the dynamic at 220 stays out
    const SelectMoreResult r = selectMore(all, { 0, 3 }, 0, score6Bars());
    EXPECT_EQ(r.stage, 1);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 3, 4 }));
}

TEST(DoricoSelectMore, GalleySecondPressIsTheWholeFlow)
{
    const std::vector<SelectMoreItem> all = { dyn(110), dyn(150), dyn(250), dyn(550) };
    SelectMoreResult r = selectMore(all, { 0 }, 0, score6Bars(true));
    EXPECT_EQ(r.indices, (Indices { 0, 1 }));
    r = selectMore(all, r.indices, r.stage, score6Bars(true));
    EXPECT_EQ(r.stage, 2);
    EXPECT_EQ(r.indices, (Indices { 0, 1, 2, 3 }));
}

TEST(DoricoSelectMore, LyricsKeepToTheirVerse)
{
    const SelectMoreItem verse1 { SelectKey { 30, 0, 0, 0 }, 110 };
    const SelectMoreItem verse1b { SelectKey { 30, 0, 0, 0 }, 140 };
    const SelectMoreItem verse2 { SelectKey { 30, 0, 0, 2 }, 120 };
    const SelectMoreResult r = selectMore({ verse1, verse1b, verse2 }, { 0 }, 0, score6Bars());
    EXPECT_EQ(r.indices, (Indices { 0, 1 }));
}

TEST(DoricoSelectMore, LastStageStaysPut)
{
    const std::vector<SelectMoreItem> all = { dyn(110), dyn(550) };
    const SelectMoreResult r = selectMore(all, { 0, 1 }, 3, score6Bars());
    EXPECT_EQ(r.stage, 3);
    EXPECT_EQ(r.indices, (Indices { 0, 1 }));
}
