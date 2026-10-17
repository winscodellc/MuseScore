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

#include "holdsparser.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace mu::dorico;
using mu::engraving::SymId;

namespace {
struct HoldWords {
    HoldEntry::Kind kind;
    SymId sym;
    std::vector<std::string> words; // the first one is the canonical entry (prefill)
};

const std::vector<HoldWords> HOLDS = {
    { HoldEntry::Kind::Fermata, SymId::fermataAbove,           { "fer", "fermata", "pause" } },
    { HoldEntry::Kind::Fermata, SymId::fermataVeryLongAbove,   { "verylongfer", "verylongfermata" } },
    { HoldEntry::Kind::Fermata, SymId::fermataLongAbove,       { "longfer", "longfermata" } },
    { HoldEntry::Kind::Fermata, SymId::fermataShortAbove,      { "shortfer", "shortfermata" } },
    { HoldEntry::Kind::Fermata, SymId::fermataVeryShortAbove,  { "veryshortfer", "veryshortfermata" } },
    { HoldEntry::Kind::Fermata, SymId::fermataShortHenzeAbove, { "shorthenzefer", "shorthenzefermata" } },
    { HoldEntry::Kind::Fermata, SymId::fermataLongHenzeAbove,  { "longhenzefer", "longhenzefermata" } },
    { HoldEntry::Kind::Fermata, SymId::curlewSign,             { "curlew" } },
    { HoldEntry::Kind::Breath,  SymId::breathMarkComma,        { "comma", ",", "breathmarkcomma" } },
    { HoldEntry::Kind::Breath,  SymId::breathMarkTick,         { "breathmarktick" } },
    { HoldEntry::Kind::Breath,  SymId::breathMarkUpbow,        { "breathmarkupbow" } },
    { HoldEntry::Kind::Breath,  SymId::breathMarkSalzedo,      { "breathmarksalzedo" } },
    { HoldEntry::Kind::Caesura, SymId::caesura,                { "//", "caes", "caesura" } },
    { HoldEntry::Kind::Caesura, SymId::caesuraThick,           { "thick//", "caesurathick", "thickcaesura" } },
    { HoldEntry::Kind::Caesura, SymId::caesuraCurved,          { "curved//", "caesuracurved", "curvedcaesura" } },
    { HoldEntry::Kind::Caesura, SymId::caesuraShort,           { "short//", "caesurashort", "shortcaesura" } },
    { HoldEntry::Kind::Caesura, SymId::caesuraSingleStroke,    { "single//", "caesurasingle", "singlecaesura" } },
};

// Trimmed and lower case: hold entries ignore case.
std::string holdsNormalised(const std::string& s)
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
}

HoldParse mu::dorico::parseHold(const std::string& input)
{
    const std::string text = holdsNormalised(input);
    if (text.empty()) {
        return { ParseStatus::Incomplete, {} };
    }
    bool prefix = false;
    for (const HoldWords& h : HOLDS) {
        for (const std::string& w : h.words) {
            if (w == text) {
                return { ParseStatus::Ok, { h.kind, h.sym } };
            }
            prefix = prefix || w.compare(0, text.size(), text) == 0;
        }
    }
    return { prefix ? ParseStatus::Incomplete : ParseStatus::Invalid, {} };
}

std::string mu::dorico::holdText(SymId sym)
{
    auto it = std::find_if(HOLDS.begin(), HOLDS.end(), [&](const HoldWords& h) { return h.sym == sym; });
    return it == HOLDS.end() ? std::string() : it->words.front();
}
