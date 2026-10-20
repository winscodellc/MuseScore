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

#include "dorico/internal/pastespecial.h"

using namespace mu::dorico;

namespace {
muse::ByteArray clip(const char* staffBody)
{
    return muse::ByteArray::fromRawData(
        staffBody, static_cast<size_t>(std::char_traits<char>::length(staffBody)));
}
}

TEST(DoricoPasteSpecial, ClipVoicesNotAStaffList)
{
    EXPECT_TRUE(pasteClipVoices(clip("<Foo/>")).empty());
    EXPECT_TRUE(pasteClipVoices(muse::ByteArray()).empty());
}

TEST(DoricoPasteSpecial, ClipVoicesFirstVoiceOnly)
{
    const auto data = clip("<StaffList tick=\"0\" len=\"1/1\" staff=\"0\" staves=\"1\"><Staff id=\"0\">"
                           "<Chord><durationType>quarter</durationType><Note><pitch>60</pitch></Note></Chord>"
                           "<Rest><durationType>quarter</durationType></Rest></Staff></StaffList>");
    EXPECT_EQ(pasteClipVoices(data), (std::set<int> { 0 }));
}

TEST(DoricoPasteSpecial, ClipVoicesSecondVoiceOnlyIsReadFromTheLocationStep)
{
    const auto data = clip("<StaffList tick=\"0\" len=\"1/1\" staff=\"0\" staves=\"1\"><Staff id=\"0\">"
                           "<Rest><durationType>measure</durationType></Rest>"
                           "<location><voices>1</voices><fractions>-1/1</fractions></location>"
                           "<Chord><durationType>quarter</durationType><Note><pitch>60</pitch></Note></Chord>"
                           "</Staff></StaffList>");
    EXPECT_EQ(pasteClipVoices(data), (std::set<int> { 1 }));
}

TEST(DoricoPasteSpecial, ClipVoicesSeveralVoicesAndNestedLocationsAreIgnored)
{
    // Voice 0 chord has a spanner whose <location> belongs to the slur end, not to the voice
    const auto data = clip("<StaffList tick=\"0\" len=\"1/1\" staff=\"0\" staves=\"1\"><Staff id=\"0\">"
                           "<Chord><Spanner type=\"Slur\"><Slur/><next><location><voices>3</voices></location></next></Spanner></Chord>"
                           "<location><voices>2</voices><fractions>-1/4</fractions></location>"
                           "<Chord/>"
                           "<location><voices>-1</voices></location>"
                           "<Chord/>"
                           "</Staff></StaffList>");
    EXPECT_EQ(pasteClipVoices(data), (std::set<int> { 0, 1, 2 }));
}

TEST(DoricoPasteSpecial, ClipVoicesRestartPerStaff)
{
    const auto data = clip("<StaffList tick=\"0\" len=\"1/1\" staff=\"0\" staves=\"2\">"
                           "<Staff id=\"0\"><location><voices>1</voices></location><Chord/></Staff>"
                           "<Staff id=\"1\"><Chord/></Staff></StaffList>");
    EXPECT_EQ(pasteClipVoices(data), (std::set<int> { 0, 1 }));
}

TEST(DoricoPasteSpecial, ClipFromReadsLengthAndStaves)
{
    const auto parsed = pasteClipFrom(clip("<StaffList tick=\"0\" len=\"3/4\" staff=\"1\" staves=\"2\"/>"));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->len, mu::engraving::Fraction(3, 4));
    EXPECT_EQ(parsed->staves, 2u);
}

TEST(DoricoPasteSpecial, ClipFromRefusesWhatIsNotAStaffList)
{
    EXPECT_FALSE(pasteClipFrom(clip("<Foo/>")));
    EXPECT_FALSE(pasteClipFrom(muse::ByteArray()));
    EXPECT_FALSE(pasteClipFrom(clip("<StaffList len=\"0/1\" staves=\"1\"/>")));
    EXPECT_FALSE(pasteClipFrom(clip("<StaffList len=\"1/1\" staves=\"0\"/>")));
}

TEST(DoricoPasteSpecial, ReduceGroupsAreTopDownLargestFirst)
{
    EXPECT_EQ(reduceGroups(5, 3), (std::vector<size_t> { 2, 2, 1 }));
    EXPECT_EQ(reduceGroups(4, 2), (std::vector<size_t> { 2, 2 }));
    EXPECT_EQ(reduceGroups(3, 3), (std::vector<size_t> { 1, 1, 1 }));
    EXPECT_EQ(reduceGroups(7, 3), (std::vector<size_t> { 3, 2, 2 }));
    EXPECT_EQ(reduceGroups(1, 1), (std::vector<size_t> { 1 }));
    EXPECT_TRUE(reduceGroups(3, 0).empty());
}

TEST(DoricoPasteSpecial, ExplodeAsManyNotesAsStavesIsOnePerStaff)
{
    using Notes = std::vector<std::vector<size_t> >;
    EXPECT_EQ(explodeNotes(3, 3), (Notes { { 0 }, { 1 }, { 2 } }));
}

TEST(DoricoPasteSpecial, ExplodeFewerNotesThanStavesSharesNotes)
{
    using Notes = std::vector<std::vector<size_t> >;
    EXPECT_EQ(explodeNotes(2, 3), (Notes { { 0 }, { 1 }, { 1 } }));
    EXPECT_EQ(explodeNotes(1, 3), (Notes { { 0 }, { 0 }, { 0 } }));
    EXPECT_EQ(explodeNotes(2, 4), (Notes { { 0 }, { 0 }, { 1 }, { 1 } }));
}

TEST(DoricoPasteSpecial, ExplodeMoreNotesThanStavesGivesTheExtrasToTheUpperStaves)
{
    using Notes = std::vector<std::vector<size_t> >;
    EXPECT_EQ(explodeNotes(4, 3), (Notes { { 0, 1 }, { 2 }, { 3 } }));
    EXPECT_EQ(explodeNotes(5, 3), (Notes { { 0, 1 }, { 2, 3 }, { 4 } }));
    EXPECT_EQ(explodeNotes(4, 2), (Notes { { 0, 1 }, { 2, 3 } }));
}

TEST(DoricoPasteSpecial, StaffClipOfKeepsOneStaffAndItsSpanners)
{
    const auto data = clip("<StaffList version=\"5.00\" tick=\"0\" len=\"1/1\" staff=\"2\" staves=\"2\">"
                           "<Staff id=\"2\"><Chord><Note><pitch>60</pitch></Note></Chord></Staff>"
                           "<Staff id=\"3\"><Chord><Note><pitch>72</pitch></Note></Chord></Staff>"
                           "<SpannerMap>"
                           "<Slur><track>8</track><Foo><track>99</track></Foo></Slur>"
                           "<Slur><track>12</track></Slur>"
                           "</SpannerMap></StaffList>");

    const auto lower = staffClipOf(data, 1);
    const std::string text(reinterpret_cast<const char*>(lower.constData()), lower.size());
    EXPECT_NE(text.find("staff=\"3\""), std::string::npos);
    EXPECT_NE(text.find("staves=\"1\""), std::string::npos);
    EXPECT_NE(text.find("<pitch>72</pitch>"), std::string::npos);
    EXPECT_EQ(text.find("<pitch>60</pitch>"), std::string::npos);
    EXPECT_NE(text.find("<track>12</track>"), std::string::npos);
    EXPECT_EQ(text.find("<track>8</track>"), std::string::npos);

    const auto upper = staffClipOf(data, 0);
    const std::string upperText(reinterpret_cast<const char*>(upper.constData()), upper.size());
    EXPECT_NE(upperText.find("<track>8</track>"), std::string::npos);
    EXPECT_NE(upperText.find("<track>99</track>"), std::string::npos); // nested tracks are not the slur's own
    EXPECT_EQ(upperText.find("<track>12</track>"), std::string::npos);

    EXPECT_TRUE(staffClipOf(data, 2).empty());
    EXPECT_TRUE(staffClipOf(clip("<Foo/>"), 0).empty());
}

TEST(DoricoPasteSpecial, StaffClipOfRoundTripsThroughTheClipReader)
{
    const auto data = clip("<StaffList tick=\"0\" len=\"1/1\" staff=\"0\" staves=\"2\">"
                           "<Staff id=\"0\"><Chord/></Staff><Staff id=\"1\"><location><voices>1</voices></location><Chord/></Staff>"
                           "</StaffList>");
    const auto second = staffClipOf(data, 1);
    EXPECT_EQ(pasteClipVoices(second), (std::set<int> { 1 }));
    const auto parsed = pasteClipFrom(second);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->staves, 1u);
}

TEST(DoricoPasteSpecial, StaffClipOfStartsEveryStaffAtTheClipTick)
{
    // Real shape: staff 1 rewinds from the end of staff 0; on its own that would land before the destination
    const auto data = clip("<StaffList version=\"5.00\" tick=\"8/1\" len=\"4/4\" staff=\"0\" staves=\"3\">"
                           "<Staff id=\"0\"><location><fractions>8/1</fractions></location><Chord/></Staff>"
                           "<Staff id=\"1\"><location><fractions>-1/1</fractions></location><Chord/></Staff>"
                           "<Staff id=\"2\"><Chord/></Staff></StaffList>");
    for (size_t i = 0; i < 3; ++i) {
        const auto one = staffClipOf(data, i);
        const std::string text(reinterpret_cast<const char*>(one.constData()), one.size());
        EXPECT_NE(text.find("<location><fractions>8/1</fractions></location><Chord"), std::string::npos) << i << ": " << text;
        EXPECT_EQ(text.find("-1/1"), std::string::npos) << i;
    }
}
