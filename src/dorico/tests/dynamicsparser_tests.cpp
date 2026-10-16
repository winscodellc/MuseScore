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

#include "dorico/popover/parsers/dynamicsparser.h"

using namespace mu::dorico;
using Kind = DynamicItem::Kind;

namespace {
DynamicEntry ok(const std::string& text)
{
    const DynamicsParse p = parseDynamics(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

ParseStatus status(const std::string& text)
{
    return parseDynamics(text).status;
}
}

TEST(DoricoDynamicsParser, ImmediateDynamics)
{
    for (const char* d : { "p", "mf", "mp", "ffffff", "pppppp", "fp", "sfz", "rfz" }) {
        const DynamicEntry e = ok(d);
        ASSERT_EQ(e.items.size(), 1u) << d;
        EXPECT_EQ(e.items[0].kind, Kind::Immediate);
        EXPECT_EQ(e.items[0].dynamic, d);
    }
    EXPECT_EQ(status("fffffff"), ParseStatus::Invalid);
    EXPECT_EQ(status("ppppppp"), ParseStatus::Invalid);
    EXPECT_EQ(status("xyz"), ParseStatus::Invalid);
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("s"), ParseStatus::Incomplete);
}

TEST(DoricoDynamicsParser, CombinedWithSeparator)
{
    for (char sep : { '-', ':', '/' }) {
        const DynamicEntry e = ok(std::string("f") + sep + "p");
        ASSERT_EQ(e.items.size(), 1u);
        EXPECT_EQ(e.items[0].dynamic, "f");
        EXPECT_EQ(e.items[0].second, "p");
        EXPECT_EQ(e.items[0].separator, sep);
    }
    EXPECT_EQ(status("f-"), ParseStatus::Incomplete);
    EXPECT_EQ(status("f-x"), ParseStatus::Invalid);
}

TEST(DoricoDynamicsParser, Parenthesised)
{
    const DynamicEntry e = ok("(f)");
    EXPECT_TRUE(e.parenthesised);
    EXPECT_EQ(e.items[0].dynamic, "f");
    EXPECT_EQ(status("(f"), ParseStatus::Incomplete);
    EXPECT_EQ(status("(x"), ParseStatus::Invalid);
}

TEST(DoricoDynamicsParser, HairpinsAndGradualText)
{
    EXPECT_EQ(ok("<").items[0].kind, Kind::Hairpin);
    EXPECT_TRUE(ok("<").items[0].cresc);
    EXPECT_FALSE(ok(">").items[0].cresc);

    DynamicItem c = ok("cresc").items[0];
    EXPECT_EQ(c.kind, Kind::GradualText);
    EXPECT_EQ(c.text, "cresc.");
    EXPECT_FALSE(c.line);
    EXPECT_TRUE(ok("crescendo...").items[0].line);
    c = ok("decresc...").items[0];
    EXPECT_EQ(c.text, "decresc.");
    EXPECT_FALSE(c.cresc);
    EXPECT_TRUE(c.line);
    EXPECT_EQ(ok("dim").items[0].text, "dim.");
    EXPECT_EQ(status("cres"), ParseStatus::Incomplete);
    EXPECT_EQ(status("cresc.."), ParseStatus::Incomplete);
}

// <> (messa di voce) and < > both become two hairpins: no shared centre handle in MuseScore.
TEST(DoricoDynamicsParser, MessaDiVoceIsTwoHairpins)
{
    for (const char* t : { "<>", "< >" }) {
        const DynamicEntry e = ok(t);
        ASSERT_EQ(e.items.size(), 2u) << t;
        EXPECT_TRUE(e.items[0].cresc);
        EXPECT_FALSE(e.items[1].cresc);
    }
}

TEST(DoricoDynamicsParser, Niente)
{
    EXPECT_EQ(ok("o<").items[0].niente, DynamicItem::Niente::Circle);
    EXPECT_EQ(ok(">o").items[0].niente, DynamicItem::Niente::Circle);
    EXPECT_EQ(ok("n<").items[0].niente, DynamicItem::Niente::Letter);
    EXPECT_EQ(ok(">n").items[0].niente, DynamicItem::Niente::Letter);
    EXPECT_EQ(ok(">n").items.size(), 1u);
    EXPECT_EQ(status("o"), ParseStatus::Incomplete);
}

TEST(DoricoDynamicsParser, Modifiers)
{
    DynamicItem d = ok("f molto").items[0];
    EXPECT_EQ(d.modifier, "molto");
    EXPECT_TRUE(d.modifierBefore);
    d = ok("p sub.").items[0];
    EXPECT_EQ(d.modifier, "subito");
    EXPECT_FALSE(d.modifierBefore);
    EXPECT_EQ(ok("p piu").items[0].modifier, "più");
    d = ok("p espressivo e dolce").items[0];
    EXPECT_EQ(d.modifier, "espressivo e dolce"); // free text, after the dynamic
    EXPECT_FALSE(d.modifierBefore);
    EXPECT_EQ(ok("f mo").items[0].modifier, "mo"); // decided: an unknown word is free text
    EXPECT_EQ(status("molto"), ParseStatus::Invalid); // a modifier needs an immediate dynamic
    EXPECT_EQ(status("fmolto"), ParseStatus::Invalid);
}

TEST(DoricoDynamicsParser, PhrasesAndOpenEnded)
{
    const DynamicEntry e = ok("p<f>p");
    ASSERT_EQ(e.items.size(), 5u);
    EXPECT_EQ(e.items[0].dynamic, "p");
    EXPECT_EQ(e.items[1].kind, Kind::Hairpin);
    EXPECT_TRUE(e.items[1].cresc);
    EXPECT_EQ(e.items[2].dynamic, "f");
    EXPECT_FALSE(e.items[3].cresc);
    EXPECT_EQ(e.items[4].dynamic, "p");
    EXPECT_FALSE(e.openEnded());

    EXPECT_TRUE(ok("p<").openEnded());
    EXPECT_TRUE(ok("f>").openEnded());
    EXPECT_TRUE(ok("p cresc...").openEnded());
    EXPECT_EQ(status("p f"), ParseStatus::Invalid); // two immediate dynamics need a gradual one between them
}

TEST(DoricoDynamicsParser, Stop)
{
    EXPECT_TRUE(ok("?").stop);
}
