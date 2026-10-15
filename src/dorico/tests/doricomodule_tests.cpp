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

#include <algorithm>

#include "dorico/doricocommands.h"
#include "dorico/internal/doricocommandsregister.h"
#include "dorico/internal/partlayouts.h"

using namespace mu::dorico;

TEST(DoricoModule, CommandListContainsNoop)
{
    DoricoCommandsRegister reg;
    const auto& cmds = reg.commandList();
    EXPECT_NE(std::find(cmds.begin(), cmds.end(), DORICO_NOOP_COMMAND), cmds.end());
}

TEST(DoricoModule, AddDeletedPartLayout)
{
    EXPECT_EQ(addDeletedPartLayout(u"", u"7"), u"7");
    EXPECT_EQ(addDeletedPartLayout(u"7", u"12"), u"7,12");
    EXPECT_EQ(addDeletedPartLayout(u"7,12", u"7"), u"7,12");
    // The filter in MasterNotation::updatePotentialExcerpts reads the value back with split(','):
    EXPECT_TRUE(addDeletedPartLayout(u"7", u"12").split(u',').contains(u"12"));
}
