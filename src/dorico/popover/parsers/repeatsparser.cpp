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

#include "repeatsparser.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace mu::dorico;
using mu::engraving::JumpType;
using mu::engraving::MarkerType;
using mu::engraving::TremoloType;
using RepeatsKind = RepeatsEntry::Kind;

namespace {
struct RepeatsWords {
    RepeatsEntry entry;
    std::vector<std::string> words; // any case; the first one is the canonical entry (prefill)
};

RepeatsEntry markerEntry(MarkerType t)
{
    RepeatsEntry e;
    e.kind = RepeatsKind::Marker;
    e.marker = t;
    return e;
}

RepeatsEntry jumpEntry(JumpType t)
{
    RepeatsEntry e;
    e.kind = RepeatsKind::Jump;
    e.jump = t;
    return e;
}

RepeatsEntry tremoloEntry(TremoloType t)
{
    RepeatsEntry e;
    e.kind = RepeatsKind::Tremolo;
    e.tremolo = t;
    return e;
}

RepeatsEntry kindEntry(RepeatsKind kind, bool add = false, int measures = 0)
{
    RepeatsEntry e;
    e.kind = kind;
    e.add = add;
    e.measures = measures;
    return e;
}

const std::vector<RepeatsWords> REPEATS = {
    { kindEntry(RepeatsKind::Ending),                     { "end", "ending" } },
    { kindEntry(RepeatsKind::Ending, true),               { "add" } },
    { jumpEntry(JumpType::DC),                     { "D.C.", "da capo", "dc" } },
    { jumpEntry(JumpType::DC_AL_FINE),             { "D.C. al Fine", "DC al Fine", "dcalf" } },
    { jumpEntry(JumpType::DC_AL_CODA),             { "D.C. al Coda", "DC al Coda", "dcalc" } },
    { jumpEntry(JumpType::DS),                     { "D.S.", "dal segno", "ds" } },
    { jumpEntry(JumpType::DS_AL_FINE),             { "D.S. al Fine", "DS al Fine", "dsalf" } },
    { jumpEntry(JumpType::DS_AL_CODA),             { "D.S. al Coda", "DS al Coda", "dsalc" } },
    { markerEntry(MarkerType::TOCODA),             { "to coda", "toc", "tc" } },
    { markerEntry(MarkerType::SEGNO),              { "segno", "seg", "s", "$" } },
    { markerEntry(MarkerType::FINE),               { "fine", "fin", "f" } },
    { markerEntry(MarkerType::CODA),               { "coda", "co", "c" } },
    { tremoloEntry(TremoloType::R8),               { "/", "\\", "1" } },
    { tremoloEntry(TremoloType::R16),              { "//", "\\\\", "2" } },
    { tremoloEntry(TremoloType::R32),              { "///", "\\\\\\", "3" } },
    { tremoloEntry(TremoloType::R64),              { "////", "\\\\\\\\", "4" } },
    { tremoloEntry(TremoloType::BUZZ_ROLL),        { "z", "zonstem" } },
    { tremoloEntry(TremoloType::C8),               { "/2", "\\2", "12" } },
    { tremoloEntry(TremoloType::C16),              { "//2", "\\\\2", "22" } },
    { tremoloEntry(TremoloType::C32),              { "///2", "\\\\\\2", "32" } },
    { tremoloEntry(TremoloType::C64),              { "////2", "\\\\\\\\2", "42" } },
    { kindEntry(RepeatsKind::ClearTremolo),               { "0", "clear" } },
    { kindEntry(RepeatsKind::MeasureRepeat, false, 1),    { "%1", "%" } },
    { kindEntry(RepeatsKind::MeasureRepeat, false, 2),    { "%2" } },
    { kindEntry(RepeatsKind::MeasureRepeat, false, 4),    { "%4" } },
    { kindEntry(RepeatsKind::Slash),                      { "slash" } },
};

// Valid Dorico entries MuseScore has no model for.
const std::vector<std::string> REPEATS_UNSUPPORTED = {
    "//rel", "//r", "2rel", "2r",          // tremolo with release
    "//att", "//a", "2att", "2a",          // tremolo with attack
    "number bars", "numbars", "num", "nb", // numbered bar region
    "%1,2", "%1,4", "%2,4",                // grouped bar repeats
};

// Trimmed and lower case: repeat entries ignore case ("DC al Fine" = "dc al fine").
std::string repeatsNormalised(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    std::string text = s.substr(b, s.find_last_not_of(' ') - b + 1);
    for (char& c : text) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

bool startsWithText(const std::string& word, const std::string& text)
{
    return word.compare(0, text.size(), text) == 0;
}
}

RepeatsParse mu::dorico::parseRepeats(const std::string& input)
{
    const std::string text = repeatsNormalised(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }
    bool prefix = false;
    for (const RepeatsWords& r : REPEATS) {
        for (const std::string& word : r.words) {
            const std::string w = repeatsNormalised(word);
            if (w == text) {
                return { ParseStatus::Ok, r.entry };
            }
            prefix = prefix || startsWithText(w, text);
        }
    }
    for (const std::string& w : REPEATS_UNSUPPORTED) {
        if (w == text) {
            return { ParseStatus::Unsupported, {} };
        }
        prefix = prefix || startsWithText(w, text);
    }
    // %3, %8, ...: a bar repeat MuseScore cannot draw (it has 1, 2 and 4 bars).
    if (text.size() > 1 && text[0] == '%'
        && std::all_of(text.begin() + 1, text.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return { ParseStatus::Unsupported, {} };
    }
    return { prefix ? ParseStatus::Incomplete : ParseStatus::Invalid, {} };
}

std::string mu::dorico::repeatsText(const RepeatsEntry& entry)
{
    auto it = std::find_if(REPEATS.begin(), REPEATS.end(), [&](const RepeatsWords& r) { return r.entry == entry; });
    return it == REPEATS.end() ? std::string() : it->words.front();
}
