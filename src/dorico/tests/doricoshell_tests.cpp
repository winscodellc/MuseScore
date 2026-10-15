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
#include "dorico/internal/doricoshell.h"

using namespace mu::dorico;

TEST(DoricoShell, ModeDefaultsToWrite)
{
    DoricoModeState state;
    EXPECT_EQ(state.mode(), Mode::Write);
}

TEST(DoricoShell, ModeChangedFiresOncePerChange)
{
    DoricoModeState state;
    int fired = 0;
    muse::async::Asyncable owner;
    state.modeChanged().onNotify(&owner, [&fired]() { ++fired; });

    state.setMode(Mode::Engrave);
    EXPECT_EQ(state.mode(), Mode::Engrave);
    EXPECT_EQ(fired, 1);

    state.setMode(Mode::Engrave);
    EXPECT_EQ(fired, 1);

    state.setMode(Mode::Print);
    EXPECT_EQ(fired, 2);
}

TEST(DoricoShell, ModeUriRoundTrip)
{
    for (int i = 0; i < MODE_COUNT; ++i) {
        const Mode mode = static_cast<Mode>(i);
        EXPECT_EQ(modeFromUri(modeUri(mode)), mode);
    }
    EXPECT_FALSE(modeFromUri(muse::Uri("musescore://home")).has_value());
    EXPECT_EQ(modeUri(Mode::Write), muse::Uri("musescore://notation"));
}

TEST(DoricoShell, WriteZonesHaveDocks)
{
    for (int i = 0; i < ZONE_COUNT; ++i) {
        EXPECT_FALSE(zoneDocks(Mode::Write, static_cast<Zone>(i)).empty());
    }
    EXPECT_TRUE(zoneDocks(Mode::Print, Zone::Left).empty());
}
