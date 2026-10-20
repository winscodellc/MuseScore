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

#include "dorico/popover/parsers/ornamentsparser.h"

using namespace mu::dorico;
using namespace mu::engraving;
using Kind = OrnamentEntry::Kind;

namespace {
OrnamentEntry ok(const std::string& text)
{
    const OrnamentParse p = parseOrnament(text);
    EXPECT_EQ(p.status, ParseStatus::Ok) << text;
    return p.entry;
}

ParseStatus status(const std::string& text)
{
    return parseOrnament(text).status;
}

void expectTrill(const std::string& text, IntervalStep step, IntervalType type)
{
    const OrnamentEntry e = ok(text);
    EXPECT_EQ(e.kind, Kind::Trill) << text;
    ASSERT_TRUE(e.interval.has_value()) << text;
    EXPECT_EQ(e.interval->step, step) << text;
    EXPECT_EQ(e.interval->type, type) << text;
}

void expectSymbol(const std::string& text, Kind kind, SymId sym)
{
    const OrnamentEntry e = ok(text);
    EXPECT_EQ(e.kind, kind) << text;
    EXPECT_EQ(e.sym, sym) << text;
}
}

TEST(DoricoOrnamentsParser, Trills)
{
    for (const char* t : { "tr", "trill", " TR" }) {
        const OrnamentEntry e = ok(t);
        EXPECT_EQ(e.kind, Kind::Trill) << t;
        EXPECT_FALSE(e.interval.has_value()) << t;
    }
    expectTrill("tr m3", IntervalStep::THIRD, IntervalType::MINOR);
    expectTrill("trill m3", IntervalStep::THIRD, IntervalType::MINOR);
    expectTrill("tr M2", IntervalStep::SECOND, IntervalType::MAJOR);
    expectTrill("tr maj2", IntervalStep::SECOND, IntervalType::MAJOR);
    expectTrill("tr Minor6", IntervalStep::SIXTH, IntervalType::MINOR);
    expectTrill("tr 2", IntervalStep::SECOND, IntervalType::AUTO);
    expectTrill("tr perf4", IntervalStep::FOURTH, IntervalType::PERFECT);
    expectTrill("tr p5", IntervalStep::FIFTH, IntervalType::PERFECT);
    expectTrill("tr aug2", IntervalStep::SECOND, IntervalType::AUGMENTED);
    expectTrill("tr d7", IntervalStep::SEVENTH, IntervalType::DIMINISHED);
    expectTrill("tr diminished5", IntervalStep::FIFTH, IntervalType::DIMINISHED);
    expectTrill("tr 8", IntervalStep::OCTAVE, IntervalType::AUTO);
}

TEST(DoricoOrnamentsParser, TrillIntervalErrors)
{
    for (const char* t : { "tr ", "tr m", "tr M", "tr maj", "tr ma", "tr per", "tr dimin", "tr  " }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "tr 9", "tr 0", "tr m9", "tr 10", "tr x3", "tr p3", "tr M4", "tr m5", "mor 3" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoOrnamentsParser, Ornaments)
{
    expectSymbol("shorttr", Kind::Ornament, SymId::ornamentShortTrill);
    expectSymbol("mordent", Kind::Ornament, SymId::ornamentMordent);
    expectSymbol("mor", Kind::Ornament, SymId::ornamentMordent);
    expectSymbol("turn", Kind::Ornament, SymId::ornamentTurn);
    expectSymbol("invertedturn", Kind::Ornament, SymId::ornamentTurnInverted);
    expectSymbol("invturn", Kind::Ornament, SymId::ornamentTurnInverted);
}

TEST(DoricoOrnamentsParser, Jazz)
{
    expectSymbol("brassbend", Kind::JazzOrnament, SymId::brassBend);
    expectSymbol("flip", Kind::JazzOrnament, SymId::brassFlip);
    expectSymbol("jazz", Kind::JazzOrnament, SymId::brassJazzTurn);
    expectSymbol("shake", Kind::JazzOrnament, SymId::brassJazzTurn);
    expectSymbol("smear", Kind::JazzOrnament, SymId::brassSmear);

    const struct { const char* text; ChordLineType type; bool smooth; } lines[] = {
        { "plop", ChordLineType::PLOP, false }, { "plopsmooth", ChordLineType::PLOP, true },
        { "scoop", ChordLineType::SCOOP, false },
        { "doit", ChordLineType::DOIT, false }, { "doitsmooth", ChordLineType::DOIT, true },
        { "fall", ChordLineType::FALL, false }, { "fallsmooth", ChordLineType::FALL, true },
    };
    for (const auto& l : lines) {
        const OrnamentEntry e = ok(l.text);
        EXPECT_EQ(e.kind, Kind::JazzArticulation) << l.text;
        EXPECT_EQ(e.chordLine, l.type) << l.text;
        EXPECT_EQ(e.smooth, l.smooth) << l.text;
    }
}

TEST(DoricoOrnamentsParser, ArpeggiosAndGlissandos)
{
    for (const char* t : { "arp", "arpup", "arpeggioup" }) {
        EXPECT_EQ(ok(t).arpeggio, ArpeggioType::NORMAL) << t;
    }
    for (const char* t : { "arpdown", "arpeggiodown" }) {
        EXPECT_EQ(ok(t).arpeggio, ArpeggioType::DOWN) << t;
    }
    for (const char* t : { "nonarp", "nonarpeggio" }) {
        EXPECT_EQ(ok(t).kind, Kind::Arpeggio) << t;
        EXPECT_EQ(ok(t).arpeggio, ArpeggioType::BRACKET) << t;
    }
    EXPECT_EQ(ok("gliss").kind, Kind::Glissando);
    EXPECT_FALSE(ok("gliss").glissando.has_value());
    EXPECT_EQ(ok("glissstraight").glissando, GlissandoType::STRAIGHT);
    EXPECT_EQ(ok("glisswavy").glissando, GlissandoType::WAVY);
}

TEST(DoricoOrnamentsParser, UnsupportedIncompleteInvalid)
{
    EXPECT_EQ(status("slurarp"), ParseStatus::Unsupported);
    for (const char* t : { "", "  ", "t", "mo", "arpeg", "gli", "w/", "fallsm", "inv", "vib", "lhtappu", "hammerp" }) {
        EXPECT_EQ(status(t), ParseStatus::Incomplete) << t;
    }
    for (const char* t : { "xyz", "trills", "arpx", "glissy", "mor x" }) {
        EXPECT_EQ(status(t), ParseStatus::Invalid) << t;
    }
}

TEST(DoricoOrnamentsParser, GuitarTechniques)
{
    using G = OrnamentEntry::GuitarTech;
    const std::vector<std::pair<std::string, G> > words = {
        { "bend", G::Bend }, { "vibbend", G::Dive }, { "vibscoop", G::Scoop }, { "vibdip", G::Dip },
        { "wbar", G::WhammyBar }, { "w/bar", G::WhammyBar },
        { "hammer", G::HammerPull }, { "ho", G::HammerPull }, { "pull", G::HammerPull }, { "po", G::HammerPull },
        { "hammerpull", G::HammerPull }, { "hopo", G::HammerPull }, { "hp", G::HammerPull }, { "ligado", G::HammerPull },
        { "lig", G::HammerPull },
        { "tap", G::Tap }, { "lhtap", G::LeftTap }, { "tappull", G::TapPull }, { "lhtappull", G::LeftTapPull },
    };
    for (const auto& [text, tech] : words) {
        const OrnamentEntry e = ok(text);
        EXPECT_EQ(e.kind, Kind::Guitar) << text;
        EXPECT_EQ(e.guitar, tech) << text;
    }
    EXPECT_EQ(ok("BEND").guitar, G::Bend);
    EXPECT_EQ(status("bends"), ParseStatus::Invalid);
    EXPECT_EQ(guitarText(G::HammerPull), "hopo");
    EXPECT_EQ(guitarText(G::LeftTap), "lhtap");
    EXPECT_EQ(guitarText(G::WhammyBar), "wbar");
}

TEST(DoricoOrnamentsParser, PrefillText)
{
    EXPECT_EQ(ornamentText(SymId::ornamentMordent), "mor");
    EXPECT_EQ(ornamentText(SymId::ornamentTurnInverted), "invturn");
    EXPECT_EQ(ornamentText(SymId::brassJazzTurn), "jazz");
    EXPECT_EQ(ornamentText(SymId::ornamentTrill), "tr");
    EXPECT_EQ(ornamentText(SymId::ornamentPrecompSlide), "");
    EXPECT_EQ(trillText(OrnamentInterval()), "tr");
    EXPECT_EQ(trillText(OrnamentInterval(IntervalStep::THIRD, IntervalType::MINOR)), "tr m3");
    EXPECT_EQ(trillText(OrnamentInterval(IntervalStep::SECOND, IntervalType::MAJOR)), "tr M2");
    EXPECT_EQ(trillText(OrnamentInterval(IntervalStep::FOURTH, IntervalType::PERFECT)), "tr p4");
    EXPECT_EQ(trillText(OrnamentInterval(IntervalStep::THIRD, IntervalType::AUTO)), "tr 3");
    EXPECT_EQ(arpeggioText(ArpeggioType::NORMAL), "arp");
    EXPECT_EQ(arpeggioText(ArpeggioType::UP), "arpup");
    EXPECT_EQ(arpeggioText(ArpeggioType::DOWN), "arpdown");
    EXPECT_EQ(arpeggioText(ArpeggioType::BRACKET), "nonarp");
    EXPECT_EQ(arpeggioText(ArpeggioType::UP_STRAIGHT), "");
    EXPECT_EQ(glissandoText(GlissandoType::WAVY), "glisswavy");
    EXPECT_EQ(glissandoText(GlissandoType::STRAIGHT), "gliss");
    EXPECT_EQ(jazzArticulationText(ChordLineType::FALL, true), "fallsmooth");
    EXPECT_EQ(jazzArticulationText(ChordLineType::SCOOP, true), "");
    for (const OrnamentInterval& i : { OrnamentInterval(IntervalStep::SIXTH, IntervalType::AUGMENTED),
                                       OrnamentInterval(IntervalStep::FIFTH, IntervalType::DIMINISHED) }) {
        EXPECT_EQ(parseOrnament(trillText(i)).entry.interval, i); // round trip
    }
}
