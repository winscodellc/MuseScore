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
#include "dorico/popover/popovernavigation.h"
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
        EXPECT_TRUE(!def->glyph.empty() || def->openInline) << command.toString(); // lyrics: no box of its own
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

TEST(DoricoPopoverRegistry, FiguredBassIsNavigating)
{
    const PopoverDef* def = makePopoverRegistry().find("figuredbass");
    ASSERT_NE(def, nullptr);
    EXPECT_TRUE(def->applyAt);
    EXPECT_EQ(def->validate(""), ParseStatus::Incomplete);
    EXPECT_EQ(def->validate("6"), ParseStatus::Ok);
    EXPECT_EQ(def->validate("4->3"), ParseStatus::Unsupported);
    ASSERT_NE(makePopoverRegistry().findByType(ElementType::FIGURED_BASS), nullptr);
}

TEST(DoricoPopoverRegistry, FingeringStepsByChord)
{
    const PopoverDef* def = makePopoverRegistry().find("fingering");
    ASSERT_NE(def, nullptr);
    EXPECT_TRUE(def->stepsByChord);
    EXPECT_TRUE(def->applyAt);
    EXPECT_EQ(def->validate("1,3,5"), ParseStatus::Ok);
    EXPECT_EQ(def->validate("zzz"), ParseStatus::Invalid);
    EXPECT_EQ(makePopoverRegistry().findByType(ElementType::FINGERING)->id, "fingering");
}

TEST(DoricoPopoverRegistry, LyricsOpenInline)
{
    const PopoverDef* def = makePopoverRegistry().find("lyrics");
    ASSERT_NE(def, nullptr);
    EXPECT_TRUE(def->openInline); // the score's lyric editor is the entry field, no box
    EXPECT_TRUE(def->itemTypes.empty()); // Return on a lyric stays MuseScore's edit-text
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

TEST(DoricoPopoverNavigation, BeatsFollowTheTimeSignature)
{
    using mu::engraving::Fraction;
    EXPECT_EQ(beatLength(Fraction(4, 4)), Fraction(1, 4));
    EXPECT_EQ(beatLength(Fraction(2, 2)), Fraction(1, 2));
    EXPECT_EQ(beatLength(Fraction(6, 8)), Fraction(3, 8)); // compound: dotted quarter
    EXPECT_EQ(beatLength(Fraction(3, 8)), Fraction(1, 8));
    EXPECT_EQ(beatStart(Fraction(3, 8), Fraction(4, 4)), Fraction(1, 4)); // inside beat 2
    EXPECT_EQ(beatStart(Fraction(1, 2), Fraction(4, 4)), Fraction(1, 2)); // on beat 3
    EXPECT_EQ(beatStart(Fraction(5, 8), Fraction(6, 8)), Fraction(3, 8));
}
