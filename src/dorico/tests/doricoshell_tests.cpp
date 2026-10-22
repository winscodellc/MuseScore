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


#include <set>

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

TEST(DoricoShell, WriteZonesHaveDocks)
{
    for (int i = 0; i < ZONE_COUNT; ++i) {
        EXPECT_FALSE(zoneDocks(Mode::Write, static_cast<Zone>(i)).empty());
    }
}

// One page for all modes (spec 95): each zone is the same dock in every mode that has it.
TEST(DoricoShell, ZonesAreSharedDocks)
{
    for (int i = 0; i < MODE_COUNT; ++i) {
        const Mode mode = static_cast<Mode>(i);
        EXPECT_EQ(zoneDocks(mode, Zone::Left).front(), QString(LEFT_ZONE_PANEL));
        if (mode != Mode::Play) {
            EXPECT_EQ(zoneDocks(mode, Zone::Right).front(), QString(RIGHT_ZONE_PANEL));
        }
        if (mode != Mode::Print) {
            EXPECT_EQ(zoneDocks(mode, Zone::Lower), std::vector<QString>({ LOWER_ZONE_PANEL }));
        }
    }
}

TEST(DoricoShell, MuseScorePanelsOnlyInWrite)
{
    EXPECT_EQ(zoneDocks(Mode::Write, Zone::Right),
              std::vector<QString>({ RIGHT_ZONE_PANEL, "palettesPanel", "selectionFiltersPanel", "undoHistoryPanel" }));
    EXPECT_EQ(zoneDocks(Mode::Setup, Zone::Right).size(), 1u);
}

TEST(DoricoShell, PlayHasNoRightZoneAndPrintNoLowerZone)
{
    EXPECT_TRUE(zoneDocks(Mode::Play, Zone::Right).empty());
    EXPECT_TRUE(zoneDocks(Mode::Print, Zone::Lower).empty());
}

TEST(DoricoShell, Toolboxes)
{
    EXPECT_EQ(toolboxDocks(Mode::Write), std::vector<QString>({ "doricoLeftToolbox", "doricoRightToolbox" }));
    EXPECT_EQ(toolboxDocks(Mode::Engrave), std::vector<QString>({ "doricoLeftToolbox" }));
    EXPECT_TRUE(toolboxDocks(Mode::Setup).empty());
    EXPECT_TRUE(toolboxDocks(Mode::Play).empty());
    EXPECT_TRUE(toolboxDocks(Mode::Print).empty());
}

TEST(DoricoShell, DefaultZones)
{
    for (int z = 0; z < ZONE_COUNT; ++z) {
        EXPECT_TRUE(zoneOpenByDefault(Mode::Setup, static_cast<Zone>(z)));
    }
    EXPECT_TRUE(zoneOpenByDefault(Mode::Write, Zone::Left));
    EXPECT_FALSE(zoneOpenByDefault(Mode::Write, Zone::Right));
    EXPECT_FALSE(zoneOpenByDefault(Mode::Write, Zone::Lower));
    EXPECT_TRUE(zoneOpenByDefault(Mode::Print, Zone::Right));
    EXPECT_EQ(zoneDefaultSize(Mode::Write, Zone::Left), 92);
    EXPECT_EQ(zoneDefaultSize(Mode::Write, Zone::Right), 230);
    EXPECT_EQ(zoneDefaultSize(Mode::Setup, Zone::Lower), 130);
}

TEST(DoricoShell, ExportUnitType)
{
    using UnitType = mu::project::INotationWriter::UnitType;
    EXPECT_EQ(exportUnitType("pdf", true), UnitType::PER_PART);
    EXPECT_EQ(exportUnitType("pdf", false), UnitType::MULTI_PART);
    EXPECT_EQ(exportUnitType("png", false), UnitType::PER_PAGE);
    EXPECT_EQ(exportUnitType("svg", true), UnitType::PER_PAGE);
}

TEST(DoricoShell, LayoutPageInfo)
{
    // A4 portrait, 1.75 mm staff space = 4.96 pt.
    EXPECT_EQ(layoutPageInfo(8.27, 11.69, 4.96), QString("210 x 297 mm, portrait, 5.0 pt"));
    EXPECT_EQ(layoutPageInfo(11.69, 8.27, 7.0), QString("297 x 210 mm, landscape, 7.0 pt"));
}

TEST(DoricoShell, LowerZoneIsOneDock)
{
    const std::vector<QString> docks = zoneDocks(Mode::Write, Zone::Lower);
    ASSERT_EQ(docks.size(), 1u);
    EXPECT_EQ(docks.front(), QString(LOWER_ZONE_PANEL));
}

TEST(DoricoShell, ToolbarDocks)
{
    EXPECT_EQ(toolbarDocks(), std::vector<QString>({ "doricoTopBar", "playbackToolBar", "undoRedoToolBar" }));
}

TEST(DoricoShell, LowerZoneTabDefaultsAndNotifiesOncePerChange)
{
    DoricoModeState state;
    EXPECT_EQ(state.lowerZoneTab(), QString("properties"));

    int fired = 0;
    muse::async::Asyncable owner;
    state.lowerZoneTabChanged().onNotify(&owner, [&fired]() { ++fired; });

    state.setLowerZoneTab("mixer");
    state.setLowerZoneTab("mixer");
    EXPECT_EQ(state.lowerZoneTab(), QString("mixer"));
    EXPECT_EQ(fired, 1);
}

TEST(DoricoShell, MuseScorePanelTogglesMapToLowerZoneTabs)
{
    EXPECT_EQ(lowerZoneTabForCommand("command://app/dock/toggle-properties"), QString("properties"));
    EXPECT_EQ(lowerZoneTabForCommand("command://app/dock/toggle-piano-keyboard"), QString("keyboard"));
    EXPECT_EQ(lowerZoneTabForCommand("command://app/dock/toggle-percussion"), QString("drumPads"));
    EXPECT_EQ(lowerZoneTabForCommand("command://app/dock/toggle-mixer"), QString("mixer"));
    EXPECT_EQ(lowerZoneTabForCommand("command://app/dock/toggle-timeline"), QString("timeline"));
    EXPECT_FALSE(lowerZoneTabForCommand("command://app/dock/toggle-palettes").has_value());
}
