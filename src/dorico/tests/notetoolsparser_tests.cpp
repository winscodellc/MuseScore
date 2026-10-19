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

#include "dorico/popover/parsers/notetoolsparser.h"

using namespace mu::dorico;

namespace {
NoteToolsParse ok(const std::string& text)
{
    const NoteToolsParse p = parseNoteTools(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p;
}

void expectInterval(const IntervalSpec& i, int degree, IntervalQuality quality, std::optional<int> semitones = std::nullopt)
{
    EXPECT_EQ(i.degree, degree);
    EXPECT_EQ(i.quality, quality);
    EXPECT_EQ(i.semitones, semitones);
}

void expectStatus(const std::string& text, ParseStatus status)
{
    EXPECT_EQ(parseNoteTools(text).status, status) << text;
}
}

TEST(DoricoNoteToolsParserTests, AddAbove)
{
    for (const char* text : { "3", "3rd" }) {
        const NoteToolsParse p = ok(text);
        EXPECT_FALSE(p.transpose);
        EXPECT_EQ(p.scope, NoteToolsScope::All);
        ASSERT_EQ(p.intervals.size(), 1u);
        expectInterval(p.intervals[0], 3, IntervalQuality::None);
        EXPECT_TRUE(p.intervals[0].followsKey());
    }
}

TEST(DoricoNoteToolsParserTests, AddBelow)
{
    for (const char* text : { "-4", "-4th" }) {
        const NoteToolsParse p = ok(text);
        ASSERT_EQ(p.intervals.size(), 1u);
        expectInterval(p.intervals[0], -4, IntervalQuality::None);
    }
}

TEST(DoricoNoteToolsParserTests, Qualities)
{
    expectInterval(ok("M3").intervals[0], 3, IntervalQuality::Major);
    expectInterval(ok("maj3").intervals[0], 3, IntervalQuality::Major);
    expectInterval(ok("major3").intervals[0], 3, IntervalQuality::Major);
    expectInterval(ok("m3").intervals[0], 3, IntervalQuality::Minor);
    expectInterval(ok("min3").intervals[0], 3, IntervalQuality::Minor);
    expectInterval(ok("perf5").intervals[0], 5, IntervalQuality::Perfect);
    expectInterval(ok("p5").intervals[0], 5, IntervalQuality::Perfect);
    expectInterval(ok("per5").intervals[0], 5, IntervalQuality::Perfect);
    expectInterval(ok("aug4").intervals[0], 4, IntervalQuality::Augmented);
    expectInterval(ok("a4").intervals[0], 4, IntervalQuality::Augmented);
    expectInterval(ok("dim5").intervals[0], 5, IntervalQuality::Diminished);
    expectInterval(ok("d5").intervals[0], 5, IntervalQuality::Diminished);
    expectInterval(ok("diatonic3").intervals[0], 3, IntervalQuality::None);
    expectInterval(ok("MAJ3").intervals[0], 3, IntervalQuality::Major);
}

TEST(DoricoNoteToolsParserTests, MajorAndMinorAreCaseSensitive)
{
    expectInterval(ok("M3").intervals[0], 3, IntervalQuality::Major);
    expectInterval(ok("m3").intervals[0], 3, IntervalQuality::Minor);
}

TEST(DoricoNoteToolsParserTests, QualityMustFitTheDegree)
{
    expectStatus("p3", ParseStatus::Invalid);
    expectStatus("M5", ParseStatus::Invalid);
    expectStatus("m4", ParseStatus::Invalid);
    expectStatus("p8", ParseStatus::Ok);
    expectStatus("dim3", ParseStatus::Ok);
    expectStatus("aug5", ParseStatus::Ok);
}

TEST(DoricoNoteToolsParserTests, SeveralNotes)
{
    const NoteToolsParse p = ok("3,6");
    ASSERT_EQ(p.intervals.size(), 2u);
    expectInterval(p.intervals[0], 3, IntervalQuality::None);
    expectInterval(p.intervals[1], 6, IntervalQuality::None);

    const NoteToolsParse q = ok("-3,3,4");
    ASSERT_EQ(q.intervals.size(), 3u);
    expectInterval(q.intervals[0], -3, IntervalQuality::None);
}

TEST(DoricoNoteToolsParserTests, TopAndBottom)
{
    const NoteToolsParse p = ok("-M2,m3 bottom");
    EXPECT_EQ(p.scope, NoteToolsScope::Bottom);
    ASSERT_EQ(p.intervals.size(), 2u);
    expectInterval(p.intervals[0], -2, IntervalQuality::Major);
    expectInterval(p.intervals[1], 3, IntervalQuality::Minor);

    EXPECT_EQ(ok("3 top").scope, NoteToolsScope::Top);
}

TEST(DoricoNoteToolsParserTests, Units)
{
    for (const char* text : { "5 8 st", "5 8 div", "5 8 s", "5 8 d", "5 8 semitones", "5 8 divisions" }) {
        const NoteToolsParse p = ok(text);
        ASSERT_EQ(p.intervals.size(), 1u) << text;
        expectInterval(p.intervals[0], 5, IntervalQuality::None, 8);
        EXPECT_FALSE(p.intervals[0].followsKey());
    }
    expectInterval(ok("-6 9 st").intervals[0], -6, IntervalQuality::None, 9);
}

TEST(DoricoNoteToolsParserTests, QuarterTonesAreUnsupported)
{
    expectStatus("5 8 qt", ParseStatus::Unsupported);
    expectStatus("-6 9 q", ParseStatus::Unsupported);
    expectStatus("t 5 8 quartertones", ParseStatus::Unsupported);
}

TEST(DoricoNoteToolsParserTests, Transpose)
{
    const NoteToolsParse up = ok("t3");
    EXPECT_TRUE(up.transpose);
    ASSERT_EQ(up.intervals.size(), 1u);
    expectInterval(up.intervals[0], 3, IntervalQuality::None);

    expectInterval(ok("t-6").intervals[0], -6, IntervalQuality::None);
    expectInterval(ok("t-min6").intervals[0], -6, IntervalQuality::Minor);
    expectInterval(ok("t 5 8 div").intervals[0], 5, IntervalQuality::None, 8);
    expectInterval(ok("t -6 9 st").intervals[0], -6, IntervalQuality::None, 9);

    const NoteToolsParse top = ok("t top -8");
    EXPECT_TRUE(top.transpose);
    EXPECT_EQ(top.scope, NoteToolsScope::Top);
    expectInterval(top.intervals[0], -8, IntervalQuality::None);

    const NoteToolsParse bottom = ok("t bottom 8");
    EXPECT_EQ(bottom.scope, NoteToolsScope::Bottom);
    expectInterval(bottom.intervals[0], 8, IntervalQuality::None);
}

TEST(DoricoNoteToolsParserTests, TopIsNotATransposition)
{
    const NoteToolsParse p = ok("top 3");
    EXPECT_FALSE(p.transpose);
    EXPECT_EQ(p.scope, NoteToolsScope::Top);
}

TEST(DoricoNoteToolsParserTests, TransposeTakesOneInterval)
{
    expectStatus("t3,6", ParseStatus::Invalid);
}

TEST(DoricoNoteToolsParserTests, OtherToolsAreUnsupported)
{
    for (const char* text : { "inv", "invert", "mirror", "rev", "ret", "rot", "rotate 2", "rep 2 pit", "c4=eb4", "c4=eb4,f4=g4",
                              "c M to c m", "inv diat" }) {
        expectStatus(text, ParseStatus::Unsupported);
    }
}

TEST(DoricoNoteToolsParserTests, Invalid)
{
    expectStatus("16", ParseStatus::Invalid);
    expectStatus("0", ParseStatus::Invalid);
    expectStatus("banana", ParseStatus::Invalid);
    expectStatus("3,,6", ParseStatus::Invalid);
    expectStatus("5 8 foo", ParseStatus::Invalid);
    expectStatus("M3 8 st", ParseStatus::Invalid);
    expectStatus("5 500 st", ParseStatus::Invalid);
}

TEST(DoricoNoteToolsParserTests, IncompleteIsNotAnError)
{
    for (const char* text : { "", "  ", "-", "M", "dim", "ma", "3,", "5 8", "5 8 se", "3r", "t", "t-", "t top", "bot", "3 bo", "in",
                              "re", "t to" }) {
        expectStatus(text, ParseStatus::Incomplete);
    }
}

TEST(DoricoNoteToolsParserTests, PlainUnisonAddsNothing)
{
    expectStatus("1", ParseStatus::Incomplete); // may become 12
    expectStatus("-1", ParseStatus::Incomplete);
    expectStatus("a1", ParseStatus::Ok);
    expectStatus("t1", ParseStatus::Ok);
    expectStatus("12", ParseStatus::Ok);
}

TEST(DoricoNoteToolsParserTests, IntervalSteps)
{
    struct Row {
        const char* text;
        int semitones;
        int diatonic;
    };
    const Row rows[] = {
        { "M3", 4, 2 },
        { "m3", 3, 2 },
        { "p5", 7, 4 },
        { "dim5", 6, 4 },
        { "aug4", 6, 3 },
        { "M9", 14, 8 },
        { "p8", 12, 7 },
        { "m7", 10, 6 },
        { "dim7", 9, 6 },
        { "m2", 1, 1 },
        { "dim8", 11, 7 },
        { "p15", 24, 14 },
        { "-M3", -4, -2 },
        { "-p8", -12, -7 },
        { "3 8 st", 8, 2 },
        { "-6 9 st", -9, -5 },
    };
    for (const Row& r : rows) {
        const NoteToolsParse p = ok(r.text);
        const IntervalSteps s = toSteps(p.intervals[0]);
        EXPECT_EQ(s.semitones, r.semitones) << r.text;
        EXPECT_EQ(s.diatonic, r.diatonic) << r.text;
    }
}
