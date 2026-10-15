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

#include "dorico/internal/selectionsummary.h"

using namespace mu::dorico;

TEST(DoricoSelectionSummary, NothingSelectedIsEmpty)
{
    EXPECT_TRUE(summarise({}).isEmpty());
}

TEST(DoricoSelectionSummary, SingleNote)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Single;
    i.firstBar = 3;
    i.pitch = "Eb4";
    i.voice = 1;
    EXPECT_EQ(summarise(i), "Bar 3   Eb4   V1");
}

TEST(DoricoSelectionSummary, SingleNonNoteOmitsPitchAndVoice)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Single;
    i.firstBar = 7;
    EXPECT_EQ(summarise(i), "Bar 7");
}

TEST(DoricoSelectionSummary, ChordTextIsShownAsGiven)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Single;
    i.firstBar = 2;
    i.pitch = "C4, E4, G4";
    i.voice = 2;
    EXPECT_EQ(summarise(i), "Bar 2   C4, E4, G4   V2");
}

TEST(DoricoSelectionSummary, RangeShowsBarsAndDuration)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Range;
    i.firstBar = 5;
    i.lastBar = 6;
    i.startSeconds = 8;
    i.endSeconds = 9.5;
    EXPECT_EQ(summarise(i), "Bars 5-6   00:00:08-00:00:10 (1.5\")");
}

TEST(DoricoSelectionSummary, RangeInOneBarSaysBar)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Range;
    i.firstBar = 4;
    i.lastBar = 4;
    i.startSeconds = 3600;
    i.endSeconds = 3602;
    EXPECT_EQ(summarise(i), "Bar 4   01:00:00-01:00:02 (2\")");
}

TEST(DoricoSelectionSummary, MultipleSelection)
{
    SelectionInfo i;
    i.kind = SelectionInfo::Kind::Multiple;
    EXPECT_EQ(summarise(i), "Multiple selection");
}
