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
