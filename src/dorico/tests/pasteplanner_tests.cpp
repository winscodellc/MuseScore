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

#include "dorico/internal/pasteplanner.h"

using namespace mu::dorico;
using mu::engraving::Fraction;

namespace {
const Fraction BAR(1, 1);
}

TEST(DoricoPastePlanner, ListNothingSelected)
{
    EXPECT_TRUE(planListPaste({}, 1).empty());
}

TEST(DoricoPastePlanner, ListSingleAnchor)
{
    EXPECT_EQ(planListPaste({ { 2, Fraction(3, 4) } }, 1), (std::vector<PasteTarget> { { 2, Fraction(3, 4) } }));
}

TEST(DoricoPastePlanner, ListOneCopyPerStaffAtTopStaffEarliestTick)
{
    // Staff 1 is selected at 2/1 and 3/1, staff 0 (the top one) at 3/1 and 5/1, staff 3 at 1/1: the top staff's earliest tick wins
    const std::vector<PasteTarget> items = { { 1, Fraction(2, 1) }, { 0, Fraction(5, 1) }, { 3, Fraction(1, 1) },
        { 0, Fraction(3, 1) }, { 1, Fraction(3, 1) } };
    const std::vector<PasteTarget> expected = { { 0, Fraction(3, 1) }, { 1, Fraction(3, 1) }, { 3, Fraction(3, 1) } };
    EXPECT_EQ(planListPaste(items, 1), expected);
}

TEST(DoricoPastePlanner, ListMultiStaffClipboardPastesOnceFromTopStaff)
{
    const std::vector<PasteTarget> items = { { 1, Fraction(2, 1) }, { 2, Fraction(1, 1) } };
    EXPECT_EQ(planListPaste(items, 2), (std::vector<PasteTarget> { { 1, Fraction(2, 1) } }));
}

TEST(DoricoPastePlanner, RangeRepeatsWholeCopiesOnly)
{
    // 1 bar copied, bars 3-6 selected (ticks 2..6): 4 copies
    auto r = planRangePaste(0, 1, Fraction(2, 1), Fraction(6, 1), BAR, 1, true);
    EXPECT_EQ(r.size(), 4u);
    EXPECT_EQ(r.back().tick, Fraction(5, 1));
}

TEST(DoricoPastePlanner, RangeRemainderIsLeftAlone)
{
    // 2 bars copied, 3 bars selected: one copy, the third bar is not touched
    auto r = planRangePaste(0, 1, Fraction(2, 1), Fraction(5, 1), Fraction(2, 1), 1, true);
    EXPECT_EQ(r, (std::vector<PasteTarget> { { 0, Fraction(2, 1) } }));
}

TEST(DoricoPastePlanner, RangeShorterThanClipboardStillPastesOnce)
{
    auto r = planRangePaste(0, 1, Fraction(0, 1), Fraction(1, 1), Fraction(2, 1), 1, true);
    EXPECT_EQ(r.size(), 1u);
}

TEST(DoricoPastePlanner, RangeWithoutFillPastesOnce)
{
    EXPECT_EQ(planRangePaste(0, 1, Fraction(0, 1), Fraction(8, 1), BAR, 1, false).size(), 1u);
}

TEST(DoricoPastePlanner, RangeOnTwoStavesPastesOnBoth)
{
    auto r = planRangePaste(1, 3, Fraction(0, 1), Fraction(2, 1), BAR, 1, true);
    const std::vector<PasteTarget> expected = { { 1, Fraction(0, 1) }, { 2, Fraction(0, 1) }, { 1, Fraction(1, 1) },
        { 2, Fraction(1, 1) } };
    EXPECT_EQ(r, expected);
}

TEST(DoricoPastePlanner, RangeMultiStaffClipboardUsesTopStaffOnly)
{
    auto r = planRangePaste(1, 3, Fraction(0, 1), Fraction(2, 1), BAR, 2, true);
    EXPECT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0].staff, 1u);
    EXPECT_EQ(r[1].staff, 1u);
}
