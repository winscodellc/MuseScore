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

#include "dorico/internal/trackblocks.h"

using namespace mu::dorico;

TEST(DoricoTrackBlocks, ConsecutiveBarsWithNotesMakeOneBlock)
{
    // Bars of 3 quarters: notes, notes, empty, notes
    const std::vector<TrackBlock> blocks = trackBlocks({ { 0, 3, true }, { 3, 6, true }, { 6, 9, false }, { 9, 12, true } });

    ASSERT_EQ(blocks.size(), 2u);
    EXPECT_EQ(blocks[0].start, 0);
    EXPECT_EQ(blocks[0].end, 6);
    EXPECT_EQ(blocks[1].start, 9);
    EXPECT_EQ(blocks[1].end, 12);
}

TEST(DoricoTrackBlocks, NoNotesNoBlocks)
{
    EXPECT_TRUE(trackBlocks({ { 0, 4, false }, { 4, 8, false } }).empty());
}
