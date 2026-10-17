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

#include "dorico/doricocommands.h"
#include "dorico/popover/popoverregistry.h"

using namespace mu::dorico;
using mu::engraving::ElementType;

TEST(DoricoPopoverRegistry, EveryPopoverCommandHasADef)
{
    const PopoverRegistry registry = makePopoverRegistry();
    for (const muse::rcommand::Command& command : DORICO_POPOVER_COMMANDS) {
        const PopoverDef* def = registry.find(command.pathSegments().back());
        ASSERT_NE(def, nullptr) << command.toString();
        EXPECT_FALSE(def->title.empty());
        EXPECT_NE(def->icon, muse::ui::IconCode::Code::NONE);
    }
}

TEST(DoricoPopoverRegistry, DuplicateIdIsRejected)
{
    PopoverRegistry registry;
    PopoverDef def;
    def.id = "x";
    EXPECT_TRUE(registry.add(def));
    EXPECT_FALSE(registry.add(def));
    EXPECT_EQ(registry.find("y"), nullptr);
}

TEST(DoricoPopoverRegistry, FindByItemType)
{
    const PopoverRegistry registry = makePopoverRegistry();
    ASSERT_NE(registry.findByType(ElementType::DYNAMIC), nullptr);
    EXPECT_EQ(registry.findByType(ElementType::DYNAMIC)->id, "dynamics");
    EXPECT_EQ(registry.findByType(ElementType::NOTE), nullptr);
}

TEST(DoricoPopoverRegistry, PlaceholderIsUnsupported)
{
    const PopoverDef* def = makePopoverRegistry().find("tuplets");
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->validate(""), ParseStatus::Incomplete);
    EXPECT_EQ(def->validate("G"), ParseStatus::Unsupported);
}

TEST(DoricoPopoverRegistry, DynamicsValidate)
{
    const PopoverRegistry registry = makePopoverRegistry();
    const PopoverDef* def = registry.find("dynamics");
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->validate(""), ParseStatus::Incomplete);
    EXPECT_EQ(def->validate("p"), ParseStatus::Ok);
    EXPECT_EQ(def->validate("mf"), ParseStatus::Ok);
    EXPECT_EQ(def->validate("sfz"), ParseStatus::Ok);
    EXPECT_EQ(def->validate("m"), ParseStatus::Incomplete); // start of mp / mf
    EXPECT_TRUE(def->supportsLocal);
    EXPECT_EQ(registry.findByType(ElementType::HAIRPIN_SEGMENT)->id, "dynamics");
    EXPECT_EQ(def->validate("x"), ParseStatus::Invalid);
}
