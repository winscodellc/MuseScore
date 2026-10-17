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

#include "tempoparser.h"

#include <algorithm>
#include <cctype>
#include <regex>

using namespace mu::dorico;
using mu::engraving::DurationType;
using mu::engraving::GradualTempoChangeType;
using Kind = TempoEntry::Kind;

namespace {
struct TempoWord {
    const char* name;
    double bpm;
};

// The Tempo palette's Italian terms and their quarter-note speeds (palettecreator.cpp, newTempoPalette). Lento is a
// relative change in Dorico, so it is not here.
const TempoWord TEMPO_WORDS[] = {
    { "Grave", 35 }, { "Largo", 50 }, { "Larghetto", 63 }, { "Adagio", 71 }, { "Andante", 92 }, { "Andantino", 94 },
    { "Moderato", 114 }, { "Allegretto", 116 }, { "Allegro", 144 }, { "Vivace", 172 }, { "Presto", 187 },
    { "Prestissimo", 200 },
};

// Dorico's absolute words with no speed of their own, and its relative changes: printed, no tempo change.
const char* const OTHER_ABSOLUTE[] = { "più", "meno", "Rubato" };
const char* const RELATIVE[] = { "Mosso", "Lento", "più mosso", "meno mosso" };

struct TempoGradual {
    const char* entry; // lower case, without the continuation dots
    const char* text;  // what is printed
    GradualTempoChangeType type;
};

const TempoGradual TEMPO_GRADUALS[] = {
    { "rit", "rit.", GradualTempoChangeType::Ritardando },
    { "rit.", "rit.", GradualTempoChangeType::Ritardando },
    { "ritardando", "ritardando", GradualTempoChangeType::Ritardando },
    { "rall", "rall.", GradualTempoChangeType::Rallentando },
    { "rall.", "rall.", GradualTempoChangeType::Rallentando },
    { "rallentando", "rallentando", GradualTempoChangeType::Rallentando },
    { "accel", "accel.", GradualTempoChangeType::Accelerando },
    { "accel.", "accel.", GradualTempoChangeType::Accelerando },
    { "accelerando", "accelerando", GradualTempoChangeType::Accelerando },
};

// Rhythmic feels (Dorico's swing playback): no MuseScore equivalent wired yet.
const char* const FEELS[] = {
    "light swing 16ths", "light swing 8ths", "medium swing 16ths", "medium swing 8ths", "heavy swing 16ths",
    "heavy swing 8ths", "straight (no swing)", "2:1 swing 16ths (fixed)", "2:1 swing 8ths (fixed)",
    "3:1 swing 16ths (fixed)", "3:1 swing 8ths (fixed)",
};

const char* const SUGGESTIONS[] = {
    "Grave", "Largo", "Larghetto", "Lento", "Adagio", "Andante", "Andantino", "Moderato", "Allegretto", "Allegro",
    "Vivace", "Presto", "Prestissimo", "Mosso", "più mosso", "meno mosso", "Rubato", "a tempo", "tempo primo",
    "rit.", "ritardando", "rall.", "rallentando", "accel.", "accelerando",
};

const size_t MAX_SUGGESTIONS = 8;

// The beat units: letters, or the digit codes 2-9 (2 = 64th ... 9 = double whole).
const std::pair<const char*, DurationType> UNITS[] = {
    { "2w", DurationType::V_BREVE }, { "z", DurationType::V_64TH }, { "y", DurationType::V_32ND },
    { "x", DurationType::V_16TH }, { "e", DurationType::V_EIGHTH }, { "q", DurationType::V_QUARTER },
    { "h", DurationType::V_HALF }, { "w", DurationType::V_WHOLE }, { "2", DurationType::V_64TH },
    { "3", DurationType::V_32ND }, { "4", DurationType::V_16TH }, { "5", DurationType::V_EIGHTH },
    { "6", DurationType::V_QUARTER }, { "7", DurationType::V_HALF }, { "8", DurationType::V_WHOLE },
    { "9", DurationType::V_BREVE },
};

// Lower-cases ASCII only, so "più" stays intact.
std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return c < 128 ? std::tolower(c) : c; });
    return s;
}

std::string trimTempo(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    return b == std::string::npos ? std::string() : s.substr(b, s.find_last_not_of(' ') - b + 1);
}

bool tempoBeginsWith(const std::string& word, const std::string& prefix)
{
    return word.compare(0, prefix.size(), prefix) == 0;
}

std::optional<BeatUnit> parseUnit(const std::string& token, bool lettersOnly)
{
    const size_t dot = token.find('.');
    const std::string name = token.substr(0, dot);
    if (dot != std::string::npos && token.find_first_not_of('.', dot) != std::string::npos) {
        return std::nullopt;
    }
    for (const auto& [entry, type] : UNITS) {
        if (name == entry && !(lettersOnly && std::isdigit(static_cast<unsigned char>(name.back())))) {
            return BeatUnit { type, dot == std::string::npos ? 0 : static_cast<int>(token.size() - dot) };
        }
    }
    return std::nullopt;
}

// The right side of `q=`: a speed, a range (unsupported) or a beat unit (an equation, unsupported).
ParseStatus parseSpeed(const std::string& s, double& bpm)
{
    static const std::regex number("\\d+(\\.\\d+)?");
    static const std::regex rangeStart("\\d+(\\.\\d+)?\\s*-\\s*");
    static const std::regex range("\\d+(\\.\\d+)?\\s*-\\s*\\d+(\\.\\d+)?");
    if (s.empty()) {
        return ParseStatus::Incomplete;
    }
    if (std::regex_match(s, number)) {
        bpm = std::stod(s);
        return bpm > 0 ? ParseStatus::Ok : ParseStatus::Invalid;
    }
    if (std::regex_match(s, rangeStart)) {
        return ParseStatus::Incomplete;
    }
    if (std::regex_match(s, range) || parseUnit(s, true)) {
        return ParseStatus::Unsupported; // ponytail: ranges could play their midpoint; equations need relative tempo
    }
    return ParseStatus::Invalid;
}

// Finds `<unit>=<speed>` in s, with the words left around it. Without a `<unit>=`, the whole entry is words.
ParseStatus parseMark(const std::string& s, TempoEntry& entry, std::string& words)
{
    const size_t eq = s.find('=');
    if (eq == std::string::npos) {
        words = s;
        return ParseStatus::Ok;
    }

    size_t tokenEnd = eq;
    while (tokenEnd > 0 && s[tokenEnd - 1] == ' ') {
        --tokenEnd;
    }
    size_t tokenStart = tokenEnd;
    while (tokenStart > 0 && s[tokenStart - 1] != ' ' && s[tokenStart - 1] != '(' && s[tokenStart - 1] != '[') {
        --tokenStart;
    }
    entry.unit = parseUnit(s.substr(tokenStart, tokenEnd - tokenStart), false);
    if (!entry.unit) {
        words = s; // "a=b" that is no metronome mark: free text
        return ParseStatus::Ok;
    }

    // (q=72) and [q=96] are closed by their bracket; a bare mark ends at the next space.
    const char open = tokenStart > 0 ? s[tokenStart - 1] : 0;
    const char close = open == '(' ? ')' : open == '[' ? ']' : 0;
    const size_t markStart = close ? tokenStart - 1 : tokenStart;
    size_t speedEnd = close ? s.find(close, eq + 1) : s.find(' ', s.find_first_not_of(' ', eq + 1));
    if (close && speedEnd == std::string::npos) {
        return ParseStatus::Incomplete;
    }
    speedEnd = std::min(speedEnd, s.size());
    const size_t afterMark = close ? speedEnd + 1 : speedEnd;

    const ParseStatus speed = parseSpeed(trimTempo(s.substr(eq + 1, speedEnd - eq - 1)), entry.bpm);
    if (speed != ParseStatus::Ok) {
        return speed;
    }
    entry.parenthesised = open == '(';
    entry.markHidden = open == '[';
    words = trimTempo(trimTempo(s.substr(0, markStart)) + " " + trimTempo(s.substr(std::min(afterMark, s.size()))));
    return ParseStatus::Ok;
}

bool contains(const char* const* begin, const char* const* end, const std::string& lowerWord)
{
    return std::any_of(begin, end, [&](const char* w) { return lower(w) == lowerWord; });
}
}

TempoParse mu::dorico::parseTempo(const std::string& text)
{
    TempoParse result;
    TempoEntry& entry = result.entry;
    std::string s = trimTempo(text);
    if (s.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }
    if (contains(std::begin(FEELS), std::end(FEELS), lower(s))) {
        result.status = ParseStatus::Unsupported;
        return result;
    }

    // [ ... ]: hidden, still plays. An entry that opens a bracket is unfinished until it closes it.
    if (s.front() == '[') {
        if (s.back() != ']') {
            result.status = ParseStatus::Incomplete;
            return result;
        }
        entry.hidden = true;
        s = trimTempo(s.substr(1, s.size() - 2));
    }

    std::string words;
    result.status = parseMark(s, entry, words);
    if (result.status != ParseStatus::Ok) {
        return result;
    }
    if (entry.hidden && entry.markHidden) {
        result.status = ParseStatus::Invalid; // [[q=96]]
        return result;
    }
    entry.text = words;
    const std::string w = lower(words);

    if (words.empty()) {
        entry.kind = Kind::Absolute;
        result.status = entry.unit ? ParseStatus::Ok : ParseStatus::Incomplete; // "[]"
        return result;
    }

    if (!entry.unit) {
        const bool line = w.size() > 3 && w.compare(w.size() - 3, 3, "...") == 0;
        const std::string base = line ? w.substr(0, w.size() - 3) : w;
        for (const TempoGradual& g : TEMPO_GRADUALS) {
            if (base == g.entry) {
                entry.kind = Kind::Gradual;
                entry.text = g.text;
                entry.gradual = g.type;
                entry.line = line;
                return result;
            }
        }
    }

    if (w == "a tempo") {
        entry.kind = Kind::ATempo;
    } else if (w == "tempo primo") {
        entry.kind = Kind::TempoPrimo;
    } else if (contains(std::begin(RELATIVE), std::end(RELATIVE), w)) {
        entry.kind = Kind::Relative;
    } else if (contains(std::begin(OTHER_ABSOLUTE), std::end(OTHER_ABSOLUTE), w)) {
        entry.kind = Kind::Absolute;
    } else {
        entry.kind = Kind::FreeText;
        for (const TempoWord& t : TEMPO_WORDS) {
            if (lower(t.name) == w) {
                entry.kind = Kind::Absolute;
                entry.wordBpm = t.bpm;
            }
        }
    }
    return result;
}

std::vector<std::string> mu::dorico::tempoSuggestions(const std::string& prefix)
{
    const std::string p = lower(trimTempo(prefix));
    std::vector<std::string> result;
    if (p.empty()) {
        return result;
    }
    for (const char* s : SUGGESTIONS) {
        const std::string word = lower(s);
        if (tempoBeginsWith(word, p) && word != p && result.size() < MAX_SUGGESTIONS) { // the typed entry itself adds nothing
            result.push_back(s);
        }
    }
    return result;
}

std::string mu::dorico::beatUnitText(const BeatUnit& unit)
{
    for (const auto& [entry, type] : UNITS) {
        if (type == unit.type) {
            return entry + std::string(unit.dots, '.');
        }
    }
    return "q";
}
