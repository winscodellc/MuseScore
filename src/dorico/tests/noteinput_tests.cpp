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

// Ticks: quarter = 480, eighth grid = 240. Bar 2 of 4/4 starts at 1920.
TEST(DoricoCaretMath, RightStepsByGridInsideARest)
{
    EXPECT_EQ(nextCaretTick(1920, 1920, 240, 3840), 2160); // whole rest: bar start -> first eighth
    EXPECT_EQ(nextCaretTick(2160, 1920, 240, 3840), 2400);
    EXPECT_EQ(nextCaretTick(3600, 1920, 240, 3840), 3840); // last eighth -> next bar
}

TEST(DoricoCaretMath, RightStopsAtTheNextNote)
{
    EXPECT_EQ(nextCaretTick(1920, 1920, 240, 2040), 2040); // a 16th is closer than the grid
    EXPECT_EQ(nextCaretTick(2040, 1920, 240, 2400), 2160); // off-grid caret snaps to the next grid position
}

TEST(DoricoCaretMath, LeftStepsByGridOrToTheNoteStart)
{
    EXPECT_EQ(prevCaretTick(2400, 1920, 240, 1920), 2160);
    EXPECT_EQ(prevCaretTick(2160, 1920, 240, 1920), 1920);
    EXPECT_EQ(prevCaretTick(2100, 1920, 240, 2040), 2040); // note starts after the previous grid position
    EXPECT_EQ(prevCaretTick(2100, 1920, 240, 1920), 1920); // off-grid caret snaps back to the grid
    EXPECT_EQ(prevCaretTick(1920, 0, 240, 1440), 1680);    // from the barline into the previous bar's last rest
}

// Dorico manual, "Respelling notes": G-sharp up -> A-flat -> B-triple-flat; G-sharp down -> F-triple-sharp.
TEST(DoricoRespell, ManualExamples)
{
    using namespace mu::engraving;
    EXPECT_EQ(respellTpc(TPC_G_S, true), TPC_A_B);
    EXPECT_EQ(respellTpc(TPC_A_B, true), TPC_B_BBB);
    EXPECT_EQ(respellTpc(TPC_B_BBB, true), TPC_B_BBB); // C would need a quadruple flat
    EXPECT_EQ(respellTpc(TPC_G_S, false), TPC_F_SSS);
}

TEST(DoricoRespell, WrapsAroundTheOctaveAndHandlesNaturals)
{
    using namespace mu::engraving;
    EXPECT_EQ(respellTpc(TPC_B_S, true), TPC_C);
    EXPECT_EQ(respellTpc(TPC_C, false), TPC_B_S);
    EXPECT_EQ(respellTpc(TPC_C_B, false), TPC_B);
    EXPECT_EQ(respellTpc(TPC_B, true), TPC_C_B);
    EXPECT_EQ(respellTpc(TPC_C, true), TPC_D_BB);
    EXPECT_EQ(respellTpc(TPC_F_SSS, false), TPC_F_SSS); // E would need a quadruple sharp
}

TEST(DoricoVerticalStaff, SkipsHiddenStavesAndStopsAtTheEdges)
{
    const std::vector<bool> visible = { true, false, true, true };
    EXPECT_EQ(verticalStaff(visible, 2, true, false), 0u);  // staff 1 is hidden
    EXPECT_EQ(verticalStaff(visible, 2, false, false), 3u);
    EXPECT_EQ(verticalStaff(visible, 0, true, false), std::nullopt);
    EXPECT_EQ(verticalStaff(visible, 3, false, true), std::nullopt);
    EXPECT_EQ(verticalStaff(visible, 3, true, true), 0u);
    EXPECT_EQ(verticalStaff(visible, 0, false, true), 3u);
}
