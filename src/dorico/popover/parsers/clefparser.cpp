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

#include "clefparser.h"

#include <algorithm>
#include <cctype>
#include <vector>

using namespace mu::dorico;
using mu::engraving::ClefType;

namespace {
struct ClefWords {
    ClefType type; // INVALID: Dorico syntax MuseScore has no clef for
    std::vector<std::string> words; // the first one is the canonical entry (prefill)
};

const std::vector<ClefWords> CLEFS = {
    { ClefType::G,      { "treble", "g", "g2", "sol" } },
    { ClefType::F,      { "bass", "f", "f4", "fa" } },
    { ClefType::C4,     { "tenor", "ct", "c4", "ut4" } },
    { ClefType::C3,     { "alto", "ca", "c3", "ut3" } },
    { ClefType::G8_VB,  { "treble8ba", "g8ba", "g8d", "treble8d" } },
    { ClefType::G15_MB, { "treble15ba", "g15ba", "g15d", "treble15d" } },
    { ClefType::G8_VA,  { "treble8va", "g8va", "g8u", "treble8u" } },
    { ClefType::G15_MA, { "treble15ma", "g15ma", "g15u", "treble15u" } },
    { ClefType::INVALID, { "alto8ba", "ca8ba", "ca8d", "alto8d" } }, // no alto 8vb clef in MuseScore
    { ClefType::C4_8VB, { "tenor8ba", "ct8ba", "ct8d", "tenor8d" } },
    { ClefType::F8_VB,  { "bass8ba", "f8ba", "f8d", "bass8d" } },
    { ClefType::F15_MB, { "bass15ba", "f15ba", "f15d", "bass15d" } },
    { ClefType::F_8VA,  { "bass8va", "f8va", "f8u", "bass8u" } },
    { ClefType::F_15MA, { "bass15ma", "f15ma", "f15u", "bass15u" } },
    { ClefType::PERC,   { "perc" } },
    { ClefType::TAB4,   { "tab4" } },
    { ClefType::TAB,    { "tab6" } },
    { ClefType::G_1,    { "g1" } },
    { ClefType::C1,     { "soprano", "c1", "ut1" } },
    { ClefType::C2,     { "mezzo", "c2", "ut2" } },
    { ClefType::C5,     { "baritone", "c5", "ut5" } },
    { ClefType::F_B,    { "baritonebass" } },
    { ClefType::F_C,    { "subbass" } },
    { ClefType::INVALID, { "invisible" } }, // no invisible clef in MuseScore
};

struct OctaveWords {
    int octaves;
    std::vector<std::string> words;
};

const std::vector<OctaveWords> OCTAVES = {
    { 1, { "8va", "8", "8u", "1u" } },
    { 2, { "15ma", "15", "15u", "2u" } },
    { 3, { "22ma", "22", "22u", "3u" } },
    { -1, { "8ba", "8vb", "8d", "1d" } },
    { -2, { "15ba", "15vb", "15d", "2d" } },
    { -3, { "22ba", "22vb", "22d", "3d" } },
};

// Clefs with octave variants, from 2 octaves down to 2 up; `g+1` moves one step right. INVALID: no such clef.
const std::vector<std::vector<ClefType> > OCTAVE_FAMILIES = {
    { ClefType::G15_MB, ClefType::G8_VB, ClefType::G, ClefType::G8_VA, ClefType::G15_MA },
    { ClefType::F15_MB, ClefType::F8_VB, ClefType::F, ClefType::F_8VA, ClefType::F_15MA },
    { ClefType::INVALID, ClefType::C4_8VB, ClefType::C4, ClefType::INVALID, ClefType::INVALID },
};

const std::vector<std::string> OTHER_WORDS = { "stop", "loco" };

// Trimmed and lower case: clef entries ignore case.
std::string normalised(const std::string& s)
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

bool contains(const std::vector<std::string>& words, const std::string& w)
{
    return std::find(words.begin(), words.end(), w) != words.end();
}

const ClefWords* findClef(const std::string& word)
{
    auto it = std::find_if(CLEFS.begin(), CLEFS.end(), [&](const ClefWords& c) { return contains(c.words, word); });
    return it == CLEFS.end() ? nullptr : &*it;
}

ClefParse ok(ClefEntry entry)
{
    return { ParseStatus::Ok, entry };
}

ClefParse status(ParseStatus s)
{
    return { s, {} };
}

ClefParse clefResult(ClefType type)
{
    return type == ClefType::INVALID ? status(ParseStatus::Unsupported) : ok({ ClefEntry::Kind::Clef, type, 0 });
}

// `g+2`, `f-1`, `treble+1t`: a clef shifted by whole octaves.
ClefParse shiftedClef(const ClefWords& base, const std::string& shift)
{
    const int sign = shift[0] == '+' ? 1 : -1;
    size_t i = 1;
    int n = 0;
    while (i < shift.size() && std::isdigit(static_cast<unsigned char>(shift[i]))) {
        n = std::min(n * 10 + (shift[i] - '0'), 99);
        ++i;
    }
    if (i == 1) {
        return status(shift.size() == 1 ? ParseStatus::Incomplete : ParseStatus::Invalid);
    }
    if (i < shift.size()) {
        const bool pitchSuffix = i + 1 == shift.size() && (shift[i] == 'c' || shift[i] == 't');
        return status(pitchSuffix ? ParseStatus::Unsupported : ParseStatus::Invalid); // ponytail: c / t not supported yet
    }

    for (const std::vector<ClefType>& family : OCTAVE_FAMILIES) {
        auto it = std::find(family.begin(), family.end(), base.type);
        if (base.type == ClefType::INVALID || it == family.end()) {
            continue;
        }
        const int index = int(it - family.begin()) + sign * n;
        return clefResult(index >= 0 && index < int(family.size()) ? family[index] : ClefType::INVALID);
    }
    return status(ParseStatus::Unsupported); // perc+1, alto+1: no such clef in MuseScore
}

bool isPrefixOfAnyWord(const std::string& s)
{
    auto prefixOf = [&](const std::vector<std::string>& words) {
        return std::any_of(words.begin(), words.end(), [&](const std::string& w) { return w.compare(0, s.size(), s) == 0; });
    };
    return prefixOf(OTHER_WORDS)
           || std::any_of(CLEFS.begin(), CLEFS.end(), [&](const ClefWords& c) { return prefixOf(c.words); })
           || std::any_of(OCTAVES.begin(), OCTAVES.end(), [&](const OctaveWords& o) { return prefixOf(o.words); });
}
}

ClefParse mu::dorico::parseClef(const std::string& input)
{
    const std::string text = normalised(input);
    if (text.empty()) {
        return status(ParseStatus::Incomplete);
    }
    if (text == "|" || text == "stop") {
        return ok({ ClefEntry::Kind::Stop, ClefType::INVALID, 0 });
    }
    if (text == "loco") {
        return status(ParseStatus::Unsupported); // a loco line: MuseScore has no such octave line type
    }
    for (const OctaveWords& o : OCTAVES) {
        if (contains(o.words, text)) {
            return ok({ ClefEntry::Kind::OctaveLine, ClefType::INVALID, o.octaves });
        }
    }
    if (const ClefWords* c = findClef(text)) {
        return clefResult(c->type);
    }

    const size_t sign = text.find_first_of("+-");
    if (sign != std::string::npos) {
        const ClefWords* base = findClef(text.substr(0, sign));
        return base ? shiftedClef(*base, text.substr(sign)) : status(ParseStatus::Invalid);
    }
    return status(isPrefixOfAnyWord(text) ? ParseStatus::Incomplete : ParseStatus::Invalid);
}

std::string mu::dorico::clefText(ClefType type)
{
    if (type == ClefType::INVALID) {
        return std::string();
    }
    auto it = std::find_if(CLEFS.begin(), CLEFS.end(), [&](const ClefWords& c) { return c.type == type; });
    return it == CLEFS.end() ? std::string() : it->words.front();
}

std::string mu::dorico::octaveLineText(int octaves)
{
    auto it = std::find_if(OCTAVES.begin(), OCTAVES.end(), [&](const OctaveWords& o) { return o.octaves == octaves; });
    return it == OCTAVES.end() ? std::string() : it->words.front();
}
