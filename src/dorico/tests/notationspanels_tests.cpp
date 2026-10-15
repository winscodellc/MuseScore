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

#include "dorico/internal/notationspanels.h"

using namespace mu::dorico;
using mu::engraving::ElementType;

TEST(NotationsPanels, NamePatterns)
{
    EXPECT_TRUE(nameMatches("Treble clef", "Treble clef"));
    EXPECT_FALSE(nameMatches("Tenor clef", "Tenor clef (French, 18th century)"));
    EXPECT_TRUE(nameMatches("sf (*", "sf (sforzando)"));
    EXPECT_FALSE(nameMatches("sf (*", "sfz (sforzato)"));
    EXPECT_TRUE(nameMatches("*caesura*", "Curved caesura"));
    EXPECT_TRUE(nameMatches("*century)*", "Alto clef (French, 18th century)"));
}

TEST(NotationsPanels, RestTakesWhatOtherSectionsLeave)
{
    const std::vector<PaletteCellInfo> clefs = {
        { ElementType::CLEF, "Treble clef" },
        { ElementType::CLEF, "Soprano clef" },
        { ElementType::CLEF, "Alto clef (French, 18th century)" },
        { ElementType::CLEF, "Bass clef" },
    };
    const std::vector<PaletteCellInfo> lines = {
        { ElementType::TEXTLINE, "Text line" },
        { ElementType::OTTAVA, "8va alta" },
    };
    const CellsOf cellsOf = [&](SourcePalette p) -> const std::vector<PaletteCellInfo>& {
        return p == SourcePalette::Lines ? lines : clefs;
    };

    const std::vector<NotationsSection> sections = {
        { "clefs", "Common", SourcePalette::Clefs, { ElementType::CLEF }, { "Treble clef", "Bass clef", "Gone clef" } },
        { "clefs", "Uncommon", SourcePalette::Clefs, { ElementType::CLEF }, {}, true },
        { "clefs", "Archaic Clefs", SourcePalette::Clefs, { ElementType::CLEF }, { "*century)*" } },
        { "clefs", "Octave Lines", SourcePalette::Lines, { ElementType::OTTAVA }, {} },
    };

    QStringList unknown;
    const auto cells = resolveSections(sections, cellsOf, &unknown);

    EXPECT_EQ(cells[0], std::vector<size_t>({ 0, 3 }));
    EXPECT_EQ(cells[1], std::vector<size_t>({ 1 }));       // the archaic clef is not "rest"
    EXPECT_EQ(cells[2], std::vector<size_t>({ 2 }));
    EXPECT_EQ(cells[3], std::vector<size_t>({ 1 }));
    EXPECT_EQ(unknown, QStringList({ "Common: Gone clef" }));
}

TEST(NotationsPanels, TableHasElevenBackedPanelsPlusBracketing)
{
    QStringList panels;
    for (const NotationsSection& s : notationsSections()) {
        if (!panels.contains(s.panel)) {
            panels << s.panel;
        }
    }
    EXPECT_EQ(panels, QStringList({ "clefs", "keySignatures", "timeSignatures", "tempo", "dynamics", "ornaments",
                                    "repeatStructures", "barsAndBarlines", "holdsAndPauses", "playingTechniques", "lines",
                                    "engraveBracketing" }));
    EXPECT_TRUE(sectionsOfPanel("cues").empty());
}
