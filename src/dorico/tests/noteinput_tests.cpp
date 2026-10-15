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

#include "global/async/asyncable.h"

#include "dorico/internal/doricomodestate.h"
#include "dorico/internal/noteinpututils.h"
#include "engraving/dom/articulation.h"

using namespace mu::dorico;

namespace {
constexpr int C = 0, D = 1, E = 2, G = 4, B = 6;
constexpr int stepOfOctave(int octave, int note) { return octave * 7 + note; }
}

TEST(DoricoStepMath, AboveIsStrictlyAbove)
{
    EXPECT_EQ(stepAbove(stepOfOctave(4, B), C), stepOfOctave(5, C)); // B4 -> C5
    EXPECT_EQ(stepAbove(stepOfOctave(4, C), C), stepOfOctave(5, C)); // same letter: next octave, not the same note
    EXPECT_EQ(stepAbove(stepOfOctave(4, C), D), stepOfOctave(4, D));
    EXPECT_EQ(stepAbove(stepOfOctave(4, G), E), stepOfOctave(5, E));
}

TEST(DoricoStepMath, BelowIsStrictlyBelow)
{
    EXPECT_EQ(stepBelow(stepOfOctave(4, C), C), stepOfOctave(3, C));
    EXPECT_EQ(stepBelow(stepOfOctave(4, B), C), stepOfOctave(4, C)); // B4 -> C4, the nearest C below
    EXPECT_EQ(stepBelow(stepOfOctave(4, C), B), stepOfOctave(3, B));
    EXPECT_EQ(stepBelow(stepOfOctave(4, G), E), stepOfOctave(4, E));
}

TEST(DoricoStepMath, AllLettersStayWithinOneOctaveOfReference)
{
    for (int ref = 14; ref < 60; ++ref) {
        for (int n = 0; n < 7; ++n) {
            const int up = stepAbove(ref, n);
            const int down = stepBelow(ref, n);
            EXPECT_GT(up, ref);
            EXPECT_LE(up, ref + 7);
            EXPECT_EQ(up % 7, n);
            EXPECT_LT(down, ref);
            EXPECT_GE(down, ref - 7);
            EXPECT_EQ(down % 7, n);
        }
    }
}

TEST(DoricoDotTap, SecondQuickTapOfSameDigitDots)
{
    DotTap tap;
    EXPECT_FALSE(tap.press(6, 1000, 400));
    EXPECT_TRUE(tap.press(6, 1300, 400));
}

TEST(DoricoDotTap, DifferentDigitOrSlowTapDoesNotDot)
{
    DotTap tap;
    EXPECT_FALSE(tap.press(6, 1000, 400));
    EXPECT_FALSE(tap.press(5, 1100, 400));
    EXPECT_FALSE(tap.press(5, 1600, 400)); // 500 ms later
}

TEST(DoricoDotTap, ThirdTapStartsOver)
{
    DotTap tap;
    tap.press(6, 1000, 400);
    EXPECT_TRUE(tap.press(6, 1100, 400));
    EXPECT_FALSE(tap.press(6, 1200, 400));
}

TEST(DoricoDotTap, ResetForgetsThePreviousTap)
{
    DotTap tap;
    tap.press(6, 1000, 400);
    tap.reset();
    EXPECT_FALSE(tap.press(6, 1100, 400));
}

TEST(DoricoModeState, ChordAndInsertExcludeEachOther)
{
    DoricoModeState s;
    s.setChordMode(true);
    EXPECT_TRUE(s.isChordMode());
    s.setInsertMode(true);
    EXPECT_TRUE(s.isInsertMode());
    EXPECT_FALSE(s.isChordMode());
    s.setChordMode(true);
    EXPECT_FALSE(s.isInsertMode());
}

TEST(DoricoModeState, NotifiesOncePerRealChange)
{
    DoricoModeState s;
    int count = 0;
    muse::async::Asyncable owner;
    s.modesChanged().onNotify(&owner, [&count]() { ++count; });

    s.setChordMode(false); // no change
    EXPECT_EQ(count, 0);
    s.setChordMode(true);
    EXPECT_EQ(count, 1);
    s.setInsertMode(true); // flips both flags, still one notification
    EXPECT_EQ(count, 2);
    s.setInsertMode(true);
    EXPECT_EQ(count, 2);
}

// The note-input latch (`}` and friends) goes through engraving's updateArticulations, which used to ignore
// every articulation except accent, marcato, tenuto and staccato.
TEST(DoricoArticulationLatch, StaccatissimoTogglesOnAndOff)
{
    using namespace mu::engraving;
    const std::set<SymId> on = updateArticulations({}, SymId::articStaccatissimoAbove, ArticulationsUpdateMode::Remove);
    EXPECT_EQ(on, std::set<SymId>({ SymId::articStaccatissimoAbove }));
    EXPECT_TRUE(updateArticulations(on, SymId::articStaccatissimoAbove, ArticulationsUpdateMode::Remove).empty());
    EXPECT_EQ(updateArticulations(on, SymId::articStaccatissimoAbove, ArticulationsUpdateMode::Insert), on);
}
