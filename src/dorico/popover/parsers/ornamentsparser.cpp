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

#include "ornamentsparser.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace mu::dorico;
using namespace mu::engraving;
using OKind = OrnamentEntry::Kind;

namespace {
struct OrnamentWords {
    OrnamentEntry entry;
    std::vector<std::string> words; // the first one is the canonical entry (prefill)
};

OrnamentEntry symbolEntry(OKind kind, SymId sym)
{
    OrnamentEntry e;
    e.kind = kind;
    e.sym = sym;
    return e;
}

OrnamentEntry jazzLineEntry(ChordLineType type, bool smooth)
{
    OrnamentEntry e;
    e.kind = OKind::JazzArticulation;
    e.chordLine = type;
    e.smooth = smooth;
    return e;
}

OrnamentEntry arpeggioEntry(ArpeggioType type)
{
    OrnamentEntry e;
    e.kind = OKind::Arpeggio;
    e.arpeggio = type;
    return e;
}

OrnamentEntry glissandoEntry(std::optional<GlissandoType> type)
{
    OrnamentEntry e;
    e.kind = OKind::Glissando;
    e.glissando = type;
    return e;
}

OrnamentEntry guitarEntry(OrnamentEntry::GuitarTech tech)
{
    OrnamentEntry e;
    e.kind = OKind::Guitar;
    e.guitar = tech;
    return e;
}

using GTech = OrnamentEntry::GuitarTech;

const std::vector<OrnamentWords> ORNAMENT_WORDS = {
    { { OKind::Trill },                                         { "tr", "trill" } },
    { symbolEntry(OKind::Ornament, SymId::ornamentShortTrill),       { "shorttr" } },
    { symbolEntry(OKind::Ornament, SymId::ornamentMordent),          { "mor", "mordent" } },
    { symbolEntry(OKind::Ornament, SymId::ornamentTurn),             { "turn" } },
    { symbolEntry(OKind::Ornament, SymId::ornamentTurnInverted),     { "invturn", "invertedturn" } },
    { symbolEntry(OKind::JazzOrnament, SymId::brassBend),            { "brassbend" } },
    { symbolEntry(OKind::JazzOrnament, SymId::brassFlip),            { "flip" } },
    { symbolEntry(OKind::JazzOrnament, SymId::brassJazzTurn),        { "jazz", "shake" } },
    { symbolEntry(OKind::JazzOrnament, SymId::brassSmear),           { "smear" } },
    { jazzLineEntry(ChordLineType::PLOP, false),                    { "plop" } },
    { jazzLineEntry(ChordLineType::PLOP, true),                     { "plopsmooth" } },
    { jazzLineEntry(ChordLineType::SCOOP, false),                   { "scoop" } },
    { jazzLineEntry(ChordLineType::DOIT, false),                    { "doit" } },
    { jazzLineEntry(ChordLineType::DOIT, true),                     { "doitsmooth" } },
    { jazzLineEntry(ChordLineType::FALL, false),                    { "fall" } },
    { jazzLineEntry(ChordLineType::FALL, true),                     { "fallsmooth" } },
    { arpeggioEntry(ArpeggioType::NORMAL),                          { "arp", "arpup", "arpeggioup" } }, // up, no arrow
    { arpeggioEntry(ArpeggioType::UP),                              { "arpup" } },                      // prefill only
    { arpeggioEntry(ArpeggioType::DOWN),                            { "arpdown", "arpeggiodown" } },
    { arpeggioEntry(ArpeggioType::BRACKET),                         { "nonarp", "nonarpeggio" } },
    { glissandoEntry(std::nullopt),                                 { "gliss" } },
    { glissandoEntry(GlissandoType::STRAIGHT),                      { "glissstraight" } },
    { glissandoEntry(GlissandoType::WAVY),                          { "glisswavy" } },
    { guitarEntry(GTech::Bend),                                     { "bend" } },
    { guitarEntry(GTech::Dive),                                     { "vibbend" } },
    { guitarEntry(GTech::Scoop),                                    { "vibscoop" } },
    { guitarEntry(GTech::Dip),                                      { "vibdip" } },
    { guitarEntry(GTech::WhammyBar),                                { "wbar", "w/bar" } },
    { guitarEntry(GTech::HammerPull),                               { "hopo", "hammer", "ho", "pull", "po", "hammerpull", "hp",
                                                                      "ligado", "lig" } },
    { guitarEntry(GTech::Tap),                                      { "tap" } },
    { guitarEntry(GTech::LeftTap),                                  { "lhtap" } },
    { guitarEntry(GTech::TapPull),                                  { "tappull" } },
    { guitarEntry(GTech::LeftTapPull),                              { "lhtappull" } },
};

// Valid Dorico entries MuseScore has no item for (the curved arpeggio).
const std::vector<std::string> UNSUPPORTED_ORNAMENTS = { "slurarp" };

struct TrillQuality {
    IntervalType type;
    std::vector<std::string> words; // compared case-sensitively: M is major, m minor
};

const std::vector<TrillQuality> TRILL_QUALITIES = {
    { IntervalType::MAJOR,      { "M", "maj", "major" } },
    { IntervalType::MINOR,      { "m", "min", "minor" } },
    { IntervalType::PERFECT,    { "p", "per", "perf" } },
    { IntervalType::DIMINISHED, { "d", "dim", "diminished" } },
    { IntervalType::AUGMENTED,  { "a", "aug", "augmented" } },
};

bool hasPrefix(const std::string& word, const std::string& prefix)
{
    return word.compare(0, prefix.size(), prefix) == 0;
}

std::string lowered(std::string s)
{
    for (char& c : s) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string trimOrnament(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    return b == std::string::npos ? std::string() : s.substr(b, s.find_last_not_of(' ') - b + 1);
}

// "m3", "M2", "perf4", "aug2", "3": an optional quality, then the degree 1-8.
OrnamentParse parseTrillInterval(const std::string& text)
{
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }
    const bool hasDegree = std::isdigit(static_cast<unsigned char>(text.back()));
    const std::string quality = hasDegree ? text.substr(0, text.size() - 1) : text;
    const auto matches = [&](const std::string& w) { return w.size() == 1 ? w == quality : w == lowered(quality); };

    std::optional<IntervalType> type;
    bool prefix = false;
    for (const TrillQuality& q : TRILL_QUALITIES) {
        for (const std::string& w : q.words) {
            if (matches(w)) {
                type = q.type;
            }
            prefix = prefix || (w.size() > 1 && hasPrefix(w, lowered(quality)));
        }
    }
    if (!hasDegree) {
        return { type || prefix ? ParseStatus::Incomplete : ParseStatus::Invalid, {} };
    }
    const int degree = text.back() - '0';
    if ((!quality.empty() && !type) || degree < 1 || degree > 8) {
        return { ParseStatus::Invalid, {} };
    }

    OrnamentInterval interval(IntervalStep(degree - 1), type.value_or(IntervalType::AUTO));
    // A perfect second or a major fifth does not exist.
    const bool perfectStep = OrnamentInterval::isPerfectStep(interval.step);
    if ((perfectStep && (interval.type == IntervalType::MAJOR || interval.type == IntervalType::MINOR))
        || (!perfectStep && interval.type == IntervalType::PERFECT)) {
        return { ParseStatus::Invalid, {} };
    }
    OrnamentEntry e;
    e.kind = OKind::Trill;
    e.interval = interval;
    return { ParseStatus::Ok, e };
}
}

OrnamentParse mu::dorico::parseOrnament(const std::string& input)
{
    const std::string text = trimOrnament(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }

    // "tr m3": the interval follows a space and keeps its case (M / m). "tr " waits for the interval.
    const std::string left = input.substr(input.find_first_not_of(' '));
    const size_t space = left.find(' ');
    if (space != std::string::npos) {
        const std::string first = lowered(left.substr(0, space));
        if (first == "tr" || first == "trill") {
            return parseTrillInterval(trimOrnament(left.substr(space)));
        }
    }

    const std::string word = lowered(text);
    bool prefix = false;
    for (const OrnamentWords& o : ORNAMENT_WORDS) {
        for (const std::string& w : o.words) {
            if (w == word) {
                return { ParseStatus::Ok, o.entry };
            }
            prefix = prefix || hasPrefix(w, word);
        }
    }
    for (const std::string& w : UNSUPPORTED_ORNAMENTS) {
        if (w == word) {
            return { ParseStatus::Unsupported, {} };
        }
        prefix = prefix || hasPrefix(w, word);
    }
    return { prefix ? ParseStatus::Incomplete : ParseStatus::Invalid, {} };
}

namespace {
std::string firstOrnamentWord(const std::function<bool(const OrnamentEntry&)>& match)
{
    auto it = std::find_if(ORNAMENT_WORDS.begin(), ORNAMENT_WORDS.end(), [&](const OrnamentWords& o) { return match(o.entry); });
    return it == ORNAMENT_WORDS.end() ? std::string() : it->words.front();
}
}

std::string mu::dorico::ornamentText(SymId sym)
{
    if (sym == SymId::ornamentTrill) {
        return "tr"; // MuseScore's trill symbol without a line
    }
    return firstOrnamentWord([&](const OrnamentEntry& e) {
        return (e.kind == OKind::Ornament || e.kind == OKind::JazzOrnament) && e.sym == sym;
    });
}

std::string mu::dorico::trillText(const OrnamentInterval& interval)
{
    if (interval == OrnamentInterval()) {
        return "tr";
    }
    std::string quality;
    for (const TrillQuality& q : TRILL_QUALITIES) {
        if (q.type == interval.type) {
            quality = q.words.front();
        }
    }
    return "tr " + quality + std::to_string(int(interval.step) + 1);
}

std::string mu::dorico::arpeggioText(ArpeggioType type)
{
    return firstOrnamentWord([&](const OrnamentEntry& e) { return e.kind == OKind::Arpeggio && e.arpeggio == type; });
}

std::string mu::dorico::glissandoText(GlissandoType type)
{
    return type == GlissandoType::WAVY ? "glisswavy" : "gliss";
}

std::string mu::dorico::jazzArticulationText(ChordLineType type, bool smooth)
{
    return firstOrnamentWord([&](const OrnamentEntry& e) {
        return e.kind == OKind::JazzArticulation && e.chordLine == type && e.smooth == smooth;
    });
}

std::string mu::dorico::guitarText(OrnamentEntry::GuitarTech tech)
{
    return firstOrnamentWord([&](const OrnamentEntry& e) { return e.kind == OKind::Guitar && e.guitar == tech; });
}
