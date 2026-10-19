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

#include "dorico/popover/parsers/fingeringparser.h"

using namespace mu::dorico;
using mu::engraving::TextStyleType;

namespace {
void expectItems(const std::string& text, bool fretted, std::vector<FingeringItem> expected)
{
    const FingeringParse p = parseFingering(text, fretted);
    ASSERT_EQ(p.status, ParseStatus::Ok) << text;
    ASSERT_EQ(p.items.size(), expected.size()) << text;
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(p.items[i].text, expected[i].text) << text;
        EXPECT_EQ(p.items[i].style, expected[i].style) << text;
    }
}

ParseStatus status(const std::string& text, bool fretted = false)
{
    return parseFingering(text, fretted).status;
}

const auto F = TextStyleType::FINGERING;
const auto LH = TextStyleType::LH_GUITAR_FINGERING;
const auto RH = TextStyleType::RH_GUITAR_FINGERING;
}

TEST(DoricoFingeringParser, NonFretted)
{
    expectItems("1", false, { { "1", F } });
    expectItems("12", false, { { "12", F } });
    expectItems("1,3,5", false, { { "1", F }, { "3", F }, { "5", F } });
    expectItems("L2", false, { { "2", LH } });
    expectItems("R5", false, { { "5", RH } });
    expectItems("T", false, { { "T", F } });
    expectItems("2(3)", false, { { "2(3)", F } });
    expectItems("[4]", false, { { "[4]", F } });
    expectItems("1-3", false, { { "1-3", F } });
    expectItems("2343", false, { { "2343", F } });
    expectItems("2/", false, { { "2/", F } });
}

TEST(DoricoFingeringParser, Fretted)
{
    expectItems("p", true, { { "p", RH } });
    expectItems("i,m,a", true, { { "i", RH }, { "m", RH }, { "a", RH } });
    expectItems("3", true, { { "3", LH } });
    expectItems("t", true, { { "t", LH } });
}

TEST(DoricoFingeringParser, LettersDoNotDependOnFretted)
{
    expectItems("p", false, { { "p", RH } });
    EXPECT_EQ(status("zzz", true), ParseStatus::Invalid);
}

TEST(DoricoFingeringParser, Statuses)
{
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("L"), ParseStatus::Incomplete);
    EXPECT_EQ(status("1,"), ParseStatus::Incomplete);
    EXPECT_EQ(status("["), ParseStatus::Incomplete);
    EXPECT_EQ(status("zzz"), ParseStatus::Invalid);
    EXPECT_EQ(status("1,,3"), ParseStatus::Invalid);
    EXPECT_EQ(status(","), ParseStatus::Invalid);
    EXPECT_EQ(status("L2x"), ParseStatus::Invalid);
}
