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

#include "dorico/popover/parsers/tempoparser.h"

using namespace mu::dorico;
using mu::engraving::DurationType;
using mu::engraving::GradualTempoChangeType;
using Kind = TempoEntry::Kind;

namespace {
TempoEntry ok(const std::string& text)
{
    const TempoParse p = parseTempo(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

void expectMark(const std::string& text, DurationType type, int dots, double bpm)
{
    const TempoEntry e = ok(text);
    ASSERT_TRUE(e.unit.has_value()) << text;
    EXPECT_EQ(e.unit->type, type) << text;
    EXPECT_EQ(e.unit->dots, dots) << text;
    EXPECT_DOUBLE_EQ(e.bpm, bpm) << text;
}

void expectGradual(const std::string& text, GradualTempoChangeType type, const std::string& printed, bool line)
{
    const TempoEntry e = ok(text);
    EXPECT_EQ(e.kind, Kind::Gradual) << text;
    EXPECT_EQ(e.gradual, type) << text;
    EXPECT_EQ(e.text, printed) << text;
    EXPECT_EQ(e.line, line) << text;
}

ParseStatus status(const std::string& text)
{
    return parseTempo(text).status;
}
}

TEST(DoricoTempoParser, Words)
{
    TempoEntry e = ok("Allegro");
    EXPECT_EQ(e.kind, Kind::Absolute);
    EXPECT_EQ(e.text, "Allegro");
    EXPECT_DOUBLE_EQ(e.wordBpm, 144);
    EXPECT_FALSE(e.unit.has_value());

    EXPECT_EQ(ok("Mosso").kind, Kind::Relative);
    EXPECT_EQ(ok("Lento").kind, Kind::Relative);
    EXPECT_EQ(ok("più").kind, Kind::Absolute);
    EXPECT_EQ(ok("a tempo").kind, Kind::ATempo);
    EXPECT_EQ(ok("Tempo primo").kind, Kind::TempoPrimo);

    e = ok("anything goes");
    EXPECT_EQ(e.kind, Kind::FreeText);
    EXPECT_EQ(e.text, "anything goes");
}

TEST(DoricoTempoParser, MetronomeMarks)
{
    expectMark("q=120", DurationType::V_QUARTER, 0, 120);
    expectMark("q = 120", DurationType::V_QUARTER, 0, 120);
    expectMark("e=90", DurationType::V_EIGHTH, 0, 90);
    expectMark("h.=60", DurationType::V_HALF, 1, 60);
    expectMark("q.=80", DurationType::V_QUARTER, 1, 80);
    expectMark("5=100", DurationType::V_EIGHTH, 0, 100);
    expectMark("2w=30", DurationType::V_BREVE, 0, 30);
    expectMark("2=200", DurationType::V_64TH, 0, 200);
    expectMark("q=72.5", DurationType::V_QUARTER, 0, 72.5);

    const TempoEntry e = ok("Presto q=176");
    EXPECT_EQ(e.kind, Kind::Absolute);
    EXPECT_EQ(e.text, "Presto");
    expectMark("Presto q=176", DurationType::V_QUARTER, 0, 176);
}

TEST(DoricoTempoParser, ParenthesesAndBrackets)
{
    TempoEntry e = ok("(q=72)");
    EXPECT_TRUE(e.parenthesised);
    EXPECT_TRUE(e.text.empty());
    EXPECT_DOUBLE_EQ(e.bpm, 72);

    e = ok("Presto (q=176)");
    EXPECT_TRUE(e.parenthesised);
    EXPECT_EQ(e.text, "Presto");

    e = ok("[q=96]");
    EXPECT_TRUE(e.hidden);
    EXPECT_FALSE(e.markHidden);
    EXPECT_DOUBLE_EQ(e.bpm, 96);

    e = ok("Allegro [q=96]");
    EXPECT_FALSE(e.hidden);
    EXPECT_TRUE(e.markHidden);
    EXPECT_EQ(e.text, "Allegro");

    EXPECT_TRUE(ok("[rit.]").hidden);
    EXPECT_EQ(status("[q=96"), ParseStatus::Incomplete);
    EXPECT_EQ(status("(q=96"), ParseStatus::Incomplete);
}

TEST(DoricoTempoParser, GradualChanges)
{
    expectGradual("rit.", GradualTempoChangeType::Ritardando, "rit.", false);
    expectGradual("rit", GradualTempoChangeType::Ritardando, "rit.", false);
    expectGradual("ritardando", GradualTempoChangeType::Ritardando, "ritardando", false);
    expectGradual("rit....", GradualTempoChangeType::Ritardando, "rit.", true);
    expectGradual("ritardando...", GradualTempoChangeType::Ritardando, "ritardando", true);
    expectGradual("rall.", GradualTempoChangeType::Rallentando, "rall.", false);
    expectGradual("rallentando", GradualTempoChangeType::Rallentando, "rallentando", false);
    expectGradual("rall....", GradualTempoChangeType::Rallentando, "rall.", true);
    expectGradual("accel.", GradualTempoChangeType::Accelerando, "accel.", false);
    expectGradual("accelerando", GradualTempoChangeType::Accelerando, "accelerando", false);
    expectGradual("accel....", GradualTempoChangeType::Accelerando, "accel.", true);
}

TEST(DoricoTempoParser, Unsupported)
{
    for (const char* t : { "q=96-112", "e=e.", "q=e", "q.=h", "Light swing 8ths", "Straight (no swing)",
                           "2:1 swing 16ths (fixed)" }) {
        EXPECT_EQ(status(t), ParseStatus::Unsupported) << t;
    }
}

TEST(DoricoTempoParser, IncompleteAndInvalid)
{
    EXPECT_EQ(status(""), ParseStatus::Incomplete);
    EXPECT_EQ(status("  "), ParseStatus::Incomplete);
    EXPECT_EQ(status("q="), ParseStatus::Incomplete);
    EXPECT_EQ(status("q=96-"), ParseStatus::Incomplete);
    EXPECT_EQ(status("q=0"), ParseStatus::Invalid);
    EXPECT_EQ(status("q=-5"), ParseStatus::Invalid);
    EXPECT_EQ(status("q=fast"), ParseStatus::Invalid);
}

TEST(DoricoTempoParser, Suggestions)
{
    const std::vector<std::string> all = tempoSuggestions("all");
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], "Allegretto");
    EXPECT_EQ(all[1], "Allegro");
    EXPECT_EQ(tempoSuggestions("RIT").front(), "rit.");
    EXPECT_TRUE(tempoSuggestions("").empty());
    EXPECT_TRUE(tempoSuggestions("Allegro").empty());
    EXPECT_EQ(tempoSuggestions("rit").size(), 2u); // rit., ritardando
    EXPECT_TRUE(tempoSuggestions("q=1").empty());
    EXPECT_LE(tempoSuggestions("a").size(), 8u);
}

TEST(DoricoTempoParser, BeatUnitText)
{
    EXPECT_EQ(beatUnitText({ DurationType::V_QUARTER, 0 }), "q");
    EXPECT_EQ(beatUnitText({ DurationType::V_EIGHTH, 1 }), "e.");
    EXPECT_EQ(beatUnitText({ DurationType::V_BREVE, 0 }), "2w");
}
