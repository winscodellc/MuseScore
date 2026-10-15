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

#include "dorico/internal/doricomenu.h"

using namespace mu::dorico;

TEST(DoricoMenu, TenTopLevelMenusInDoricoOrder)
{
    QStringList titles;
    for (const MenuSpec& menu : doricoMenus()) {
        EXPECT_TRUE(menu.isSubmenu()) << menu.title.toStdString();
        titles << menu.title;
    }
    EXPECT_EQ(titles, QStringList({ "File", "Edit", "Setup", "Write", "Engrave", "Play", "Library", "View", "Window", "Help" }));
}

TEST(DoricoMenu, KeepsMuseScoreRefreshedSubmenus)
{
    // AppMenuModel refreshes these by id (findMenu), so they must be moved, not rebuilt.
    const QStringList placed = placedItemIds(doricoMenus());
    EXPECT_TRUE(placed.contains("menu-file-open"));
    EXPECT_TRUE(placed.contains("menu-extensions"));
    EXPECT_TRUE(placed.contains("menu-workspaces"));
    EXPECT_TRUE(placed.contains("command://notation/undo"));
}

TEST(DoricoMenu, WindowHasTheFiveModes)
{
    const MenuSpec& window = doricoMenus()[8];
    QStringList ids;
    for (const MenuSpec& child : window.children) {
        ids << child.id;
    }
    for (const char* mode : { "setup", "write", "engrave", "play", "print" }) {
        EXPECT_TRUE(ids.contains(QString("command://dorico/mode-") + mode)) << mode;
    }
}
