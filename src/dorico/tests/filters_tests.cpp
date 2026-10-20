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

#include <set>

#include "dorico/internal/filters.h"

using namespace mu::dorico;

namespace {
FilterItem note(int group, int voice, int index, int size, bool grace = false)
{
    FilterItem i;
    i.kind = FilterKind::Note;
    i.voice = voice;
    i.group = group;
    i.indexFromTop = index;
    i.chordSize = size;
    i.grace = grace;
    return i;
}

FilterItem ofKind(FilterKind kind)
{
    FilterItem i;
    i.kind = kind;
    return i;
}

FilterPredicate def(const char* id)
{
    const FilterDef* d = findFilterDef(id);
    EXPECT_NE(d, nullptr) << id;
    return d ? d->matches : FilterPredicate([](const FilterItem&) { return false; });
}

using Indices = std::vector<size_t>;
}

TEST(Dorico_FiltersTests, DefIdsAreUniqueAndCoverTheListedCategories)
{
    std::set<std::string> ids;
    for (const FilterDef& d : filterDefs()) {
        EXPECT_TRUE(ids.insert(d.id).second) << d.id;
    }
    for (const char* id : { "notes", "grace-notes", "chords", "voice-1", "voice-4", "dynamics", "dynamics-gradual",
                            "dynamics-immediate", "tempo", "lyrics-line-1", "text", "slurs", "clefs" }) {
        EXPECT_TRUE(ids.count(id)) << id;
    }
    // Barlines, beams, articulations, fingerings and tremolos are not categories
    for (const char* id : { "barlines", "beams", "articulations", "fingerings", "tremolos" }) {
        EXPECT_FALSE(ids.count(id)) << id;
    }
}

TEST(Dorico_FiltersTests, SelectOnlyDynamics)
{
    const std::vector<FilterItem> items = { note(0, 0, 0, 1), ofKind(FilterKind::Dynamic), ofKind(FilterKind::Hairpin),
                                            ofKind(FilterKind::Slur) };
    EXPECT_EQ(applyFilter(items, def("dynamics"), false), (Indices { 1, 2 }));
    EXPECT_EQ(applyFilter(items, def("dynamics-gradual"), false), (Indices { 2 }));
    EXPECT_EQ(applyFilter(items, def("dynamics-immediate"), false), (Indices { 1 }));
}

TEST(Dorico_FiltersTests, DeselectOnlyIsTheComplementOfSelectOnly)
{
    const std::vector<FilterItem> items = { note(0, 0, 0, 1), ofKind(FilterKind::Dynamic), ofKind(FilterKind::Hairpin),
                                            ofKind(FilterKind::Slur), ofKind(FilterKind::None) };
    EXPECT_EQ(applyFilter(items, def("dynamics"), true), (Indices { 0, 3, 4 }));
}

TEST(Dorico_FiltersTests, PartsNeverStayAndAttachedItemsFollowTheirNote)
{
    FilterItem stem = ofKind(FilterKind::Part);
    FilterItem artic1 = ofKind(FilterKind::Attached);
    artic1.group = 0;
    FilterItem artic2 = ofKind(FilterKind::Attached);
    artic2.group = 1;
    const std::vector<FilterItem> items = { note(0, 0, 0, 1), stem, artic1, note(1, 1, 0, 1), artic2 };

    EXPECT_EQ(applyFilter(items, def("voice-1"), false), (Indices { 0, 2 }));
    EXPECT_EQ(applyFilter(items, def("voice-1"), true), (Indices { 3, 4 }));
    // Select Only on something that is not a note drops the articulations with their notes
    EXPECT_EQ(applyFilter(items, def("slurs"), false), (Indices {}));
}

TEST(Dorico_FiltersTests, VoicesApplyToNotesAndRests)
{
    FilterItem rest = ofKind(FilterKind::Rest);
    rest.voice = 1;
    const std::vector<FilterItem> items = { note(0, 0, 0, 1), note(1, 1, 0, 1), rest };
    EXPECT_EQ(applyFilter(items, def("voice-2"), false), (Indices { 1, 2 }));
    EXPECT_EQ(applyFilter(items, def("rests"), false), (Indices { 2 }));
}

TEST(Dorico_FiltersTests, NotesChordsAndPositionsInChords)
{
    const std::vector<FilterItem> items = { note(0, 0, 0, 3), note(0, 0, 1, 3), note(0, 0, 2, 3), note(1, 0, 0, 1),
                                            note(2, 0, 0, 1, true) };
    EXPECT_EQ(applyFilter(items, def("chords"), false), (Indices { 0, 1, 2 }));
    EXPECT_EQ(applyFilter(items, def("top-notes"), false), (Indices { 0, 3, 4 }));
    EXPECT_EQ(applyFilter(items, def("bottom-notes"), false), (Indices { 2, 3, 4 }));
    EXPECT_EQ(applyFilter(items, def("grace-notes"), false), (Indices { 4 }));
    EXPECT_EQ(applyFilter(items, def("notes"), false).size(), 5u);
}

TEST(Dorico_FiltersTests, LyricsLines)
{
    FilterItem l1 = ofKind(FilterKind::Lyrics);
    FilterItem l2 = ofKind(FilterKind::Lyrics);
    l2.verse = 1;
    const std::vector<FilterItem> items = { l1, l2, ofKind(FilterKind::Text) };
    EXPECT_EQ(applyFilter(items, def("lyrics"), false), (Indices { 0, 1 }));
    EXPECT_EQ(applyFilter(items, def("lyrics-line-2"), false), (Indices { 1 }));
}

TEST(Dorico_FiltersTests, ParsePitchTokens)
{
    const auto ok = [](const char* s) {
        const auto t = parsePitchTokens(s);
        EXPECT_TRUE(t.has_value()) << s;
        return t.value_or(std::vector<PitchToken> {});
    };
    EXPECT_EQ(pitchTokensToString(ok("c")), "c");
    EXPECT_EQ(pitchTokensToString(ok("c4")), "c4");
    EXPECT_EQ(pitchTokensToString(ok("Eb4")), "eb4");
    EXPECT_EQ(pitchTokensToString(ok("f#5")), "f#5");
    EXPECT_EQ(pitchTokensToString(ok("c4, e")), "c4,e");
    EXPECT_EQ(pitchTokensToString(ok("bb")), "bb");
    EXPECT_EQ(pitchTokensToString(ok("bbb")), "bbb");
    EXPECT_EQ(pitchTokensToString(ok("cx2")), "cx2");
    EXPECT_EQ(pitchTokensToString(ok("c##")), "cx");
    EXPECT_EQ(pitchTokensToString(ok("c-1")), "c-1");
    EXPECT_TRUE(ok("").empty());

    const std::vector<PitchToken> bFlat = ok("bb");
    ASSERT_EQ(bFlat.size(), 1u);
    EXPECT_EQ(bFlat[0].step, 6);
    EXPECT_EQ(bFlat[0].alter, -1);
    EXPECT_FALSE(bFlat[0].octave.has_value());

    for (const char* bad : { "h", "4", "c#b", "c4#", "c###", "c4,h", "cc", "c100" }) {
        EXPECT_FALSE(parsePitchTokens(bad).has_value()) << bad;
    }
}

TEST(Dorico_FiltersTests, MatchesPitchBySpellingAndOctave)
{
    const auto toks = [](const char* s) { return *parsePitchTokens(s); };
    const FilterPitch c4 { 0, 0, 4 };
    const FilterPitch c5 { 0, 0, 5 };
    const FilterPitch bSharp3 { 6, 1, 3 }; // sounds as C4 but is spelled B#3
    EXPECT_TRUE(matchesPitch(toks("c4"), c4));
    EXPECT_FALSE(matchesPitch(toks("c4"), c5));
    EXPECT_FALSE(matchesPitch(toks("c4"), bSharp3));
    EXPECT_TRUE(matchesPitch(toks("c"), c5));
    EXPECT_FALSE(matchesPitch(toks("c#"), c4));
    EXPECT_TRUE(matchesPitch(toks("c4,e"), FilterPitch { 2, 0, 1 }));
    EXPECT_FALSE(matchesPitch({}, c4));
}
