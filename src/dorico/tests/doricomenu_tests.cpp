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

#include <functional>
#include <set>

#include <gtest/gtest.h>

#include "dorico/doricocommands.h"
#include "dorico/internal/doricomenu.h"

using namespace mu::dorico;

namespace {
const Mode ALL_MODES[] = { Mode::Setup, Mode::Write, Mode::Engrave, Mode::Play, Mode::Print };

//! The fixed menus plus every mode's menu, as the arranger sees them
std::vector<MenuSpec> everyMenu()
{
    std::vector<MenuSpec> all = doricoMenus();
    for (Mode mode : ALL_MODES) {
        if (const MenuSpec* menu = modeMenu(mode)) {
            all.push_back(*menu);
        }
    }
    return all;
}

void forEachSpec(const std::vector<MenuSpec>& specs, const std::function<void(const MenuSpec&)>& visit)
{
    for (const MenuSpec& spec : specs) {
        visit(spec);
        forEachSpec(spec.children, visit);
    }
}

const MenuSpec* find(const std::vector<MenuSpec>& specs, const QString& id)
{
    const MenuSpec* found = nullptr;
    forEachSpec(specs, [&](const MenuSpec& spec) {
        if (!found && spec.id == id) {
            found = &spec;
        }
    });
    return found;
}
}

TEST(DoricoMenu, NineTopLevelMenusInDoricoOrder)
{
    QStringList titles;
    for (const MenuSpec& menu : doricoMenus()) {
        titles << menu.title;
    }
    // the mode menu's slot is titled for the default mode and swapped by the arranger
    EXPECT_EQ(titles, QStringList({ "File", "Edit", "View", "Write", "Play", "Library", "Window", "Help", "Script" }));
    EXPECT_EQ(doricoMenus()[3].id, MODE_MENU_ID);
}

TEST(DoricoMenu, SetupWriteAndEngraveHaveAMenuPlayAndPrintDoNot)
{
    // captured from Dorico Pro 6: Play mode shows only the Play menu, Print mode no mode menu at all
    ASSERT_NE(modeMenu(Mode::Setup), nullptr);
    ASSERT_NE(modeMenu(Mode::Write), nullptr);
    ASSERT_NE(modeMenu(Mode::Engrave), nullptr);
    EXPECT_EQ(modeMenu(Mode::Setup)->title, "Setup");
    EXPECT_EQ(modeMenu(Mode::Write)->title, "Write");
    EXPECT_EQ(modeMenu(Mode::Engrave)->title, "Engrave");
    EXPECT_EQ(modeMenu(Mode::Play), nullptr);
    EXPECT_EQ(modeMenu(Mode::Print), nullptr);
}

TEST(DoricoMenu, KeepsMuseScoreRefreshedSubmenus)
{
    // AppMenuModel refreshes these by id (findMenu), so they must be moved, not rebuilt.
    const QStringList placed = placedItemIds(everyMenu());
    EXPECT_TRUE(placed.contains("menu-file-open"));
    EXPECT_TRUE(placed.contains("command://notation/undo"));
}

TEST(DoricoMenu, WindowHasTheFiveModes)
{
    const MenuSpec* window = find(doricoMenus(), "dorico-menu-window");
    ASSERT_NE(window, nullptr);
    QStringList ids;
    for (const MenuSpec& child : window->children) {
        ids << child.id;
    }
    for (const char* mode : { "setup", "write", "engrave", "play", "print" }) {
        EXPECT_TRUE(ids.contains(QString("command://dorico/mode-") + mode)) << mode;
    }
}

TEST(DoricoMenu, WriteHasTheTransformSubmenu)
{
    // Spec 65: Pitches, Rhythm (2) and All (3), every command wired
    const std::vector<MenuSpec> write = { *modeMenu(Mode::Write) };
    const MenuSpec* transform = find(write, "dorico-menu-transform");
    ASSERT_NE(transform, nullptr);
    ASSERT_EQ(transform->children.size(), 3u);
    EXPECT_EQ(transform->children[0].title, "Pitches");
    EXPECT_EQ(transform->children[1].title, "Rhythm");
    EXPECT_EQ(transform->children[2].title, "All");
    EXPECT_EQ(transform->children[1].children.size(), 2u);
    EXPECT_EQ(transform->children[2].children.size(), 3u);

    const QStringList placed = placedItemIds(everyMenu());
    for (const auto& command : DORICO_TRANSFORM_COMMANDS) {
        EXPECT_TRUE(placed.contains(QString::fromStdString(command.toString()))) << command.toString();
    }
}

TEST(DoricoMenu, SubmenusAndStubsAreNotDuplicated)
{
    std::set<QString> seen;
    forEachSpec(everyMenu(), [&](const MenuSpec& spec) {
        if (spec.isSeparator() || spec.id.startsWith("command://") || spec.id.startsWith("menu-") || spec.id.startsWith("dorico-ctx-")) {
            return;   // commands may be placed twice (Export's three rows), MuseScore's ids are moved once, ctx ids repeat across submenus
        }
        EXPECT_TRUE(seen.insert(spec.id).second) << spec.id.toStdString();
    });
}

TEST(DoricoMenu, LabelsAreDoricoStyle)
{
    forEachSpec(everyMenu(), [](const MenuSpec& spec) {
        const std::string title = spec.title.toStdString();
        EXPECT_NE(title.rfind("Dorico:", 0), 0u) << title;
        EXPECT_FALSE(spec.title.contains("(⌘") || spec.title.contains("⇧") || spec.title.contains("⌫")) << title;
        EXPECT_FALSE(spec.title.endsWith("…")) << title;   // three dots, as Dorico shows
        if (spec.id.startsWith("command://dorico/")) {
            EXPECT_FALSE(spec.title.isEmpty()) << spec.id.toStdString();   // the registered title is "Dorico: ..."
        }
        if (isStubId(spec.id)) {
            EXPECT_FALSE(spec.title.isEmpty()) << spec.id.toStdString();
        }
    });
}

TEST(DoricoMenu, NoDuplicateMeasuresMenu)
{
    int measures = 0;
    forEachSpec(everyMenu(), [&](const MenuSpec& spec) {
        measures += spec.id == "menu-measures";
    });
    EXPECT_EQ(measures, 0);
}
