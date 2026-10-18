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

#include "chordsymbolsparser.h"

#include <cctype>
#include <cstring>
#include <utility>

#include "engraving/dom/chordlist.h"

using namespace mu::dorico;

// Named, not anonymous: the unity build may compile the parsers together, and they share helper names.
namespace chordsymbolsparser {
using Rule = std::pair<const char*, const char*>;

// Words a chord is still being typed towards: their prefixes are Incomplete (neutral border), not Invalid.
const char* const KEYWORDS[] = {
    "half-dim", "halfdim", "dim", "aug", "maj", "min", "sus", "add", "omit", "no chord", "none", "n.c.",
    "ionian", "dorian", "phrygian", "lydian", "mixolydian", "aeolian", "locrian", "melodic minor", "harmonic minor",
    "whole tone", "diminished", "octatonic",
};

// Valid Dorico entries MuseScore has no model for.
const char* const MODES[] = {
    "ionian", "dorian", "phrygian", "lydian", "mixolydian", "aeolian", "locrian", "melodic minor", "harmonic minor",
    "whole tone", "diminished half whole", "diminished semitone tone", "octatonic half whole", "octatonic semitone tone",
    "diminished whole half", "diminished tone semitone", "octatonic whole half", "octatonic tone semitone",
};

// Dorico quality spellings at the start of the chord, compared in lower case; first match wins.
const Rule QUALITIES[] = {
    { "half-dim7", "m7b5" }, { "half-dim", "m7b5" }, { "halfdim7", "m7b5" }, { "halfdim", "m7b5" },
    { "hd7", "m7b5" }, { "hd", "m7b5" },
    { "omit", "omit" }, { "sus", "sus" }, { "add", "add" }, // not o / s / a qualities
    { "maj", "maj" }, { "ma", "maj" }, { "min", "m" }, { "mi", "m" },
    { "dim", "dim" }, { "di", "dim" }, { "aug", "aug" }, { "au", "aug" }, { "ag", "aug" },
    { "6/9", "6/9" }, { "69", "6/9" }, { "%", "6/9" }, { "+", "aug" }, { "o", "dim" },
};

struct Note {
    std::string text;    // English spelling: C, Db, F##
    size_t length = 0;   // characters read; 0 = no note here
    bool triple = false; // x#, ###, bbb: valid Dorico, but MuseScore spells at most a double accidental
};

std::string lowerCase(std::string s)
{
    for (char& c : s) {
        c = char(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string trimSpaces(const std::string& s)
{
    const size_t b = s.find_first_not_of(' ');
    return b == std::string::npos ? std::string() : s.substr(b, s.find_last_not_of(' ') - b + 1);
}

bool startsWith(const std::string& s, size_t i, const char* word)
{
    return s.compare(i, std::strlen(word), word) == 0;
}

// A root or bass at s[i]: English (C, Db, Fx), German (H, Fis, Des, Es, As) or fixed-do (do, reb, fa#), as English.
Note note(const std::string& s, size_t i)
{
    static const std::pair<const char*, char> FIXED_DO[] = {
        { "sol", 'G' }, { "do", 'C' }, { "re", 'D' }, { "mi", 'E' }, { "fa", 'F' }, { "la", 'A' }, { "si", 'B' }, { "ti", 'B' },
    };
    static const Rule GERMAN[] = { { "isis", "##" }, { "is", "#" }, { "eses", "bb" }, { "es", "b" } };
    static const Rule ACCIDENTALS[] = {
        { "###", nullptr }, { "x#", nullptr }, { "#x", nullptr }, { "bbb", nullptr },
        { "##", "##" }, { "x", "##" }, { "#", "#" }, { "bb", "bb" }, { "b", "b" },
    };

    Note n;
    for (const auto& [syllable, letter] : FIXED_DO) {
        if (startsWith(s, i, syllable)) {
            n.text = letter;
            n.length = std::strlen(syllable);
            break;
        }
    }
    if (!n.length) {
        if (i >= s.size() || s[i] < 'A' || s[i] > 'H') {
            return n;
        }
        n.text = s[i] == 'H' ? 'B' : s[i];
        n.length = 1;
        for (const auto& [suffix, accidental] : GERMAN) {
            if (startsWith(s, i + 1, suffix)) {
                n.text += accidental;
                n.length += std::strlen(suffix);
                return n;
            }
        }
        // As, Es, Ases; but Esus4 is E sus4
        if ((s[i] == 'A' || s[i] == 'E') && i + 1 < s.size() && s[i + 1] == 's' && !startsWith(s, i + 2, "u")) {
            const bool doubleFlat = startsWith(s, i + 1, "ses");
            n.text += doubleFlat ? "bb" : "b";
            n.length += doubleFlat ? 3 : 1;
            return n;
        }
    }
    for (const auto& [accidental, spelling] : ACCIDENTALS) {
        if (startsWith(s, i + n.length, accidental)) {
            n.length += std::strlen(accidental);
            n.triple = !spelling;
            n.text += spelling ? spelling : "";
            break;
        }
    }
    return n;
}

// Alterations anywhere after the quality: ^7 / ^ = maj7, no3 = omit3, -5 = b5, +9 = #9, 9maj7 = maj9.
std::string alterations(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        const bool digitNext = i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1]));
        if (s[i] == '^') {
            out += digitNext ? "maj" : "maj7";
        } else if (startsWith(s, i, "no") && i + 2 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 2]))) {
            out += "omit";
            ++i;
        } else if (s[i] == '-' && digitNext) {
            out += 'b';
        } else if (s[i] == '+' && digitNext) {
            out += '#';
        } else {
            out += s[i];
        }
    }
    const size_t nine = out.find("9maj7");
    if (nine != std::string::npos) {
        out.replace(nine, 5, "maj9");
    }
    return out;
}

// Quality and alterations, in MuseScore's spelling.
std::string chordBody(const std::string& s)
{
    if (s == "M") { // case-sensitive major; plain C is major
        return std::string();
    }
    const std::string lower = lowerCase(s);
    for (const auto& [dorico, musescore] : QUALITIES) {
        if (startsWith(lower, 0, dorico)) {
            return musescore + alterations(s.substr(std::strlen(dorico)));
        }
    }
    if (!s.empty() && s[0] == 'M') {
        return "maj" + alterations(s.substr(1));
    }
    return alterations(s);
}

// Does MuseScore's chord parser understand this quality and extension?
bool understood(const std::string& body)
{
    mu::engraving::ChordList chordList; // empty: no chord descriptions are needed to classify the tokens
    mu::engraving::ParsedChord chord;
    chord.parse(muse::String::fromStdString(body), &chordList);
    return chord.understandable();
}

// Is the text on its way to a keyword (`Chalf-d`, `Csu`, `no ch`)?
bool typingKeyword(const std::string& text)
{
    size_t start = text.size();
    while (start > 0 && (std::isalpha(static_cast<unsigned char>(text[start - 1])) || text[start - 1] == '-'
                         || text[start - 1] == ' ' || text[start - 1] == '.')) {
        --start;
    }
    const std::string tail = lowerCase(text.substr(start));
    for (size_t i = 0; i < tail.size(); ++i) { // any suffix of the tail: "Cm7su" types towards sus
        for (const char* word : KEYWORDS) {
            const std::string part = tail.substr(i);
            if (!part.empty() && part != word && std::string(word).compare(0, part.size(), part) == 0) {
                return true;
            }
        }
    }
    return false;
}

// One chord of a polychord: root, quality and extensions, optional bass after / or ,.
ChordSymbolParse parseOne(const std::string& text)
{
    ChordSymbolParse result;
    if (text.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }
    if (std::isdigit(static_cast<unsigned char>(text[0]))) {
        result.status = ParseStatus::Unsupported; // Nashville numbers need the Nashville chord style
        return result;
    }

    const Note root = note(text, 0);
    if (!root.length) {
        return result;
    }
    std::string body = text.substr(root.length);
    std::string bass;
    bool triple = root.triple;
    const size_t slash = body.find_last_of("/,");
    if (slash != std::string::npos) {
        const Note b = note(body, slash + 1);
        if (b.length && slash + 1 + b.length == body.size()) {
            bass = "/" + b.text;
            triple |= b.triple;
            body.resize(slash);
        }
    }

    const std::string lower = lowerCase(trimSpaces(body));
    for (const char* mode : MODES) {
        if (lower == mode) {
            triple = true; // reported as Unsupported below
        }
    }
    const size_t add = body.find("add");
    if (add != std::string::npos && add + 3 < body.size() && body[add + 3] >= 'A' && body[add + 3] <= 'H') {
        triple = true; // addF#: an added note by name; MuseScore adds degrees only
    }
    if (triple) {
        result.status = ParseStatus::Unsupported;
        return result;
    }

    body = chordBody(trimSpaces(body));
    if (!understood(body)) {
        result.status = typingKeyword(text) ? ParseStatus::Incomplete : ParseStatus::Invalid;
        return result;
    }
    result.status = ParseStatus::Ok;
    result.normalized = root.text + body + bass;
    return result;
}
}

ChordSymbolParse mu::dorico::parseChordSymbol(const std::string& input)
{
    using namespace chordsymbolsparser;

    std::string text = trimSpaces(input);
    ChordSymbolParse result;
    if (text.empty()) {
        result.status = ParseStatus::Incomplete;
        return result;
    }

    const std::string lower = lowerCase(text);
    if (lower == "n.c." || lower == "n.c" || lower == "nc" || lower == "no chord" || lower == "none") {
        return { ParseStatus::Ok, "N.C." };
    }
    if (lower == "reg") {
        result.status = ParseStatus::Unsupported; // chord symbol regions
        return result;
    }

    // (Cmaj7): MuseScore's Harmony keeps the parentheses itself.
    const bool open = text.front() == '(';
    const bool closed = open && text.size() > 1 && text.back() == ')';
    if (open) {
        text = trimSpaces(text.substr(1, text.size() - (closed ? 2 : 1)));
    }

    // Cmaj7|D, Cmaj7;D: a polychord, MuseScore's | form
    std::string normalized;
    size_t start = 0;
    for (;;) {
        const size_t end = text.find_first_of("|;", start);
        const ChordSymbolParse part = parseOne(trimSpaces(text.substr(start, end == std::string::npos ? end : end - start)));
        if (part.status != ParseStatus::Ok) {
            result.status = part.status;
            return result;
        }
        normalized += (start ? "|" : "") + part.normalized;
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    if (open && !closed) {
        result.status = ParseStatus::Incomplete; // "(Cmaj7" waits for its ")"
        return result;
    }
    result.status = ParseStatus::Ok;
    result.normalized = open ? "(" + normalized + ")" : normalized;
    return result;
}
