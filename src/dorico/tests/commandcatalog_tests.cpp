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

#include "dorico/internal/jumpbar/commandcatalog.h"

using namespace mu::dorico;

namespace {
const std::vector<CatalogEntry> ENTRIES = {
    { "command://a/remove-rests", "Remove Rests", "Cmd+Backspace", "" },
    { "command://a/select-more", "Select More", "Cmd+Shift+A", "" },
    { "command://a/select-none", "Select None", "", "sn" },
    { "command://a/paste-special", "Paste Special", "", "" },
    { "command://a/move-rests", "Move Rests Up", "", "rr" },
};

std::vector<std::string> titles(const std::vector<CatalogEntry>& entries)
{
    std::vector<std::string> v;
    for (const CatalogEntry& e : entries) {
        v.push_back(e.title);
    }
    return v;
}
}

TEST(DoricoCommandCatalog, PrefixBeforeWordPrefixBeforeSubstring)
{
    const std::vector<CatalogEntry> entries = {
        { "u1", "Unremove Rests", "", "" },     // substring
        { "u2", "Hide Remove Marks", "", "" },  // word prefix
        { "u3", "Remove Rests", "", "" },       // title prefix
    };
    EXPECT_EQ(titles(rankCommands(entries, "remove", {})), (std::vector<std::string> { "Remove Rests", "Hide Remove Marks", "Unremove Rests" }));
}

TEST(DoricoCommandCatalog, AllQueryWordsMustStartSomeTitleWord)
{
    EXPECT_EQ(titles(rankCommands(ENTRIES, "rests rem", {})), (std::vector<std::string> { "Remove Rests" }));
    EXPECT_TRUE(rankCommands(ENTRIES, "remove paste", {}).empty());
}

TEST(DoricoCommandCatalog, ExactAliasComesFirst)
{
    // "rr" also starts the words of "Remove Rests"
    EXPECT_EQ(titles(rankCommands(ENTRIES, "RR", {})).front(), "Move Rests Up");
}

TEST(DoricoCommandCatalog, UsageBreaksTies)
{
    EXPECT_EQ(titles(rankCommands(ENTRIES, "select", {})).front(), "Select More"); // alphabetical
    EXPECT_EQ(titles(rankCommands(ENTRIES, "select", { { "command://a/select-none", 3 } })).front(), "Select None");
}

TEST(DoricoCommandCatalog, EmptyQueryMatchesNothing)
{
    EXPECT_TRUE(rankCommands(ENTRIES, "", {}).empty());
    EXPECT_TRUE(rankCommands(ENTRIES, "  ", {}).empty());
}

TEST(DoricoCommandCatalog, TopUsedIsLimitedAndSkipsUnused)
{
    UsageCounts usage;
    for (int i = 0; i < 3; ++i) {
        usage[ENTRIES[i].uri] = i + 1;
    }
    EXPECT_EQ(titles(topUsed(ENTRIES, usage)), (std::vector<std::string> { "Select None", "Select More", "Remove Rests" }));
    EXPECT_EQ(topUsed(ENTRIES, usage, 2).size(), 2u);
    EXPECT_TRUE(topUsed(ENTRIES, {}).empty());
}

TEST(DoricoCommandCatalog, SplitAlias)
{
    CommandEntryText t = splitAlias("Remove Rests\\=RR ");
    EXPECT_EQ(t.name, "Remove Rests");
    EXPECT_EQ(t.alias, "rr");

    t = splitAlias("Remove Rests");
    EXPECT_EQ(t.name, "Remove Rests");
    EXPECT_FALSE(t.alias.has_value());

    EXPECT_FALSE(splitAlias("Remove Rests\\=  ").alias.has_value());
}
