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

#include "dorico/popover/parsers/techniquesparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using PedalAction = TechniquesEntry::Pedal;

namespace {
TechniquesEntry ok(const std::string& text)
{
    const TechniquesParse p = parseTechniques(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

ParseStatus status(const std::string& text)
{
    return parseTechniques(text).status;
}

void expectOne(const std::string& text, const std::string& entry)
{
    const TechniquesEntry e = ok(text);
    ASSERT_EQ(e.items.size(), 1u) << text;
    ASSERT_NE(e.items[0].technique, nullptr) << text;
    EXPECT_EQ(e.items[0].technique->entry, entry) << text;
    EXPECT_FALSE(e.withDuration) << text;
}
}

TEST(DoricoTechniquesParser, SingleTechniques)
{
    for (const char* t : { "pizz", "arco", "spicc", "nat", "con sord", "senza sord", "sul pont", "sul tasto", "pst",
                           "senza vibrato", "vibrato", "downbow", "upbow", "o", "+", "stopped" }) {
        expectOne(t, t);
    }
    expectOne("  PIZZ ", "pizz");
    EXPECT_EQ(ok("pizz").items[0].technique->type, PlayingTechniqueType::Pizzicato);
    EXPECT_EQ(ok("upbow").items[0].technique->sym, SymId::stringsUpBow);
}

TEST(DoricoTechniquesParser, BracketsDurationAndGroups)
{
    TechniquesEntry e = ok("vibrato->");
    ASSERT_EQ(e.items.size(), 1u);
    EXPECT_TRUE(e.withDuration);

    e = ok("(arco)");
    ASSERT_EQ(e.items.size(), 1u);
    EXPECT_TRUE(e.items[0].parenthesised);
    EXPECT_FALSE(e.items[0].hidden);

    e = ok("[pizz]");
    ASSERT_EQ(e.items.size(), 1u);
    EXPECT_TRUE(e.items[0].hidden);

    e = ok("pizz->arco");
    ASSERT_EQ(e.items.size(), 2u);
    EXPECT_EQ(e.items[1].technique->entry, "arco");
    EXPECT_FALSE(e.withDuration);

    EXPECT_TRUE(ok("?").stop);
    EXPECT_EQ(status("pizz->arco->"), ParseStatus::Unsupported);
    EXPECT_EQ(status("pizz-"), ParseStatus::Incomplete);
    EXPECT_EQ(status("(arc"), ParseStatus::Incomplete);
    EXPECT_EQ(status("("), ParseStatus::Incomplete);
    EXPECT_EQ(status("(piz)"), ParseStatus::Invalid);
    EXPECT_EQ(status("->pizz"), ParseStatus::Invalid);
    EXPECT_EQ(status("piz->arco"), ParseStatus::Invalid);
}

TEST(DoricoTechniquesParser, Pedals)
{
    EXPECT_EQ(ok("ped").pedal, PedalAction::Start);
    EXPECT_EQ(ok("*").pedal, PedalAction::Stop);
    for (const char* t : { "^", "notch", "retake" }) {
        EXPECT_EQ(ok(t).pedal, PedalAction::Retake) << t;
    }
    EXPECT_EQ(ok("nonotch").pedal, PedalAction::RemoveRetake);
    for (const char* t : { "1/4", "1/2", "3/4", "1", "sost", "s*", "unacorda", "u*" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    EXPECT_EQ(status("1/"), ParseStatus::Incomplete);
    EXPECT_EQ(status("unac"), ParseStatus::Incomplete);
}

TEST(DoricoTechniquesParser, UnsupportedEntries)
{
    EXPECT_EQ(status("string 3"), ParseStatus::Unsupported);
    EXPECT_EQ(status("string"), ParseStatus::Incomplete);
    EXPECT_EQ(status("string 0"), ParseStatus::Invalid);
    EXPECT_EQ(status("strong air pressure"), ParseStatus::Ok);

    EXPECT_EQ(status("harm nat n1 s1 p4 diamond"), ParseStatus::Unsupported);
    EXPECT_EQ(status("harmonic art p=4 s=D4 paren"), ParseStatus::Unsupported);
    EXPECT_EQ(status("harm"), ParseStatus::Unsupported);
    EXPECT_EQ(status("har"), ParseStatus::Incomplete);
    EXPECT_EQ(status("harm dia"), ParseStatus::Incomplete);
    EXPECT_EQ(status("harm p="), ParseStatus::Incomplete);
    EXPECT_EQ(status("harm xyz nat"), ParseStatus::Invalid);

    for (const char* t : { "--^|^---", "--^^---", "BbEb", "DCBbEbFGA" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
    EXPECT_EQ(status("--^"), ParseStatus::Incomplete);
    EXPECT_EQ(status("--^^----"), ParseStatus::Invalid);
}

TEST(DoricoTechniquesParser, IncompleteAndInvalid)
{
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("piz"), ParseStatus::Incomplete);
    EXPECT_EQ(status("con"), ParseStatus::Incomplete);
    EXPECT_EQ(status("xyzzy"), ParseStatus::Invalid);
}

TEST(DoricoTechniquesParser, Suggestions)
{
    const std::vector<std::string> piz = techniqueSuggestions("piz");
    ASSERT_FALSE(piz.empty());
    EXPECT_EQ(piz.front(), "pizz");
    EXPECT_EQ(techniqueSuggestions("(ar"), std::vector<std::string> { "(arco)" });
    EXPECT_EQ(techniqueSuggestions("pizz->ar"), std::vector<std::string> { "pizz->arco" });
    EXPECT_EQ(techniqueSuggestions("pe"), std::vector<std::string> { "ped" });
    EXPECT_TRUE(techniqueSuggestions("").empty());
    EXPECT_TRUE(techniqueSuggestions("pizz").empty());
    EXPECT_LE(techniqueSuggestions("s").size(), 8u);
}

TEST(DoricoTechniquesParser, Prefill)
{
    EXPECT_EQ(techniqueEntryForText("pizz."), "pizz");
    EXPECT_EQ(techniqueEntryForText("(arco)"), "(arco)");
    EXPECT_EQ(techniqueEntryForText("poco sul tasto"), "pst");
    EXPECT_EQ(techniqueEntryForText("hello"), "");
    EXPECT_EQ(techniqueEntryForSymbol(SymId::stringsDownBow), "downbow");
    EXPECT_EQ(techniqueEntryForSymbol(SymId::doubleTongueBelow), "double-tongue");
    EXPECT_EQ(techniqueEntryForSymbol(SymId::articAccentAbove), "");
}
